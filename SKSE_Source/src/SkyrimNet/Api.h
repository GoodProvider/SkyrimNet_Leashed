#pragma once

#include "PCH.h"

#include <functional>
#include <string>

// SkyrimNet's PublicAPI.h defines its entry points as extern "C" globals with
// initializers, so only one translation unit may include it. Api.cpp owns that include
// and every other file reaches SkyrimNet through these wrappers.
namespace SkyrimNetLeash::SkyrimNet::Api {
    [[nodiscard]] bool Initialize();
    bool RegisterDecorator(const char* a_name, const char* a_description, std::function<std::string(RE::Actor*)> a_callback);
    [[nodiscard]] std::uint64_t FormIDToUUID(RE::FormID a_formID);
    [[nodiscard]] std::string GetPluginConfigValue(const char* a_pluginName, const char* a_path, const char* a_defaultValue);
}
