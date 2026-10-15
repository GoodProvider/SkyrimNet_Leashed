#include "InjuryEngine.h"

#include "WebUI/Config.h"

#include <string>
#include <utility>

namespace SkyrimNetLeashed::InjuryEngine {
    namespace {
        constexpr const char* kPlugin = "Injury_Engine.esp";
        constexpr const char* kScript = "Injury_Engine_Main";
        constexpr const char* kInjure = "External_Damage";

        bool HasGlobal(const char* a_script, std::string_view a_fn) {
            auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
            RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo> type;
            if (!vm || !vm->GetScriptObjectType(RE::BSFixedString(a_script), type) || !type) {
                return false;
            }
            const auto* funcs = type->GetGlobalFuncIter();
            for (std::uint32_t i = 0; funcs && i < type->GetNumGlobalFuncs(); ++i) {
                if (funcs[i].func && std::string_view(funcs[i].func->GetName().c_str()) == a_fn) {
                    return true;
                }
            }
            return false;
        }

        template <class... Args>
        bool Call(const char* a_fn, Args... a_args) {
            auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
            if (!vm) {
                return false;
            }
            auto* args = RE::MakeFunctionArguments(std::move(a_args)...);
            RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
            if (!vm->DispatchStaticCall(kScript, a_fn, args, callback)) {
                SKSE::log::warn("InjuryEngine: {}.{} dispatch failed", kScript, a_fn);
                return false;
            }
            return true;
        }
    }

    bool Installed() {
        // The plugin can't change during a session; the script check stays cheap after the first answer.
        static int cached = -1;
        if (cached < 0) {
            auto* dh = RE::TESDataHandler::GetSingleton();
            if (!dh) {
                return false;
            }
            const bool plugin = dh->LookupModByName(kPlugin) != nullptr;
            const bool api = plugin && HasGlobal(kScript, kInjure);
            if (plugin && !api) {
                SKSE::log::warn("InjuryEngine: {} is loaded but has no {} (update Injury Engine)", kPlugin, kInjure);
            }
            cached = api ? 1 : 0;
            SKSE::log::info("InjuryEngine: installed={}", api);
        }
        return cached == 1;
    }

    bool FallDamage() {
        return WebUI::Config::FallDamageEnabled() && Installed();
    }

    bool Injure(RE::Actor* a_victim, std::string_view a_type, int a_count, RE::Actor* a_attacker, std::string_view a_cause,
        bool a_narrate) {
        if (!a_victim || a_count < 1 || !FallDamage()) {
            return false;
        }
        const std::string type(a_type);
        const std::string cause(a_cause);
        SKSE::log::info("InjuryEngine: {} x{} on {:08X} narrate={}", type, a_count, a_victim->GetFormID(), a_narrate);
        // External_Damage(attacker, victim, cause, body_part, damage_type, damage_amount, narrate)
        return Call(kInjure, std::move(a_attacker), std::move(a_victim), RE::BSFixedString(cause.c_str()),
            RE::BSFixedString(""), RE::BSFixedString(type.c_str()), static_cast<std::int32_t>(a_count), a_narrate);
    }
}
