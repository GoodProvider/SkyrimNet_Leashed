#include "PCH.h"

#include "Leash/LeashConstants.h"
#include "Leash/VisiblePairs.h"
#include "SkyrimNet/Registration.h"

namespace {
    void OnSKSEMessage(SKSE::MessagingInterface::Message* a_msg) {
        if (a_msg->type != SKSE::MessagingInterface::kDataLoaded) {
            return;
        }

        if (!SkyrimNetLeash::IsLeashPluginLoaded()) {
            SKSE::log::error("Leash.esm is not loaded; SkyrimNet_Leash will not register");
            return;
        }
        if (!SkyrimNetLeash::IsLeashFrameworkDllLoaded()) {
            SKSE::log::error("LeashFramework.dll is not loaded; SkyrimNet_Leash will not register");
            return;
        }

        if (!SkyrimNetLeash::SkyrimNet::Register()) {
            return;
        }
        SkyrimNetLeash::VisiblePairs::Start();
        SKSE::log::info("SkyrimNet_Leash registered with SkyrimNet");
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse) {
    SKSE::Init(a_skse);
    spdlog::set_level(spdlog::level::info);

    SKSE::log::info("{} loading", SKSE::PluginDeclaration::GetSingleton()->GetName());

    if (const auto* messaging = SKSE::GetMessagingInterface(); !messaging || !messaging->RegisterListener(OnSKSEMessage)) {
        SKSE::log::error("Failed to register SKSE messaging listener");
        return false;
    }

    SKSE::log::info("{} loaded", SKSE::PluginDeclaration::GetSingleton()->GetName());
    return true;
}
