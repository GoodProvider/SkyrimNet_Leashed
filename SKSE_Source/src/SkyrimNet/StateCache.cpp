#include "StateCache.h"

#include "Api.h"
#include "Leash/LeashConstants.h"
#include "Leash/LeashState.h"
#include "WebUI/Config.h"

#include <glaze/glaze.hpp>

#include <Windows.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stop_token>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace SkyrimNetLeashed::SkyrimNet::StateCache {
    struct PairJson {
        std::string holder{};
        std::string leashed{};
        bool tied{false};
        bool dangling{false};
        std::string kind{};
        std::string distance{};
        std::string bodyPart{};
    };

    struct PairsJson {
        std::vector<PairJson> pairs{};
    };

    struct ActorsJson {
        std::vector<std::uint64_t> actorIds{};
        std::string actorsNameString{};
    };

    namespace {
        constexpr std::string_view kEmptyPairs = R"({"pairs":[]})";
        constexpr std::string_view kEmptyActors = R"({"actorIds":[],"actorsNameString":""})";
        constexpr auto kRefreshInterval = std::chrono::milliseconds(750);
        constexpr auto kSlowRefresh = std::chrono::milliseconds(10);
        constexpr auto kSlowRefreshLogInterval = std::chrono::seconds(60);

        struct Snapshot {
            bool leashAvailable{false};
            bool unleashAvailable{false};
            bool leashedNearby{false};
            bool unleashedNearby{false};
            bool speakerOnLeash{false};
            bool speakerIsLeashed{false};
            bool collaredNearby{false};
            bool speakerIsStruggling{false};
            bool speakerHoldsLeash{false};
            bool speakerDownOrPulled{false};
            std::string visiblePairs{kEmptyPairs};
            std::string leashedActors{kEmptyActors};
            std::string unleashedActors{kEmptyActors};
            std::string leashPartners{kEmptyActors};
            std::string heldActors{kEmptyActors};
            std::string collaredActors{kEmptyActors};
            std::string nearbyActors{kEmptyActors};
        };

        struct Pair {
            RE::FormID holderID{};
            RE::FormID leashedID{};
            std::string holderName;
            std::string leashedName;
            std::string kind;
            std::string distance;
            std::string bodyPart;
            bool tied{false};
            bool dangling{false};
            RE::Actor* holder{};
            RE::Actor* leashed{};
        };

        std::mutex g_mutex;
        std::unordered_map<RE::FormID, Snapshot> g_snapshots;
        // Speakers SkyrimNet asked about that the last process-list scan missed. The next
        // refresh resolves them on the main thread so the miss is self-healing.
        std::unordered_set<RE::FormID> g_pending;
        std::atomic<bool> g_struggleEnabled{true};
        // At most one refresh waits in the SKSE task queue. The queue is not drained while
        // the game is paused or unfocused, so queueing on every tick would build a backlog
        // that all runs back to back when the game resumes.
        std::atomic<bool> g_refreshQueued{false};
        std::atomic<DWORD> g_mainThreadID{0};
        std::atomic<bool> g_loggedRefreshThread{false};
        // Off-main-thread refreshes since the last report; logged at most once per minute.
        std::atomic<std::uint32_t> g_offMainRefreshes{0};
        std::atomic<DWORD> g_lastOffMainThread{0};
        std::atomic<std::int64_t> g_lastOffMainLogMs{0};
        std::atomic<std::int64_t> g_lastSlowLogMs{0};

        std::mutex g_wakeMutex;
        std::condition_variable_any g_wake;
        std::jthread g_thread;

        RE::Actor* ActorFromID(RE::FormID a_id) {
            auto* form = a_id != 0 ? RE::TESForm::LookupByID(a_id) : nullptr;
            return form ? form->As<RE::Actor>() : nullptr;
        }

        bool CanSee(RE::Actor* a_speaker, RE::Actor* a_target) {
            // Pairs and pending speakers can be in unloaded cells.
            if (!a_speaker || !a_target || !a_speaker->Is3DLoaded() || !a_target->Is3DLoaded()) {
                return false;
            }
            bool los = false;
            if (!a_speaker->HasLineOfSight(a_target, los)) {
                return false;
            }
            return los;
        }

        std::string SpokenKind(std::string_view a_kind) {
            if (a_kind == "holder_shield") {
                return "shield";
            }
            return std::string{a_kind};
        }

        std::string FormatNames(const std::vector<std::string>& a_names) {
            if (a_names.empty()) {
                return {};
            }
            if (a_names.size() == 1) {
                return a_names.front();
            }
            std::string result;
            for (std::size_t i = 0; i + 1 < a_names.size(); ++i) {
                if (i > 0) {
                    result += ", ";
                }
                result += a_names[i];
            }
            result += " and " + a_names.back();
            return result;
        }

        template <class T>
        std::string WriteJson(const T& a_payload, std::string_view a_fallback) {
            std::string json;
            if (auto err = glz::write_json(a_payload, json); err) {
                return std::string{a_fallback};
            }
            return json;
        }

        // Per-actor values for one refresh. BuildSnapshot runs for every speaker over every
        // nearby actor, so each of these would otherwise be recomputed N times. Labels stay
        // lazy: only actors that land in a payload ask SkyrimNet for a UUID.
        class RefreshTable {
        public:
            struct Status {
                bool dead{false};
                bool leashed{false};
                bool leashedOrHolder{false};
            };

            struct Label {
                std::uint64_t uuid{0};
                std::string name;
            };

            const Status& StatusOf(RE::Actor* a_actor) {
                const auto [it, inserted] = _statuses.try_emplace(a_actor);
                if (inserted) {
                    it->second.dead = a_actor->IsDead();
                    it->second.leashed = LeashState::IsLeashed(a_actor);
                    it->second.leashedOrHolder = it->second.leashed || LeashState::IsLeashHolder(a_actor);
                }
                return it->second;
            }

            const Label& LabelOf(RE::Actor* a_actor) {
                const auto [it, inserted] = _labels.try_emplace(a_actor);
                if (inserted) {
                    it->second.uuid = Api::FormIDToUUID(a_actor->GetFormID());
                    it->second.name = LeashState::DisplayName(a_actor);
                }
                return it->second;
            }

        private:
            std::unordered_map<RE::Actor*, Status> _statuses;
            std::unordered_map<RE::Actor*, Label> _labels;
        };

        std::string ActorsPayload(const std::vector<RE::Actor*>& a_actors, RefreshTable& a_table) {
            ActorsJson payload;
            std::vector<std::string> names;
            names.reserve(a_actors.size());
            for (auto* actor : a_actors) {
                const auto& label = a_table.LabelOf(actor);
                if (label.uuid != 0) {
                    payload.actorIds.push_back(label.uuid);
                }
                names.push_back(label.name);
            }
            payload.actorsNameString = FormatNames(names);
            return WriteJson(payload, kEmptyActors);
        }

        PairJson MakePairJson(const Pair& a_pair) {
            return PairJson{
                .holder = a_pair.holderName,
                .leashed = a_pair.leashedName,
                .tied = a_pair.tied,
                .dangling = a_pair.dangling,
                .kind = a_pair.kind,
                .distance = a_pair.distance,
                .bodyPart = a_pair.bodyPart,
            };
        }

        Snapshot BuildSnapshot(RE::Actor* a_speaker, const std::vector<RE::Actor*>& a_nearby, const std::vector<Pair>& a_pairs, bool a_pluginLoaded, RefreshTable& a_table) {
            Snapshot snapshot;
            const auto speakerID = a_speaker->GetFormID();
            const auto& speakerStatus = a_table.StatusOf(a_speaker);

            snapshot.leashAvailable = a_pluginLoaded && !speakerStatus.dead && !a_speaker->IsInCombat();
            snapshot.unleashAvailable = speakerStatus.leashedOrHolder;

            PairsJson visible;
            std::vector<RE::Actor*> partners;
            std::vector<RE::Actor*> held;
            for (const auto& pair : a_pairs) {
                const bool endpoint = pair.holderID == speakerID || pair.leashedID == speakerID;
                if (endpoint || CanSee(a_speaker, pair.holder) || CanSee(a_speaker, pair.leashed)) {
                    visible.pairs.push_back(MakePairJson(pair));
                }
                if (pair.leashedID == speakerID && pair.holder) {
                    partners.push_back(pair.holder);
                } else if (pair.holderID == speakerID && pair.leashed) {
                    partners.push_back(pair.leashed);
                    if (!pair.tied && !pair.dangling) {
                        held.push_back(pair.leashed);
                    }
                }
            }
            snapshot.visiblePairs = WriteJson(visible, kEmptyPairs);
            snapshot.leashPartners = ActorsPayload(partners, a_table);
            snapshot.speakerOnLeash = !partners.empty();
            snapshot.heldActors = ActorsPayload(held, a_table);
            snapshot.speakerHoldsLeash = !held.empty();
            snapshot.speakerIsLeashed = speakerStatus.leashed;
            snapshot.speakerIsStruggling = LeashState::IsStruggling(a_speaker);
            snapshot.speakerDownOrPulled = LeashState::IsKnockedDown(a_speaker) || LeashState::IsBeingPulled(a_speaker);

            std::vector<RE::Actor*> leashed;
            std::vector<RE::Actor*> unleashed;
            std::vector<RE::Actor*> collared;
            std::vector<RE::Actor*> nearby;
            for (auto* actor : a_nearby) {
                if (actor->GetFormID() == speakerID) {
                    continue;
                }
                const auto& status = a_table.StatusOf(actor);
                if (status.dead) {
                    continue;
                }
                nearby.push_back(actor);
                // A holder who is not leashed belongs in both lists: they can be unleashed
                // from whoever they hold, and they are still a valid leash target.
                if (status.leashedOrHolder) {
                    leashed.push_back(actor);
                }
                if (status.leashed) {
                    collared.push_back(actor);
                }
                if (!status.leashed) {
                    unleashed.push_back(actor);
                }
            }
            snapshot.leashedNearby = !leashed.empty();
            snapshot.unleashedNearby = !unleashed.empty();
            snapshot.collaredNearby = !collared.empty();
            snapshot.leashedActors = ActorsPayload(leashed, a_table);
            snapshot.unleashedActors = ActorsPayload(unleashed, a_table);
            snapshot.collaredActors = ActorsPayload(collared, a_table);
            snapshot.nearbyActors = ActorsPayload(nearby, a_table);

            return snapshot;
        }

        // Refresh makes game calls, so it should run on the thread that loaded the plugin.
        // A queued refresh has been seen on another thread right after game ready, so count
        // every off-thread run and report the count once a minute instead of warning once.
        void NoteRefreshThread() {
            const auto current = GetCurrentThreadId();
            const auto mainThread = g_mainThreadID.load(std::memory_order_relaxed);
            if (!g_loggedRefreshThread.exchange(true, std::memory_order_relaxed)) {
                SKSE::log::info("StateCache: first queued refresh ran on thread {} (plugin load thread {})", current, mainThread);
            }
            if (current != mainThread) {
                g_lastOffMainThread.store(current, std::memory_order_relaxed);
                g_offMainRefreshes.fetch_add(1, std::memory_order_relaxed);
            }
            if (g_offMainRefreshes.load(std::memory_order_relaxed) == 0) {
                return;
            }
            const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
            const auto intervalMs = std::chrono::duration_cast<std::chrono::milliseconds>(kSlowRefreshLogInterval).count();
            auto last = g_lastOffMainLogMs.load(std::memory_order_relaxed);
            if (nowMs - last >= intervalMs && g_lastOffMainLogMs.compare_exchange_strong(last, nowMs, std::memory_order_relaxed)) {
                const auto count = g_offMainRefreshes.exchange(0, std::memory_order_relaxed);
                SKSE::log::warn("StateCache: {} queued refresh(es) ran off the main thread since the last report (last on thread {}, plugin load thread {})", count, g_lastOffMainThread.load(std::memory_order_relaxed), mainThread);
            }
        }

        void NoteRefreshCost(std::chrono::steady_clock::time_point a_started, std::size_t a_actorCount) {
            const auto now = std::chrono::steady_clock::now();
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - a_started);
            if (elapsed <= kSlowRefresh) {
                return;
            }
            const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            const auto intervalMs = std::chrono::duration_cast<std::chrono::milliseconds>(kSlowRefreshLogInterval).count();
            auto last = g_lastSlowLogMs.load(std::memory_order_relaxed);
            if (nowMs - last >= intervalMs && g_lastSlowLogMs.compare_exchange_strong(last, nowMs, std::memory_order_relaxed)) {
                SKSE::log::info("StateCache: refresh took {} ms for {} actors", elapsed.count(), a_actorCount);
            }
        }
    }

    void Refresh() {
        const auto started = std::chrono::steady_clock::now();
        g_struggleEnabled.store(WebUI::Config::StruggleEnabled(), std::memory_order_relaxed);
        LeashState::PruneBeingPulled();

        std::vector<RE::Actor*> collected;
        std::vector<RE::Actor*> recorded;
        LeashState::CollectNearby(collected, recorded);

        std::unordered_set<RE::FormID> pending;
        {
            std::lock_guard lock{g_mutex};
            pending.swap(g_pending);
        }

        std::vector<RE::Actor*> actors;
        actors.reserve(collected.size() + recorded.size() + pending.size());
        std::unordered_set<RE::FormID> seen;
        const auto addUnique = [&](RE::Actor* a_actor) {
            if (a_actor && !a_actor->IsDeleted() && seen.insert(a_actor->GetFormID()).second) {
                actors.push_back(a_actor);
            }
        };
        for (auto* actor : collected) {
            addUnique(actor);
        }
        // Recorded leashes and pending speakers get their own snapshot and count for pairs,
        // but neither is near anyone: they can be in another cell. A pending speaker is only
        // an actor SkyrimNet asked about, and there can be hundreds.
        const std::vector<RE::Actor*> nearby{actors};
        for (auto* actor : recorded) {
            addUnique(actor);
        }
        for (const auto id : pending) {
            addUnique(ActorFromID(id));
        }

        RefreshTable table;
        std::vector<Pair> pairs;
        for (auto* actor : actors) {
            const auto& status = table.StatusOf(actor);
            if (status.dead || !status.leashed) {
                continue;
            }
            auto* holder = LeashState::GetLeashHolder(actor);
            LeashState::RecordedPair recorded;
            const bool hasRecord = LeashState::TryGetRecorded(actor->GetFormID(), recorded);
            Pair pair;
            pair.leashedID = actor->GetFormID();
            pair.leashedName = LeashState::DisplayName(actor);
            pair.leashed = actor;
            pair.holder = holder;
            pair.holderID = holder ? holder->GetFormID() : 0;
            pair.tied = hasRecord ? recorded.tied : holder == nullptr;
            pair.dangling = hasRecord && !recorded.tied && recorded.holderID == 0;
            if (holder && !pair.tied && !pair.dangling) {
                pair.holderName = LeashState::DisplayName(holder);
            }
            pair.kind = SpokenKind(hasRecord ? recorded.kind : "");
            pair.distance = hasRecord ? recorded.distance : "";
            pair.bodyPart = hasRecord ? recorded.bodyPart : "";
            pairs.push_back(std::move(pair));
        }

        const bool pluginLoaded = IsLeashPluginLoaded();
        std::unordered_map<RE::FormID, Snapshot> next;
        next.reserve(actors.size());
        for (auto* speaker : actors) {
            next.insert_or_assign(speaker->GetFormID(), BuildSnapshot(speaker, nearby, pairs, pluginLoaded, table));
        }

        {
            std::lock_guard lock{g_mutex};
            g_snapshots = std::move(next);
        }

        NoteRefreshCost(started, actors.size());
    }

    bool StruggleEnabled() {
        return g_struggleEnabled.load(std::memory_order_relaxed);
    }

    bool Flagged(RE::Actor* a_speaker, Flag a_flag) {
        if (!a_speaker) {
            return false;
        }
        const auto id = a_speaker->GetFormID();

        std::lock_guard lock{g_mutex};
        const auto it = g_snapshots.find(id);
        if (it == g_snapshots.end()) {
            g_pending.insert(id);
            return false;
        }
        switch (a_flag) {
            case Flag::LeashAvailable:
                return it->second.leashAvailable;
            case Flag::UnleashAvailable:
                return it->second.unleashAvailable;
            case Flag::LeashedNearby:
                return it->second.leashedNearby;
            case Flag::UnleashedNearby:
                return it->second.unleashedNearby;
            case Flag::SpeakerOnLeash:
                return it->second.speakerOnLeash;
            case Flag::SpeakerIsLeashed:
                return it->second.speakerIsLeashed;
            case Flag::CollaredNearby:
                return it->second.collaredNearby;
            case Flag::SpeakerIsStruggling:
                return it->second.speakerIsStruggling;
            case Flag::SpeakerHoldsLeash:
                return it->second.speakerHoldsLeash;
            case Flag::SpeakerDownOrPulled:
                return it->second.speakerDownOrPulled;
        }
        return false;
    }

    std::string Json(RE::Actor* a_speaker, Payload a_payload) {
        const auto fallback = a_payload == Payload::VisiblePairs ? kEmptyPairs : kEmptyActors;
        if (!a_speaker) {
            return std::string{fallback};
        }
        const auto id = a_speaker->GetFormID();

        std::lock_guard lock{g_mutex};
        const auto it = g_snapshots.find(id);
        if (it == g_snapshots.end()) {
            g_pending.insert(id);
            return std::string{fallback};
        }
        switch (a_payload) {
            case Payload::VisiblePairs:
                return it->second.visiblePairs;
            case Payload::LeashedActors:
                return it->second.leashedActors;
            case Payload::UnleashedActors:
                return it->second.unleashedActors;
            case Payload::LeashPartners:
                return it->second.leashPartners;
            case Payload::HeldActors:
                return it->second.heldActors;
            case Payload::CollaredActors:
                return it->second.collaredActors;
            case Payload::NearbyActors:
                return it->second.nearbyActors;
        }
        return std::string{fallback};
    }

    void NoteMainThread() {
        g_mainThreadID.store(GetCurrentThreadId(), std::memory_order_relaxed);
    }

    void Start() {
        if (g_thread.joinable()) {
            return;
        }
        // No direct Refresh here: kDataLoaded is not the main thread and no save is loaded.
        // The first tick queues one; until then decorators fall back to g_pending.
        g_thread =std::jthread([](std::stop_token a_stop) {
            while (!a_stop.stop_requested()) {
                {
                    std::unique_lock lock{g_wakeMutex};
                    if (g_wake.wait_for(lock, a_stop, kRefreshInterval, [&a_stop] { return a_stop.stop_requested(); })) {
                        return;
                    }
                }
                auto* tasks = SKSE::GetTaskInterface();
                if (tasks && !g_refreshQueued.exchange(true, std::memory_order_acq_rel)) {
                    tasks->AddTask([] {
                        // Cleared first, so a tick during this refresh can queue the next one.
                        g_refreshQueued.store(false, std::memory_order_release);
                        NoteRefreshThread();
                        Refresh();
                    });
                }
            }
        });
    }

    void Stop() {
        g_thread.request_stop();
        g_wake.notify_all();
        if (g_thread.joinable()) {
            g_thread.join();
        }
    }

    void Reset() {
        // Refreshes must not stop for good if a load boundary ever drops a queued task.
        g_refreshQueued.store(false, std::memory_order_release);
        std::lock_guard lock{g_mutex};
        g_snapshots.clear();
        g_pending.clear();
    }
}
