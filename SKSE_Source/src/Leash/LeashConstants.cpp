#include "LeashConstants.h"

#include <Windows.h>

namespace SkyrimNetLeash {
    RE::TESFaction* GetLeashedFaction() {
        auto* data = RE::TESDataHandler::GetSingleton();
        return data ? data->LookupForm<RE::TESFaction>(kLeashedFactionFormID, kLeashPluginName) : nullptr;
    }

    RE::TESFaction* GetLeasherFaction() {
        auto* data = RE::TESDataHandler::GetSingleton();
        return data ? data->LookupForm<RE::TESFaction>(kLeasherFactionFormID, kLeashPluginName) : nullptr;
    }

    bool IsLeashPluginLoaded() {
        auto* data = RE::TESDataHandler::GetSingleton();
        if (!data) {
            return false;
        }
        return data->LookupLoadedModByName(kLeashPluginName) ||
               data->LookupLoadedLightModByName(kLeashPluginName);
    }

    bool IsLeashFrameworkDllLoaded() { return GetModuleHandleA(kLeashFrameworkDll.data()) != nullptr; }
}
