#pragma once

#include "PCH.h"

#include <string_view>

// Soft dependency on SkyrimNet_Whipped: its Papyrus API is dispatched through the VM by name, so nothing here
// (or in the Papyrus scripts) links against Whipped. Injury types: Whipped's own "falling" and our
// SKSE/Plugins/SkyrimNet_Whipped/injuries/leash_dragging.json (merged into Whipped's folder by the mod manager).
namespace SkyrimNetLeashed::Whipped {
    // SkyrimNet_Whipped.esp is loaded and SkyrimNet_Whipped_Main has External_Damage (checked once).
    [[nodiscard]] bool Installed();

    // Installed() and the "Falling hurts" setting is on.
    [[nodiscard]] bool FallDamage();

    // count hits of injury type `type` on `victim` (External_Damage: always applied). narrate: Whipped's torture
    // narration rules, with `cause` leading the text ("Bob yanks Nina off her feet"); false is silent. False when
    // not dispatched.
    bool Injure(RE::Actor* victim, std::string_view type, int count, RE::Actor* attacker = nullptr,
        std::string_view cause = {}, bool narrate = true);
}
