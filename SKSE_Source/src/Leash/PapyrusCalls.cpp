#include "PapyrusCalls.h"

#include "LeashConstants.h"

#include <limits>
#include <vector>

namespace SkyrimNetLeash::Papyrus {
    namespace {
        RE::TESFaction* CachedLeashed() { return GetLeashedFaction(); }
        RE::TESFaction* CachedLeasher() { return GetLeasherFaction(); }
    }

    bool IsLeashed(RE::Actor* a_actor) {
        auto* faction = CachedLeashed();
        return a_actor && faction && a_actor->IsInFaction(faction);
    }

    bool IsLeashHolder(RE::Actor* a_actor) {
        auto* faction = CachedLeasher();
        return a_actor && faction && a_actor->IsInFaction(faction);
    }

    RE::Actor* GetLeashHolder(RE::Actor* a_leashed) {
        if (!a_leashed || !IsLeashed(a_leashed)) {
            return nullptr;
        }

        auto* processLists = RE::ProcessLists::GetSingleton();
        if (!processLists) {
            return nullptr;
        }

        const auto consider = [&](RE::Actor* a_candidate) -> RE::Actor* {
            if (!a_candidate || a_candidate == a_leashed || !IsLeashHolder(a_candidate)) {
                return nullptr;
            }
            return a_candidate;
        };

        if (auto* player = RE::PlayerCharacter::GetSingleton()) {
            if (auto* holder = consider(player)) {
                if (a_leashed->GetDistance(holder) <= kMaxLength * 4.0F) {
                    return holder;
                }
            }
        }

        RE::Actor* best{};
        float bestDistance = std::numeric_limits<float>::max();
        const auto considerHandles = [&](auto& handles) {
            for (auto& handle : handles) {
                auto actorPtr = handle.get();
                auto* actor = actorPtr.get();
                if (auto* holder = consider(actor)) {
                    const auto distance = a_leashed->GetDistance(holder);
                    if (distance < bestDistance) {
                        bestDistance = distance;
                        best = holder;
                    }
                }
            }
        };
        considerHandles(processLists->highActorHandles);
        considerHandles(processLists->middleHighActorHandles);
        return bestDistance <= kMaxLength * 4.0F ? best : nullptr;
    }

    std::vector<RE::Actor*> GetLeashedActors(RE::Actor* a_holder) {
        std::vector<RE::Actor*> result;
        if (!a_holder || !IsLeashHolder(a_holder)) {
            return result;
        }

        auto* processLists = RE::ProcessLists::GetSingleton();
        if (!processLists) {
            return result;
        }

        const auto maybeAdd = [&](RE::Actor* a_actor) {
            if (!a_actor || a_actor == a_holder || !IsLeashed(a_actor)) {
                return;
            }
            if (GetLeashHolder(a_actor) == a_holder) {
                result.push_back(a_actor);
            }
        };

        if (auto* player = RE::PlayerCharacter::GetSingleton()) {
            maybeAdd(player);
        }
        for (auto& handle : processLists->highActorHandles) {
            auto actorPtr = handle.get();
            maybeAdd(actorPtr.get());
        }
        for (auto& handle : processLists->middleHighActorHandles) {
            auto actorPtr = handle.get();
            maybeAdd(actorPtr.get());
        }
        return result;
    }
}
