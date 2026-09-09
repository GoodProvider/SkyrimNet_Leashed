#include "Config.h"

#include "SkyrimNet/Api.h"

#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <ranges>
#include <string>
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

        bool ReadBool(const char* a_path, bool a_default) {
            const auto raw = Lower(GetValue(a_path, a_default ? "true" : "false"));
            if (raw == "true" || raw == "1" || raw == "yes") {
                return true;
            }
            if (raw == "false" || raw == "0" || raw == "no") {
                return false;
            }
            return a_default;
        }

        std::uint32_t ReadHotkey(const char* a_path, std::uint32_t a_default) {
            try {
                const auto value = std::stoul(GetValue(a_path, std::to_string(a_default).c_str()));
                if (value >= 1 && value <= 255) {
                    return static_cast<std::uint32_t>(value);
                }
            } catch (...) {
            }
            return a_default;
        }

        constexpr float kLengthFloor = 50.0F;
        constexpr float kLengthCeil = 2000.0F;

        // Keep in sync with the leash.distance.* defaultValue entries in the manifest.
        constexpr float kTightLength = 80.0F;
        constexpr float kShortLength = 150.0F;
        constexpr float kMiddleLength = 220.0F;
        constexpr float kLongLength = 300.0F;

        float ReadLength(const char* a_path, float a_default) {
            const auto fallback = std::to_string(static_cast<int>(a_default));
            try {
                const auto value = std::stof(GetValue(a_path, fallback.c_str()));
                if (value >= kLengthFloor && value <= kLengthCeil) {
                    return value;
                }
            } catch (...) {
            }
            return a_default;
        }
    }

    bool HotkeyEnabled() {
        return ReadBool("leash.controls.hotkeyEnabled", true);
    }

    std::uint32_t HotkeyVk() {
        return ReadHotkey("leash.controls.hotkey", 220);
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

    float DistanceMax(std::string_view a_token) {
        const auto token = Lower(std::string{a_token});
        if (token == "tight") {
            return ReadLength("leash.distance.tight", kTightLength);
        }
        // Papyrus NormalizeDistance accepts "close" as an LLM synonym for "short".
        if (token == "short" || token == "close") {
            return ReadLength("leash.distance.short", kShortLength);
        }
        if (token == "long") {
            return ReadLength("leash.distance.long", kLongLength);
        }
        return ReadLength("leash.distance.middle", kMiddleLength);
    }

    std::string DistanceFromLength(float a_maxLength) {
        if (a_maxLength <= 0.0F) {
            return "middle";
        }
        struct Candidate {
            const char* token;
            float length;
        };
        const Candidate candidates[] = {
            {"tight", DistanceMax("tight")},
            {"short", DistanceMax("short")},
            {"middle", DistanceMax("middle")},
            {"long", DistanceMax("long")},
        };
        const char* best = "middle";
        float bestDiff = std::numeric_limits<float>::max();
        for (const auto& candidate : candidates) {
            const float diff = std::abs(a_maxLength - candidate.length);
            if (diff < bestDiff) {
                bestDiff = diff;
                best = candidate.token;
            }
        }
        return best;
    }

    std::string LeashType() {
        auto value = Lower(GetValue("leash.ui.leashType", "rope"));
        if (!IsOneOf(value, {"chain", "rope", "magic"})) {
            return "rope";
        }
        return value;
    }

    std::string BodyPart() {
        auto value = Lower(GetValue("leash.ui.bodyPart", "neck"));
        if (!IsOneOf(value, {"neck", "wrists", "waist"})) {
            return "neck";
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
