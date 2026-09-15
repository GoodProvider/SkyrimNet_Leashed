#include "Bridge.h"

#include "Leash/LeashState.h"
#include "WebUI/Config.h"
#include "WebUI/WebUI.h"

#include <cctype>
#include <string>
#include <string_view>

namespace SkyrimNetLeashed::Papyrus {
    namespace {
        constexpr std::string_view kScriptName = "SkyrimNet_Leashed_Native";

        void NotifyLeash(RE::StaticFunctionTag*, RE::Actor* a_holder, RE::Actor* a_leashed, RE::BSFixedString a_kind, RE::BSFixedString a_distance, RE::BSFixedString a_bodyPart, bool a_tied) {
            if (!a_leashed) {
                return;
            }
            const char* kind = a_kind.c_str() ? a_kind.c_str() : "";
            const char* distance = a_distance.c_str() ? a_distance.c_str() : "";
            const char* bodyPart = a_bodyPart.c_str() ? a_bodyPart.c_str() : "";
            LeashState::RememberPair(a_holder ? a_holder->GetFormID() : 0, a_leashed->GetFormID(), kind, distance, bodyPart, a_tied);
        }

        void NotifyUnleash(RE::StaticFunctionTag*, RE::Actor* a_leashed) {
            if (!a_leashed) {
                return;
            }
            LeashState::ForgetLeashed(a_leashed->GetFormID());
        }

        void NotifyStruggle(RE::StaticFunctionTag*, RE::Actor* a_who, bool a_struggling) {
            if (!a_who) {
                return;
            }
            LeashState::SetStruggling(a_who->GetFormID(), a_struggling);
        }

        // The LLM supplies style and type tokens as free text, so fold case and treat any
        // run of spaces, tabs, hyphens, or underscores as a single underscore.
        RE::BSFixedString NormalizeToken(RE::StaticFunctionTag*, RE::BSFixedString a_value) {
            const std::string_view raw{a_value.c_str() ? a_value.c_str() : ""};
            std::string result;
            result.reserve(raw.size());
            for (const char ch : raw) {
                if (ch == ' ' || ch == '\t' || ch == '-' || ch == '_') {
                    if (!result.empty() && result.back() != '_') {
                        result += '_';
                    }
                    continue;
                }
                result += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            }
            if (!result.empty() && result.back() == '_') {
                result.pop_back();
            }
            return RE::BSFixedString{result.c_str()};
        }

        [[nodiscard]] bool NameContainsLeash(std::string_view a_name) {
            std::string lower;
            lower.reserve(a_name.size());
            for (const unsigned char ch : a_name) {
                lower.push_back(static_cast<char>(std::tolower(ch)));
            }
            return lower.find("leash") != std::string::npos;
        }

        void WalkLeashNamed(RE::NiAVObject* a_object, std::size_t& a_named) {
            if (!a_object) {
                return;
            }
            const std::string_view name = a_object->name;
            if (NameContainsLeash(name)) {
                const char* parentName = "(none)";
                if (a_object->parent && a_object->parent->name.c_str()) {
                    parentName = a_object->parent->name.c_str();
                }
                SKSE::log::info("TraceLeashBones node='{}' parent='{}'", name, parentName);
                ++a_named;
            }
            auto* node = a_object->AsNode();
            if (!node) {
                return;
            }
            for (const auto& child : node->GetChildren()) {
                if (child) {
                    WalkLeashNamed(child.get(), a_named);
                }
            }
        }

        std::size_t CountLeashDescendants(RE::NiAVObject* a_object) {
            if (!a_object) {
                return 0;
            }
            std::size_t count = NameContainsLeash(a_object->name) ? 1 : 0;
            auto* node = a_object->AsNode();
            if (!node) {
                return count;
            }
            for (const auto& child : node->GetChildren()) {
                if (child) {
                    count += CountLeashDescendants(child.get());
                }
            }
            return count;
        }

