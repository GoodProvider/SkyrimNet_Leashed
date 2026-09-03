#pragma once

#include "PCH.h"

#include <string>
#include <string_view>
#include <vector>

namespace SkyrimNetLeash::LeashState {
    struct RecordedPair {
        RE::FormID holderID{};
        std::string kind;
        std::string distance;
        std::string bodyPart;
    };

    [[nodiscard]] bool IsLeashed(RE::Actor* a_actor);
    [[nodiscard]] bool IsLeashHolder(RE::Actor* a_actor);
    [[nodiscard]] std::string DisplayName(RE::Actor* a_actor);

    // Main thread only: both walk RE::ProcessLists.
    [[nodiscard]] RE::Actor* GetLeashHolder(RE::Actor* a_leashed);
    void CollectNearby(std::vector<RE::Actor*>& a_out);

    // Pushed from Papyrus, the only place LeashFramework's real leash table is reachable.
    // a_holderID is 0 for a holderless world-position leash, which is distinct from a
    // leashed actor we have no record for at all.
    void RememberPair(RE::FormID a_holderID, RE::FormID a_leashedID, std::string_view a_kind, std::string_view a_distance, std::string_view a_bodyPart);
    [[nodiscard]] bool TryGetRecorded(RE::FormID a_leashedID, RecordedPair& a_out);
    void ForgetLeashed(RE::FormID a_leashedID);
    void ClearPairs();
}
