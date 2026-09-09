#include "StateCache.h"

#include "Api.h"
#include "Leash/LeashConstants.h"
#include "Leash/LeashState.h"

#include <glaze/glaze.hpp>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stop_token>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace SkyrimNetLeash::SkyrimNet::StateCache {
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

        struct Snapshot {
            bool leashAvailable{false};
            bool unleashAvailable{false};
            bool leashedNearby{false};
            bool unleashedNearby{false};
            bool speakerOnLeash{false};
            bool speakerIsLeashed{false};
            bool collaredNearby{false};
            std::string visiblePairs{kEmptyPairs};
            std::string leashedActors{kEmptyActors};
            std::string unleashedActors{kEmptyActors};
            std::string leashPartners{kEmptyActors};
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

        std::mutex g_wakeMutex;
        std::condition_variable_any g_wake;
        std::jthread g_thread;

        RE::Actor* ActorFromID(RE::FormID a_id) {
            auto* form = a_id != 0 ? RE::TESForm::LookupByID(a_id) : nullptr;
            return form ? form->As<RE::Actor>() : nullptr;
        }

        bool CanSee(RE::Actor* a_speaker, RE::Actor* a_target) {
            if (!a_speaker || !a_target) {
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

        std::string ActorsPayload(const std::vector<RE::Actor*>& a_actors) {
            ActorsJson payload;
            std::vector<std::string> names;
            names.reserve(a_actors.size());
            for (auto* actor : a_actors) {
                if (const auto uuid = Api::FormIDToUUID(actor->GetFormID()); uuid != 0) {
                    payload.actorIds.push_back(uuid);
                }
                names.push_back(LeashState::DisplayName(actor));
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

        Snapshot BuildSnapshot(RE::Actor* a_speaker, const std::vector<RE::Actor*>& a_nearby, const std::vector<Pair>& a_pairs, bool a_pluginLoaded) {
            Snapshot snapshot;
            const auto speakerID = a_speaker->GetFormID();

            snapshot.leashAvailable = a_pluginLoaded && !a_speaker->IsDead() && !a_speaker->IsInCombat();
            snapshot.unleashAvailable = LeashState::IsLeashed(a_speaker) || LeashState::IsLeashHolder(a_speaker);

            PairsJson visible;
            std::vector<RE::Actor*> partners;
            for (const auto& pair : a_pairs) {
                const bool endpoint = pair.holderID == speakerID || pair.leashedID == speakerID;
                if (endpoint || CanSee(a_speaker, pair.holder) || CanSee(a_speaker, pair.leashed)) {
                    visible.pairs.push_back(MakePairJson(pair));
                }
                if (pair.leashedID == speakerID && pair.holder) {
                    partners.push_back(pair.holder);
                } else if (pair.holderID == speakerID && pair.leashed) {
                    partners.push_back(pair.leashed);
                }
            }
            snapshot.visiblePairs = WriteJson(visible, kEmptyPairs);
            snapshot.leashPartners = ActorsPayload(partners);
            snapshot.speakerOnLeash = !partners.empty();
            snapshot.speakerIsLeashed = LeashState::IsLeashed(a_speaker);

            std::vector<RE::Actor*> leashed;
            std::vector<RE::Actor*> unleashed;
            std::vector<RE::Actor*> collared;
            std::vector<RE::Actor*> nearby;
            for (auto* actor : a_nearby) {
                if (actor->GetFormID() == speakerID || actor->IsDead()) {
                    continue;
                }
                nearby.push_back(actor);
                // A holder who is not leashed belongs in both lists: they can be unleashed
                // from whoever they hold, and they are still a valid leash target.
                const bool actorLeashed = LeashState::IsLeashed(actor);
                if (actorLeashed || LeashState::IsLeashHolder(actor)) {
                    leashed.push_back(actor);
                }
                if (actorLeashed) {
                    collared.push_back(actor);
                }
                if (!actorLeashed) {
                    unleashed.push_back(actor);
                }
            }
            snapshot.leashedNearby = !leashed.empty();
            snapshot.unleashedNearby = !unleashed.empty();
            snapshot.collaredNearby = !collared.empty();
            snapshot.leashedActors = ActorsPayload(leashed);
            snapshot.unleashedActors = ActorsPayload(unleashed);
            snapshot.collaredActors = ActorsPayload(collared);
            snapshot.nearbyActors = ActorsPayload(nearby);

            return snapshot;
        }
    }

    void Refresh() {
        std::vector<RE::Actor*> collected;
        LeashState::CollectNearby(collected);

        std::unordered_set<RE::FormID> pending;
        {
            std::lock_guard lock{g_mutex};
            pending.swap(g_pending);
        }
        for (const auto id : pending) {
            if (auto* actor = ActorFromID(id)) {
                collected.push_back(actor);
            }
        }

        std::vector<RE::Actor*> actors;
        actors.reserve(collected.size());
        std::unordered_set<RE::FormID> seen;
        for (auto* actor : collected) {
            if (actor && !actor->IsDeleted() && seen.insert(actor->GetFormID()).second) {
                actors.push_back(actor);
            }
        }

        std::vector<Pair> pairs;
        for (auto* actor : actors) {
            if (actor->IsDead() || !LeashState::IsLeashed(actor)) {
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
            next.insert_or_assign(speaker->GetFormID(), BuildSnapshot(speaker, actors, pairs, pluginLoaded));
        }

        {
            std::lock_guard lock{g_mutex};
            g_snapshots = std::move(next);
        }
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
            case Payload::CollaredActors:
                return it->second.collaredActors;
            case Payload::NearbyActors:
                return it->second.nearbyActors;
        }
        return std::string{fallback};
    }

    void Start() {
        if (g_thread.joinable()) {
            return;
        }
        Refresh();
        g_thread = std::jthread([](std::stop_token a_stop) {
            while (!a_stop.stop_requested()) {
                {
                    std::unique_lock lock{g_wakeMutex};
                    if (g_wake.wait_for(lock, a_stop, kRefreshInterval, [&a_stop] { return a_stop.stop_requested(); })) {
                        return;
                    }
                }
                if (auto* tasks = SKSE::GetTaskInterface()) {
                    tasks->AddTask([] { Refresh(); });
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
        std::lock_guard lock{g_mutex};
        g_snapshots.clear();
        g_pending.clear();
    }
}