        void LogParentLeashCount(RE::NiAVObject* a_root, std::string_view a_parentName) {
            auto* parent = a_root->GetObjectByName(RE::BSFixedString(a_parentName.data()));
            if (!parent) {
                SKSE::log::info("TraceLeashBones parent '{}' not found", a_parentName);
                return;
            }
            auto* node = parent->AsNode();
            if (!node) {
                SKSE::log::info("TraceLeashBones parent '{}' is not a node", a_parentName);
                return;
            }
            std::size_t count = 0;
            for (const auto& child : node->GetChildren()) {
                if (child) {
                    count += CountLeashDescendants(child.get());
                }
            }
            SKSE::log::info("TraceLeashBones parent '{}' leash descendants={}", a_parentName, count);
        }

        float DistanceMin(RE::StaticFunctionTag*, RE::BSFixedString a_leashDistance) {
            const char* token = a_leashDistance.c_str() ? a_leashDistance.c_str() : "";
            return WebUI::Config::DistanceMin(token);
        }

        float DistanceMax(RE::StaticFunctionTag*, RE::BSFixedString a_leashDistance) {
            const char* token = a_leashDistance.c_str() ? a_leashDistance.c_str() : "";
            return WebUI::Config::DistanceMax(token);
        }

        RE::BSFixedString DistanceFromLength(RE::StaticFunctionTag*, float a_maxLength) {
            return RE::BSFixedString{WebUI::Config::DistanceFromLength(a_maxLength).c_str()};
        }

        float StruggleNarrationInterval(RE::StaticFunctionTag*) {
            return WebUI::Config::StruggleNarrationInterval();
        }

        float StruggleCooldown(RE::StaticFunctionTag*) {
            return WebUI::Config::StruggleCooldown();
        }

        bool StruggleEnabled(RE::StaticFunctionTag*) {
            return WebUI::Config::StruggleEnabled();
        }

        void OpenPanel(RE::StaticFunctionTag*) {
            WebUI::Open();
        }

        void TraceLeashBones(RE::StaticFunctionTag*, RE::Actor* a_who) {
            if (!a_who) {
                SKSE::log::warn("TraceLeashBones skipped: null actor");
                return;
            }
            auto* root = a_who->Get3D(false);
            if (!root) {
                SKSE::log::warn("TraceLeashBones {:08X} has no third-person 3D", a_who->GetFormID());
                return;
            }
            SKSE::log::info("TraceLeashBones actor={:08X}", a_who->GetFormID());
            std::size_t named = 0;
            WalkLeashNamed(root, named);
            if (named == 0) {
                SKSE::log::info("TraceLeashBones no nodes containing 'Leash'");
            }
            LogParentLeashCount(root, "NPC Neck [Neck]");
            LogParentLeashCount(root, "NPC Spine1 [Spn1]");
            LogParentLeashCount(root, "NPC Spine2 [Spn2]");
        }

        bool HasLeashBones(RE::StaticFunctionTag*, RE::Actor* a_who) {
            if (!a_who) {
                return false;
            }
            auto* root = a_who->Get3D(false);
            if (!root) {
                return false;
            }
            return CountLeashDescendants(root) > 0;
        }
    }

    bool Register(RE::BSScript::IVirtualMachine* a_vm) {
        if (!a_vm) {
            return false;
        }
        a_vm->RegisterFunction("NotifyLeash", kScriptName, NotifyLeash);
        a_vm->RegisterFunction("NotifyUnleash", kScriptName, NotifyUnleash);
        a_vm->RegisterFunction("NotifyStruggle", kScriptName, NotifyStruggle);
        a_vm->RegisterFunction("NormalizeToken", kScriptName, NormalizeToken);
        a_vm->RegisterFunction("DistanceMin", kScriptName, DistanceMin);
        a_vm->RegisterFunction("DistanceMax", kScriptName, DistanceMax);
        a_vm->RegisterFunction("DistanceFromLength", kScriptName, DistanceFromLength);
        a_vm->RegisterFunction("StruggleNarrationInterval", kScriptName, StruggleNarrationInterval);
        a_vm->RegisterFunction("StruggleCooldown", kScriptName, StruggleCooldown);
        a_vm->RegisterFunction("StruggleEnabled", kScriptName, StruggleEnabled);
        a_vm->RegisterFunction("TraceLeashBones", kScriptName, TraceLeashBones);
        a_vm->RegisterFunction("HasLeashBones", kScriptName, HasLeashBones);
        a_vm->RegisterFunction("OpenPanel", kScriptName, OpenPanel);
        SKSE::log::info("Registered {} Papyrus functions", kScriptName);
        return true;
    }
}
