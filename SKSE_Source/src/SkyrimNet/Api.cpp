#include "Api.h"

#include "SkyrimNet/PublicAPI.h"

namespace SkyrimNetLeashed::SkyrimNet::Api {
    bool Initialize() {
        if (!FindFunctions()) {
            SKSE::log::error("SkyrimNet.dll was not found; decorators will not register");
            return false;
        }
        if (!PublicRegisterDecorator) {
            SKSE::log::warn("SkyrimNet does not export PublicRegisterDecorator (API v5+ required)");
            return false;
        }
        SKSE::log::info("SkyrimNet API v{} resolved", PublicGetVersion ? PublicGetVersion() : 0);
        return true;
    }

    bool RegisterDecorator(const char* a_name, const char* a_description, std::function<std::string(RE::Actor*)> a_callback) {
        if (!PublicRegisterDecorator) {
            return false;
        }
        if (!PublicRegisterDecorator(a_name, a_description, std::move(a_callback))) {
            SKSE::log::error("Failed to register decorator '{}' (name conflict or null callback)", a_name);
            return false;
        }
        SKSE::log::info("Registered decorator '{}'", a_name);
        return true;
    }

    std::uint64_t FormIDToUUID(RE::FormID a_formID) { return PublicFormIDToUUID ? PublicFormIDToUUID(a_formID) : 0; }

    std::string GetPluginConfigValue(const char* a_pluginName, const char* a_path, const char* a_defaultValue) {
        const char* fallback = a_defaultValue ? a_defaultValue : "";
        if (!PublicGetPluginConfigValue) {
            return fallback;
        }
        return PublicGetPluginConfigValue(a_pluginName ? a_pluginName : "", a_path ? a_path : "", fallback);
    }
}
