#pragma once

#include "PCH.h"

#include <string>
#include <string_view>

namespace SkyrimNetLeashed::WebUI::Config {
    [[nodiscard]] bool HotkeyEnabled();
    [[nodiscard]] std::uint32_t HotkeyVk();
    [[nodiscard]] std::uint32_t HotkeyDx();
    // VR controller chord for the panel (Magelight kVRButton* names): "none", "a", "stick", "trigger", "touchpad", "grip".
    [[nodiscard]] std::string VrButton();
    [[nodiscard]] std::string VrModifier();
    // "either", "left", "right".
    [[nodiscard]] std::string VrHand();
    [[nodiscard]] std::string Distance();
    [[nodiscard]] float DistanceMin(std::string_view a_token);
    [[nodiscard]] float DistanceMax(std::string_view a_token);
    [[nodiscard]] std::string DistanceFromLength(float a_maxLength);
    [[nodiscard]] std::string LeashType();
    [[nodiscard]] std::string BodyPart();
    [[nodiscard]] std::string TiePoint();
    [[nodiscard]] bool StruggleEnabled();
    [[nodiscard]] float StruggleNarrationInterval();
    [[nodiscard]] float StruggleCooldown();
    // Seconds before a struggle always ends on its own.
    [[nodiscard]] float StruggleMaxDuration();
    // Seconds between short-lived "being dragged" events; 0 = off.
    [[nodiscard]] float DragEventInterval();
    // Leash falls and drags add Injury Engine injuries (when Injury Engine is installed).
    [[nodiscard]] bool FallDamageEnabled();
}
