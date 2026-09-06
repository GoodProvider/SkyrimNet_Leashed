#include "Config.h"

#include "SkyrimNet/Api.h"

#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <ranges>
#include <string_view>
#include <vector>

namespace SkyrimNetLeash::WebUI::Config {
    namespace {
        constexpr const char* kPlugin = "SkyrimNet_Leash";

        std::string GetValue(const char* a_path, const char* a_default) {
            return SkyrimNetLeash::SkyrimNet::Api::GetPluginConfigValue(kPlugin, a_path, a_default);
        }

        std::string Lower(std::string a_value) {
            std::transform(a_value.begin(), a_value.end(), a_value.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
            return a_value;
        }

        bool IsOneOf(std::string_view a_value, const std::vector<std::string_view>& a_allowed) {
            return std::ranges::any_of(a_allowed, [&](std::string_view allowed) { return allowed == a_value; });
        }
    }

    bool HotkeyEnabled() {
        const auto raw = Lower(GetValue("leash.controls.hotkeyEnabled", "true"));
        return raw == "true" || raw == "1" || raw == "yes";
    }

    std::uint32_t HotkeyVk() {
        try {
            const auto value = std::stoul(GetValue("leash.controls.hotkey", "220"));
            if (value >= 1 && value <= 255) {
                return static_cast<std::uint32_t>(value);
            }
        } catch (...) {
        }
        return 220;
    }

    std::uint32_t HotkeyDx() {
        const auto vk = HotkeyVk();
        const auto dx = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
        return dx != 0 ? dx : 0x2B;
    }

    std::string Distance() {
        auto value = Lower(GetValue("leash.ui.distance", "middle"));
        if (!IsOneOf(value, {"tight", "short", "middle", "long"})) {
            return "middle";
        }
        return value;
    }

    std::string TiePoint() {
        auto value = Lower(GetValue("leash.ui.tiePoint", "floor"));
        if (!IsOneOf(value, {"floor", "left", "back", "front", "right", "wall"})) {
            return "floor";
        }
        return value;
    }
}
