#include "PCH.h"

#include "Leash/LeashConstants.h"
#include "Leash/LeashState.h"
#include "Papyrus/Bridge.h"
#include "SkyrimNet/Registration.h"
#include "SkyrimNet/StateCache.h"
#include "WebUI/WebUI.h"

namespace {
    void OnDataLoaded() {
        if (!SkyrimNetLeashed::IsLeashPluginLoaded()) {
            SKSE::log::error("Leash.esm is not loaded; SkyrimNet_Leashed will not register");
            return;
        }
        if (!SkyrimNetLeashed::IsLeashFrameworkDllLoaded()) {
            SKSE::log::error("LeashFramework.dll is not loaded; SkyrimNet_Leashed will not register");
            return;
        }

        if (!SkyrimNetLeashed::SkyrimNet::Register()) {
            return;
        }
        SkyrimNetLeashed::SkyrimNet::StateCache::Start();
        SKSE::log::info("SkyrimNet_Leashed registered with SkyrimNet");
    }

    void OnSKSEMessage(SKSE::MessagingInterface::Message* a_msg) {
        switch (a_msg->type) {
            case SKSE::MessagingInterface::kDataLoaded:
                OnDataLoaded();
                SkyrimNetLeashed::WebUI::Init();
                break;
            case SKSE::MessagingInterface::kPreLoadGame:
                // Recorded leashes belong to the outgoing session. Papyrus re-pushes them
                // from the LeashFramework_OnLeash events fired while a save is restored.
                SkyrimNetLeashed::LeashState::ClearPairs();
                SkyrimNetLeashed::SkyrimNet::StateCache::Reset();
                break;
            case SKSE::MessagingInterface::kNewGame:
                SkyrimNetLeashed::LeashState::ClearPairs();
                SkyrimNetLeashed::SkyrimNet::StateCache::Reset();
                SkyrimNetLeashed::WebUI::SetGameReady();
                break;
            case SKSE::MessagingInterface::kPostLoadGame:
                SkyrimNetLeashed::WebUI::SetGameReady();
                break;
            default:
                break;
        }
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

    if (const auto* papyrus = SKSE::GetPapyrusInterface(); !papyrus || !papyrus->Register(SkyrimNetLeashed::Papyrus::Register)) {
        SKSE::log::error("Failed to register Papyrus functions");
        return false;
    }

    SKSE::log::info("{} loaded", SKSE::PluginDeclaration::GetSingleton()->GetName());
    return true;
}
