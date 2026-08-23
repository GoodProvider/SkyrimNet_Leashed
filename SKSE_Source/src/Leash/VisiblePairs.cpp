#include "VisiblePairs.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include <glaze/glaze.hpp>

#include "PapyrusCalls.h"

namespace SkyrimNetLeash::VisiblePairs {
    struct PairJson {
        std::string holder{};
        std::string leashed{};
    };

    struct PairsJson {
        std::vector<PairJson> pairs{};
    };

    namespace {
        struct PairRecord {
            std::uint32_t holderID{};
            std::uint32_t leashedID{};
            std::string holderName;
            std::string leashedName;
            RE::Actor* holder{};
            RE::Actor* leashed{};
        };

        std::mutex g_mutex;
        std::unordered_map<std::uint32_t, std::string> g_jsonBySpeaker;
        std::atomic<bool> g_running{false};

        std::string ActorName(RE::Actor* a_actor) {
            if (!a_actor) {
                return "anchor";
            }
            const auto* name = a_actor->GetDisplayFullName();
            return name && name[0] != '\0' ? std::string{name} : "unknown";
        }

        void CollectActors(std::vector<RE::Actor*>& a_out) {
            if (auto* player = RE::PlayerCharacter::GetSingleton()) {
                a_out.push_back(player);
            }
            auto* lists = RE::ProcessLists::GetSingleton();
            if (!lists) {
                return;
            }
            const auto append = [&](auto& handles) {
                for (auto& handle : handles) {
                    auto ptr = handle.get();
                    if (auto* actor = ptr.get()) {
                        a_out.push_back(actor);
                    }
                }
            };
            append(lists->highActorHandles);
            append(lists->middleHighActorHandles);
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

        std::string WritePairs(const PairsJson& a_payload) {
            std::string json;
            if (auto err = glz::write_json(a_payload, json); err) {
                return "{\"pairs\":[]}";
            }
            return json;
        }
    }

    void Refresh() {
        std::vector<RE::Actor*> actors;
        CollectActors(actors);

        std::vector<PairRecord> pairs;
        for (auto* actor : actors) {
            if (!Papyrus::IsLeashed(actor)) {
                continue;
            }
            auto* holder = Papyrus::GetLeashHolder(actor);
            PairRecord record;
            record.leashedID = actor->GetFormID();
            record.leashedName = ActorName(actor);
            record.leashed = actor;
            if (holder) {
                record.holderID = holder->GetFormID();
                record.holderName = ActorName(holder);
                record.holder = holder;
            } else {
                record.holderName = "anchor";
            }
            pairs.push_back(std::move(record));
        }

        std::unordered_map<std::uint32_t, std::string> next;
        for (auto* speaker : actors) {
            PairsJson payload;
            for (const auto& pair : pairs) {
                const auto speakerID = speaker->GetFormID();
                const bool endpoint = pair.holderID == speakerID || pair.leashedID == speakerID;
                if (endpoint || CanSee(speaker, pair.holder) || CanSee(speaker, pair.leashed)) {
                    payload.pairs.push_back(PairJson{.holder = pair.holderName, .leashed = pair.leashedName});
                }
            }
            next.emplace(speaker->GetFormID(), WritePairs(payload));
        }

        std::lock_guard lock{g_mutex};
        g_jsonBySpeaker = std::move(next);
    }

    std::string JsonForSpeaker(RE::Actor* a_speaker) {
        if (!a_speaker) {
            return "{\"pairs\":[]}";
        }
        std::lock_guard lock{g_mutex};
        const auto it = g_jsonBySpeaker.find(a_speaker->GetFormID());
        return it != g_jsonBySpeaker.end() ? it->second : std::string{"{\"pairs\":[]}"};
    }

    void Start() {
        if (g_running.exchange(true)) {
            return;
        }
        Refresh();
        std::thread([] {
            while (g_running) {
                std::this_thread::sleep_for(std::chrono::milliseconds(750));
                if (auto* tasks = SKSE::GetTaskInterface()) {
                    tasks->AddTask([] { Refresh(); });
                }
            }
        }).detach();
    }
}
