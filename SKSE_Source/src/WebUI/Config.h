#pragma once

#include "PCH.h"

#include <string>

namespace SkyrimNetLeash::WebUI::Config {
    [[nodiscard]] bool HotkeyEnabled();
    [[nodiscard]] std::uint32_t HotkeyVk();
    [[nodiscard]] std::uint32_t HotkeyDx();
    [[nodiscard]] std::string Distance();
    [[nodiscard]] std::string TiePoint();
}
