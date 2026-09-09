#include "LeashState.h"

#include "LeashConstants.h"

#include <limits>
#include <mutex>
#include <unordered_map>

namespace SkyrimNetLeash::LeashState {
    namespace {
        std::mutex g_pairMutex;
        std::unordered_map<RE::FormID, RecordedPair> g_byLeashed;

        RE::Actor* ActorFromID(RE::FormID a_id) {
            auto* form = a_id != 0 ? RE::TESForm::LookupByID(a_id) : nullptr;
            return form ? form->As<RE::Actor>() : nullptr;
        }

        // Fallback for leashes applied before we were listening, where no Papyrus event
        // told us the real holder. Picks the closest holder still within leash range.
        RE::Actor* NearestHolder(RE::Actor* a_leashed) {
            auto* processLists = RE::ProcessLists::GetSingleton();
            if (!processLists) {
                return nullptr;
            }

            RE::Actor* best{};
            float bestDistance = std::numeric_limits<float>::max();
            const auto consider = [&](RE::Actor* a_candidate) {
                if (!a_candidate || a_candidate == a_leashed || !IsLeashHolder(a_candidate)) {
                    return;
                }
                const auto distance = a_leashed->GetDistance(a_candidate);
                if (distance < bestDistance) {
                    bestDistance = distance;
                    best = a_candidate;
                }
            };

            consider(RE::PlayerCharacter::GetSingleton());
            const auto considerHandles = [&](auto& a_handles) {
                for (auto& handle : a_handles) {
                    auto actorPtr = handle.get();
                    consider(actorPtr.get());
                }
            };
            considerHandles(processLists->highActorHandles);
            considerHandles(processLists->middleHighActorHandles);

            return bestDistance <= kHolderSearchRadius ? best : nullptr;
        }
    }

    bool IsLeashed(RE::Actor* a_actor) {
        if (!a_actor) {
            return false;
        }
        auto* faction = GetLeashedFaction();
        if (faction && a_actor->IsInFaction(faction)) {
            return true;
        }
        std::lock_guard lock{g_pairMutex};
        return g_byLeashed.contains(a_actor->GetFormID());
    }

    bool IsLeashHolder(RE::Actor* a_actor) {
        auto* faction = GetLeasherFaction();
        return a_actor && faction && a_actor->IsInFaction(faction);
    }

    std::string DisplayName(RE::Actor* a_actor) {
        if (!a_actor) {
            return "anchor";
        }
        const auto* name = a_actor->GetDisplayFullName();
        return name && name[0] != '\0' ? std::string{name} : "unknown";
    }

    RE::Actor* GetLeashHolder(RE::Actor* a_leashed) {
        if (!a_leashed || !IsLeashed(a_leashed)) {
            return nullptr;
        }

        bool recorded = false;
        RE::FormID holderID = 0;
        {
            std::lock_guard lock{g_pairMutex};
            if (const auto it = g_byLeashed.find(a_leashed->GetFormID()); it != g_byLeashed.end()) {
                recorded = true;
                holderID = it->second.holderID;
            }
        }
        if (recorded) {
            return ActorFromID(holderID);
        }
        return NearestHolder(a_leashed);
    }

    void CollectNearby(std::vector<RE::Actor*>& a_out) {
        if (auto* player = RE::PlayerCharacter::GetSingleton()) {
            a_out.push_back(player);
        }
        auto* lists = RE::ProcessLists::GetSingleton();
        if (!lists) {
            return;
        }
        const auto append = [&](auto& a_handles) {
            for (auto& handle : a_handles) {
                auto ptr = handle.get();
                if (auto* actor = ptr.get()) {
                    a_out.push_back(actor);
                }
            }
        };
        append(lists->highActorHandles);
        append(lists->middleHighActorHandles);
        std::lock_guard lock{g_pairMutex};
        for (const auto& [id, _] : g_byLeashed) {
            if (auto* actor = ActorFromID(id)) {
                a_out.push_back(actor);
            }
        }
    }

    void RememberPair(RE::FormID a_holderID, RE::FormID a_leashedID, std::string_view a_kind, std::string_view a_distance, std::string_view a_bodyPart, bool a_tied) {
        if (a_leashedID == 0) {
            return;
        }
        std::lock_guard lock{g_pairMutex};
        g_byLeashed[a_leashedID] = RecordedPair{
            .holderID = a_holderID,
            .kind = std::string{a_kind},
            .distance = std::string{a_distance},
            .bodyPart = std::string{a_bodyPart},
            .tied = a_holderID == 0 && a_tied,
        };
    }

    bool TryGetRecorded(RE::FormID a_leashedID, RecordedPair& a_out) {
        std::lock_guard lock{g_pairMutex};
        const auto it = g_byLeashed.find(a_leashedID);
        if (it == g_byLeashed.end()) {
            return false;
        }
        a_out = it->second;
        return true;
    }

    void ForgetLeashed(RE::FormID a_leashedID) {
        std::lock_guard lock{g_pairMutex};
        g_byLeashed.erase(a_leashedID);
    }

    void ClearPairs() {
        std::lock_guard lock{g_pairMutex};
        g_byLeashed.clear();
    }
}
