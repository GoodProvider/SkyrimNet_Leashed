#pragma once

#include "PCH.h"

#include <string>
#include <string_view>

namespace SkyrimNetLeashed::WebUI::Config {
    [[nodiscard]] bool HotkeyEnabled();
    [[nodiscard]] std::uint32_t HotkeyVk();
    [[nodiscard]] std::uint32_t HotkeyDx();
    [[nodiscard]] std::string Distance();
    [[nodiscard]] float DistanceMax(std::string_view a_token);
    [[nodiscard]] std::string DistanceFromLength(float a_maxLength);
    [[nodiscard]] std::string LeashType();
    [[nodiscard]] std::string BodyPart();
    [[nodiscard]] std::string TiePoint();
    [[nodiscard]] bool StruggleEnabled();
    [[nodiscard]] float StruggleNarrationInterval();
    [[nodiscard]] float StruggleCooldown();
}
