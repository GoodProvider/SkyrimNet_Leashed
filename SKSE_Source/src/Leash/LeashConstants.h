#pragma once

#include "PCH.h"

namespace SkyrimNetLeash {
    constexpr std::string_view kLeashPluginName = "Leash.esm";
    constexpr RE::FormID kLeashedFactionFormID = 0xD6A;
    constexpr RE::FormID kLeasherFactionFormID = 0xD6B;
    constexpr std::string_view kLeashFrameworkDll = "LeashFramework.dll";
    constexpr float kMaxLength = 300.0F;

    [[nodiscard]] RE::TESFaction* GetLeashedFaction();
    [[nodiscard]] RE::TESFaction* GetLeasherFaction();
    [[nodiscard]] bool IsLeashPluginLoaded();
    [[nodiscard]] bool IsLeashFrameworkDllLoaded();
}
