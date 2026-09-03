#include "Bridge.h"

#include "Leash/LeashState.h"

#include <cctype>
#include <string>
#include <string_view>

namespace SkyrimNetLeash::Papyrus {
    namespace {
        constexpr std::string_view kScriptName = "SkyrimNet_Leash_Native";

        void NotifyLeash(RE::StaticFunctionTag*, RE::Actor* a_holder, RE::Actor* a_leashed, RE::BSFixedString a_kind, RE::BSFixedString a_distance, RE::BSFixedString a_bodyPart) {
            if (!a_leashed) {
                return;
            }
            const char* kind = a_kind.c_str() ? a_kind.c_str() : "";
            const char* distance = a_distance.c_str() ? a_distance.c_str() : "";
            const char* bodyPart = a_bodyPart.c_str() ? a_bodyPart.c_str() : "";
            LeashState::RememberPair(a_holder ? a_holder->GetFormID() : 0, a_leashed->GetFormID(), kind, distance, bodyPart);
        }

        void NotifyUnleash(RE::StaticFunctionTag*, RE::Actor* a_leashed) {
            if (!a_leashed) {
                return;
            }
            LeashState::ForgetLeashed(a_leashed->GetFormID());
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
    }

    bool Register(RE::BSScript::IVirtualMachine* a_vm) {
        if (!a_vm) {
            return false;
        }
        a_vm->RegisterFunction("NotifyLeash", kScriptName, NotifyLeash);
        a_vm->RegisterFunction("NotifyUnleash", kScriptName, NotifyUnleash);
        a_vm->RegisterFunction("NormalizeToken", kScriptName, NormalizeToken);
        SKSE::log::info("Registered {} Papyrus functions", kScriptName);
        return true;
    }
}
