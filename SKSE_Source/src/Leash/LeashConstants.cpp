#include "LeashConstants.h"

#include <Windows.h>

#include <atomic>

namespace SkyrimNetLeashed {
    namespace {
        // LookupForm walks the whole plugin list by name, and these run per actor pair on
        // every cache refresh. A miss is not stored, so a call before kDataLoaded cannot
        // pin null for the session. ESM forms stay valid across save loads.
        RE::TESFaction* CachedFaction(std::atomic<RE::TESFaction*>& a_slot, RE::FormID a_localFormID) {
            if (auto* cached = a_slot.load(std::memory_order_acquire)) {
                return cached;
            }
            auto* data = RE::TESDataHandler::GetSingleton();
            auto* faction = data ? data->LookupForm<RE::TESFaction>(a_localFormID, kLeashPluginName) : nullptr;
            if (faction) {
                a_slot.store(faction, std::memory_order_release);
            }
            return faction;
        }
    }

    RE::TESFaction* GetLeashedFaction() {
        static std::atomic<RE::TESFaction*> slot{nullptr};
        return CachedFaction(slot, kLeashedFactionFormID);
    }

    RE::TESFaction* GetLeasherFaction() {
        static std::atomic<RE::TESFaction*> slot{nullptr};
        return CachedFaction(slot, kLeasherFactionFormID);
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
