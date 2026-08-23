#pragma once

#include "PCH.h"

#include <vector>

namespace SkyrimNetLeash::Papyrus {
    [[nodiscard]] RE::Actor* GetLeashHolder(RE::Actor* a_leashed);
    [[nodiscard]] std::vector<RE::Actor*> GetLeashedActors(RE::Actor* a_holder);
    [[nodiscard]] bool IsLeashed(RE::Actor* a_actor);
    [[nodiscard]] bool IsLeashHolder(RE::Actor* a_actor);
}
