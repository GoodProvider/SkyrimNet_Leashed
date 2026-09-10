#pragma once

#include "PCH.h"

#include <string>
#include <string_view>
#include <vector>

namespace SkyrimNetLeashed::LeashState {
    struct RecordedPair {
        RE::FormID holderID{};
        std::string kind;
        std::string distance;
        std::string bodyPart;
        bool tied{false};
    };

    [[nodiscard]] bool IsLeashed(RE::Actor* a_actor);
    [[nodiscard]] bool IsLeashHolder(RE::Actor* a_actor);
    [[nodiscard]] std::string DisplayName(RE::Actor* a_actor);

    // Main thread only: both walk RE::ProcessLists.
    [[nodiscard]] RE::Actor* GetLeashHolder(RE::Actor* a_leashed);
    void CollectNearby(std::vector<RE::Actor*>& a_out);

    // Pushed from Papyrus, the only place LeashFramework's real leash table is reachable.
    // a_holderID is 0 for a holderless leash. a_tied true is a world-position anchor;
    // a_tied false is a dangling (unheld) collar. A non-zero holder ignores a_tied.
    void RememberPair(RE::FormID a_holderID, RE::FormID a_leashedID, std::string_view a_kind, std::string_view a_distance, std::string_view a_bodyPart, bool a_tied);
    [[nodiscard]] bool TryGetRecorded(RE::FormID a_leashedID, RecordedPair& a_out);
    void ForgetLeashed(RE::FormID a_leashedID);
    void ClearPairs();

    // Pushed from Papyrus while an actor is in the looping struggle idle.
    void SetStruggling(RE::FormID a_id, bool a_struggling);
    [[nodiscard]] bool IsStruggling(RE::Actor* a_actor);
}
