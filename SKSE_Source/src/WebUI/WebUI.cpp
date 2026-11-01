#include "WebUI.h"

#include "Config.h"
#include "Leash/LeashState.h"
#include "MagelightUI_API.h"

#include <glaze/glaze.hpp>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace SkyrimNetLeashed::WebUI {
    struct ActorJson {
        std::uint32_t formId{};
        std::string name{};
        bool isPlayer{false};
        bool isLeashed{false};
        std::uint32_t holderFormId{};
        std::string holderName{};
        std::string kind{};
        std::string distance{};
        std::string bodyPart{};
    };

    struct OpenPayload {
        std::vector<ActorJson> actors{};
        std::uint32_t player{};
        std::uint32_t crosshair{};
        std::string distance{};
        std::string leashType{};
        std::string bodyPart{};
        std::string tiePoint{};
        std::string layout{};
        std::uint32_t presetLeashed{};
        std::string presetVerb{};
        std::uint32_t hotkeyVk{};
    };

    struct StartPayload {
        std::string action{};
        std::uint32_t subject{};
        std::uint32_t leashed{};
        std::uint32_t holder{};
        std::string style{};
        std::string distance{};
        std::string leashType{};
        std::string bodyPart{};
        std::string tiePoint{};
    };

    namespace {
        using namespace MAGELIGHT_API;

        constexpr const char* kModId = "SkyrimNet_Leashed";
        // PostGameTask, the page-to-game marshalling call, arrived in 0.30.1.
        constexpr std::uint32_t kMinHostVersion = PackVersion(0, 30, 1);
        constexpr std::uint32_t kEscapeDx = 0x01;
        constexpr std::uint32_t kQuestLocalFormID = 0x800;
        constexpr const char* kQuestPlugin = "SkyrimNet_Leashed.esp";
        constexpr const char* kActionsScript = "SkyrimNet_Leashed_Actions";
        constexpr float kNearbyRadius = 1024.f;

        struct OpenRequest {
            std::uint32_t presetLeashed{};
            std::string layout{};
            std::string verb{};
        };

        std::mutex g_openMutex;
        OpenRequest g_openRequest;

        void OnMenuHotkey();
        void Hide();

        const MagelightApi4* g_ml = nullptr;
        ModId g_mod = 0;
        ViewId g_view = 0;
        std::atomic<bool> g_gameReady{false};
        // A UI-mode request is out. The view is shown on UIModeEntered, so a visible panel always has the cursor.
        std::atomic<bool> g_openPending{false};

        class KeyHandler : public RE::BSTEventSink<RE::InputEvent*> {
        public:
            static KeyHandler* GetSingleton() {
                static KeyHandler singleton;
                return &singleton;
            }

            static void RegisterSink() {
                auto* inputMgr = RE::BSInputDeviceManager::GetSingleton();
                if (!inputMgr) {
                    SKSE::log::error("WebUI: failed to get InputDeviceManager");
                    return;
                }
                inputMgr->AddEventSink(GetSingleton());
                SKSE::log::info("WebUI: KeyHandler sink registered");
            }

            RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_eventList, [[maybe_unused]] RE::BSTEventSource<RE::InputEvent*>* a_eventSource) override {
                if (!a_eventList) {
                    return RE::BSEventNotifyControl::kContinue;
                }

                bool menuHotkey = false;
                for (auto* event = *a_eventList; event; event = event->next) {
                    if (event->eventType != RE::INPUT_EVENT_TYPE::kButton) {
                        continue;
                    }
                    const auto* btn = event->AsButtonEvent();
                    if (!btn || btn->GetDevice() != RE::INPUT_DEVICE::kKeyboard || !btn->IsDown()) {
                        continue;
                    }
                    const auto code = btn->GetIDCode();
                    if (code != kEscapeDx && Config::HotkeyEnabled() && code == Config::HotkeyDx()) {
                        menuHotkey = true;
                    }
                }
                if (menuHotkey) {
                    OnMenuHotkey();
                }
                return RE::BSEventNotifyControl::kContinue;
            }

        private:
            KeyHandler() = default;
        };

        class DynamicArgs : public RE::BSScript::IFunctionArguments {
        public:
            enum class Kind { Actor, String };

            struct Item {
                Kind kind{Kind::String};
                RE::Actor* actor{nullptr};
                std::string str;
            };

            std::vector<Item> items;

            bool operator()(RE::BSScrapArray<RE::BSScript::Variable>& a_dst) const override {
                a_dst.resize(static_cast<RE::BSTArrayBase::size_type>(items.size()));
                for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(items.size()); ++i) {
                    if (items[i].kind == Kind::Actor) {
                        a_dst[i].Pack(items[i].actor);
                    } else {
                        a_dst[i].Pack(RE::BSFixedString(items[i].str.c_str()));
                    }
                }
                return true;
            }
        };

        bool IsReady() {
            return g_ml && g_ml->IsViewValid(g_view);
        }

        bool IsHidden() {
            if (!IsReady()) {
                return true;
            }
            ViewDesc desc{};
            desc.size = sizeof(ViewDesc);
            if (g_ml->GetViewInfo(g_view, &desc) != Result::Ok) {
                return true;
            }
            return !desc.startVisible;  // GetViewInfo reports current visibility here
        }

        void Hide() {
            if (!IsReady()) {
                return;
            }
            g_openPending = false;
            g_ml->ReleaseUIMode(g_mod);  // also withdraws a queued request; Denied when we hold nothing
            g_ml->ShowView(g_view, false);
        }

        RE::Actor* ActorFromFormID(std::uint32_t a_formId) {
            if (a_formId == 0) {
                return nullptr;
            }
            auto* form = RE::TESForm::LookupByID(a_formId);
            return form ? form->As<RE::Actor>() : nullptr;
        }

        RE::Actor* ActorFromHandle(RE::ObjectRefHandle a_handle) {
            auto ptr = a_handle.get();
            auto* ref = ptr.get();
            return ref ? ref->As<RE::Actor>() : nullptr;
        }

        RE::Actor* CrosshairActor() {
            auto* pick = RE::CrosshairPickData::GetSingleton();
            if (!pick) {
                return nullptr;
            }
#if defined(EXCLUSIVE_SKYRIM_FLAT)
            if (auto* actor = ActorFromHandle(pick->targetActor)) {
                return actor;
            }
            return ActorFromHandle(pick->target);
#else
            if (auto* actor = ActorFromHandle(pick->targetActor[0])) {
                return actor;
            }
            return ActorFromHandle(pick->target[0]);
#endif
        }

        std::string NormalizeToken(std::string a_value, std::string_view a_fallback, std::initializer_list<std::string_view> a_allowed) {
            std::transform(a_value.begin(), a_value.end(), a_value.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
            for (auto allowed : a_allowed) {
                if (allowed == a_value) {
                    return a_value;
                }
            }
            return std::string{a_fallback};
        }

        ActorJson MakeActorJson(RE::Actor* a_actor, RE::Actor* a_player) {
            ActorJson json;
            json.formId = a_actor->GetFormID();
            json.name = LeashState::DisplayName(a_actor);
            json.isPlayer = a_player && a_actor->GetFormID() == a_player->GetFormID();
            json.isLeashed = LeashState::IsLeashed(a_actor);
            if (json.isLeashed) {
                if (auto* holder = LeashState::GetLeashHolder(a_actor)) {
                    json.holderFormId = holder->GetFormID();
                    json.holderName = LeashState::DisplayName(holder);
                }
                LeashState::RecordedPair recorded;
                if (LeashState::TryGetRecorded(a_actor->GetFormID(), recorded)) {
                    json.kind = recorded.kind;
                    json.distance = recorded.distance;
                    json.bodyPart = recorded.bodyPart;
                }
            }
            return json;
        }

        void PushUnique(std::vector<RE::Actor*>& a_out, RE::Actor* a_actor) {
            if (!a_actor || a_actor->IsDeleted()) {
                return;
            }
            const auto id = a_actor->GetFormID();
            for (auto* existing : a_out) {
                if (existing && existing->GetFormID() == id) {
                    return;
                }
            }
            a_out.push_back(a_actor);
        }

        OpenPayload BuildOpenPayload(const OpenRequest& a_request) {
            OpenPayload payload;
            payload.distance = Config::Distance();
            payload.leashType = Config::LeashType();
            payload.bodyPart = Config::BodyPart();
            payload.tiePoint = Config::TiePoint();
            payload.layout = a_request.layout == "vertical" ? "vertical" : "";
            payload.presetLeashed = a_request.presetLeashed;
            payload.presetVerb = a_request.verb;
            payload.hotkeyVk = Config::HotkeyEnabled() ? Config::HotkeyVk() : 0;

            auto* player = RE::PlayerCharacter::GetSingleton();
            if (!player) {
                return payload;
            }
            payload.player = player->GetFormID();
            if (auto* crosshair = CrosshairActor()) {
                payload.crosshair = crosshair->GetFormID();
            }

            std::vector<RE::Actor*> scanned;
            PushUnique(scanned, player);
            PushUnique(scanned, ActorFromFormID(a_request.presetLeashed));

            const float radiusSq = kNearbyRadius * kNearbyRadius;
            const auto playerPos = player->GetPosition();
            if (auto* lists = RE::ProcessLists::GetSingleton()) {
                const auto appendNear = [&](auto& handles) {
                    for (auto& handle : handles) {
                        auto ptr = handle.get();
                        auto* actor = ptr.get();
                        if (!actor || actor == player || actor->IsDeleted()) {
                            continue;
                        }
                        if (actor->GetPosition().GetSquaredDistance(playerPos) <= radiusSq) {
                            PushUnique(scanned, actor);
                        }
                    }
                };
                appendNear(lists->highActorHandles);
                appendNear(lists->middleHighActorHandles);
            }

            if (auto* crosshair = CrosshairActor()) {
                PushUnique(scanned, crosshair);
            }

            payload.actors.reserve(scanned.size());
            for (auto* actor : scanned) {
                payload.actors.push_back(MakeActorJson(actor, player));
            }
            return payload;
        }

        void Show() {
            // Consume the request first so a blocked open can't leak its preset into a later one.
            OpenRequest request;
            {
                std::lock_guard lock(g_openMutex);
                request = std::move(g_openRequest);
                g_openRequest = {};
            }
            if (!g_ml) {
                return;
            }
            if (!g_gameReady.load()) {
                SKSE::log::info("WebUI: blocked — no game loaded");
                return;
            }
            // Magelight opens a view at the main menu on request, and this page needs a world.
            if (auto* ui = RE::UI::GetSingleton(); ui && ui->IsMenuOpen(RE::MainMenu::MENU_NAME)) {
                SKSE::log::info("WebUI: blocked — main menu");
                return;
            }
            if (!IsReady()) {
                SKSE::log::error("WebUI: overlay not ready (view was not created; see earlier log lines)");
                return;
            }

            const auto payload = BuildOpenPayload(request);
            if (payload.presetLeashed) {
                auto* preset = ActorFromFormID(payload.presetLeashed);
                SKSE::log::info("WebUI: preset leashed {} {:08X} layout={} verb={}",
                    preset ? LeashState::DisplayName(preset) : "?",
                    payload.presetLeashed,
                    payload.layout,
                    payload.presetVerb);
            }
            if (payload.crosshair) {
                auto* crosshair = ActorFromFormID(payload.crosshair);
                SKSE::log::info("WebUI: crosshair {} {:08X}", crosshair ? LeashState::DisplayName(crosshair) : "?", payload.crosshair);
            } else {
                SKSE::log::info("WebUI: crosshair none");
            }
            std::string json;
            if (auto err = glz::write_json(payload, json); err) {
                SKSE::log::error("WebUI: failed to serialize nearby actors");
                return;
            }
            g_ml->InvokeJS(g_view, ("openLeashPanel(" + json + ");").c_str());
            // Already open or queued: the call above refreshed the page, and the mode needs no second request.
            if (!IsHidden() || g_openPending.exchange(true)) {
                return;
            }
            if (const auto r = g_ml->RequestUIMode(g_view, kUIModeFlagPause | kUIModeFlagQueue); r != Result::Ok) {
                g_openPending = false;
                SKSE::log::warn("WebUI: RequestUIMode -> {} ({})", static_cast<int>(r), g_ml->GetLastErrorMessage(g_mod));
            }
        }

        void ToggleOrOpen() {
            if (!g_gameReady.load()) {
                SKSE::log::info("WebUI: hotkey blocked — no game loaded");
                return;
            }
            if (g_openPending.load()) {
                return;
            }
            if (!IsHidden()) {
                SKSE::log::info("WebUI: hide overlay");
                Hide();
                return;
            }
            Show();
        }

        void OnMenuHotkey() {
            if (!Config::HotkeyEnabled()) {
                return;
            }
            ToggleOrOpen();
        }

        RE::TESQuest* FindActionsQuest() {
            auto* dh = RE::TESDataHandler::GetSingleton();
            return dh ? dh->LookupForm<RE::TESQuest>(kQuestLocalFormID, kQuestPlugin) : nullptr;
        }

        void DispatchPapyrus(const std::string& a_functionName, std::vector<DynamicArgs::Item> a_items) {
            auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
            if (!vm) {
                SKSE::log::error("WebUI: no Papyrus VM");
                return;
            }
            auto* quest = FindActionsQuest();
            if (!quest) {
                SKSE::log::error("WebUI: quest {} 0x{:X} not found", kQuestPlugin, kQuestLocalFormID);
                return;
            }
            auto handle = vm->GetObjectHandlePolicy()->GetHandleForObject(static_cast<RE::VMTypeID>(quest->GetFormType()), quest);
            RE::BSTSmartPointer<RE::BSScript::Object> scriptObject;
            vm->FindBoundObject(handle, kActionsScript, scriptObject);
            if (!scriptObject) {
                SKSE::log::error("WebUI: bound script '{}' not found", kActionsScript);
                return;
            }

            const auto count = a_items.size();
            auto* raw = new DynamicArgs();
            raw->items = std::move(a_items);
            RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
            vm->DispatchMethodCall(scriptObject, RE::BSFixedString(a_functionName.c_str()), raw, callback);
            SKSE::log::info("WebUI: dispatched {}::{} ({} args)", kActionsScript, a_functionName, count);
        }

        void DispatchStart(StartPayload a_payload) {
            const auto style = NormalizeToken(std::move(a_payload.style), "normally", {"forcefully", "normally", "gently"});
            const auto distance = NormalizeToken(std::move(a_payload.distance), "middle", {"tight", "short", "middle", "long"});
            const auto leashType = NormalizeToken(std::move(a_payload.leashType), "rope", {"chain", "rope", "magic"});
            const auto bodyPart = NormalizeToken(std::move(a_payload.bodyPart), "neck", {"neck", "wrists", "waist"});
            const auto tiePoint = NormalizeToken(std::move(a_payload.tiePoint), "floor", {"floor", "left", "back", "front", "right", "wall"});
            auto action = a_payload.action;
            std::transform(action.begin(), action.end(), action.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });

            auto* subject = ActorFromFormID(a_payload.subject);
            auto* leashed = ActorFromFormID(a_payload.leashed);
            auto* holder = ActorFromFormID(a_payload.holder);
            if (!subject || !leashed) {
                SKSE::log::warn("WebUI: Start missing subject or leashed");
                return;
            }

            const bool isLeashed = LeashState::IsLeashed(leashed);
            std::string functionName;
            std::vector<DynamicArgs::Item> items;

            const auto addActor = [&](RE::Actor* actor) {
                items.push_back(DynamicArgs::Item{.kind = DynamicArgs::Kind::Actor, .actor = actor});
            };
            const auto addString = [&](std::string value) {
                items.push_back(DynamicArgs::Item{.kind = DynamicArgs::Kind::String, .str = std::move(value)});
            };

            if (action == "unleash") {
                if (subject->GetFormID() == leashed->GetFormID()) {
                    functionName = "UnleashSpeakerExecute";
                    addActor(subject);
                } else {
                    functionName = "UnleashTargetExecute";
                    addActor(subject);
                    addActor(leashed);
                }
            } else if (action == "leash to" || action == "tie to") {
                functionName = "LeashedToTiePoint";
                addActor(subject);
                addActor(leashed);
                addString(style);
                // Re-tying an already-leashed actor keeps its current mesh and length:
                // LeashedToTiePoint ignores these three and re-detects them.
                addString(distance);
                addString(leashType);
                addString(bodyPart);
                addString(tiePoint);
            } else if (action == "give to" && isLeashed) {
                if (holder && holder->GetFormID() == subject->GetFormID()) {
                    functionName = "TakeLeash";
                    addActor(subject);
                    addActor(leashed);
                } else {
                    functionName = "GiveLeash";
                    addActor(subject);
                    addActor(leashed);
                    addActor(holder);
                }
            } else {
                functionName = "LeashedToHolder";
                addActor(subject);
                addActor(leashed);
                addActor(holder);
                addString(style);
                addString(distance);
                addString(leashType);
                addString(bodyPart);
            }

            DispatchPapyrus(functionName, std::move(items));
        }

        void HandleStart(const char* a_value) {
            StartPayload payload;
            if (auto err = glz::read_json(payload, a_value ? std::string_view{a_value} : std::string_view{}); err) {
                SKSE::log::warn("WebUI: onStart parse failed");
                return;
            }
            Hide();
            DispatchStart(std::move(payload));
        }

        void HandleCancel(const char*) {
            Hide();
        }

        using PageCallBody = void (*)(const char*);

        // Render or page thread: the argument is valid for this call only and RE:: is off limits,
        // so copy it and run the body on the game thread.
        void OnPageCall(ViewId, const char* a_argument, void* a_user) {
            if (!g_ml) {
                return;
            }
            auto* call = new std::pair<PageCallBody, std::string>(reinterpret_cast<PageCallBody>(a_user), a_argument ? a_argument : "");
            const auto result = g_ml->PostGameTask(
                [](void* a_call) {
                    const std::unique_ptr<std::pair<PageCallBody, std::string>> owned(static_cast<std::pair<PageCallBody, std::string>*>(a_call));
                    owned->first(owned->second.c_str());
                },
                call);
            if (result != Result::Ok) {
                delete call;
                SKSE::log::warn("WebUI: PostGameTask -> {}", static_cast<int>(result));
            }
        }

        // Game thread (ModDesc::callbackThread).
        void OnHostEvent(const EventData* a_event, void*) {
            if (!a_event || a_event->view != g_view) {
                return;
            }
            switch (a_event->type) {
                case Event::UIModeEntered:
                    if (g_openPending.exchange(false)) {
                        g_ml->ShowView(g_view, true);
                    } else if (IsHidden()) {
                        // An entry that outlived its open: hand the mode back, or the player is left with no controls.
                        g_ml->ReleaseUIMode(g_mod);
                    }
                    break;
                case Event::UIModeExited:
                    // Every exit path, and the host does not hide the view: PageUp, a load boundary, our own Hide.
                    g_openPending = false;
                    g_ml->ShowView(g_view, false);
                    break;
                case Event::UIModeRefused:
                    g_openPending = false;
                    SKSE::log::warn("WebUI: UI mode refused: {}", a_event->detail ? a_event->detail : "");
                    break;
                case Event::ViewLoadFailed:
                    SKSE::log::error("WebUI: page load failed: {}", a_event->detail ? a_event->detail : "");
                    break;
                default:
                    break;
            }
        }

        std::string PagePath() {
            wchar_t exe[MAX_PATH]{};
            GetModuleFileNameW(nullptr, exe, MAX_PATH);
            const auto path = std::filesystem::path(exe).parent_path() / L"Data" / L"Magelight" / L"SkyrimNet_Leashed" / L"views" / L"panel" / L"index.html";
            const auto utf8 = path.u8string();
            return std::string(reinterpret_cast<const char*>(utf8.data()), utf8.size());
        }
    }

    void Init() {
        const auto* api = RequestApi4();
        if (!api) {
            SKSE::log::warn("WebUI: Magelight UI API not available; leash panel hotkey disabled");
            return;
        }

        ModDesc modDesc{};
        modDesc.size = sizeof(ModDesc);
        modDesc.modId = kModId;
        modDesc.displayName = "SkyrimNet Leashed";
        modDesc.minHostVersion = kMinHostVersion;
        modDesc.callbackThread = CallbackThread::GameThread;
        modDesc.onEvent = OnHostEvent;
        if (const auto r = api->RegisterMod(&modDesc, &g_mod); r != Result::Ok || !g_mod) {
            SKSE::log::warn("WebUI: Magelight RegisterMod -> {} ({}); leash panel hotkey disabled", static_cast<int>(r), api->GetLastErrorMessage(0));
            return;
        }
        g_ml = api;
        SKSE::log::info("WebUI: Magelight host {} acquired", api->hostVersion);

        const auto page = PagePath();
        ViewDesc viewDesc{};
        viewDesc.size = sizeof(ViewDesc);
        viewDesc.name = "panel";
        viewDesc.htmlPath = page.c_str();
        viewDesc.fullscreen = true;
        viewDesc.startVisible = false;
        viewDesc.layer = Layer::Panel;
        if (const auto r = g_ml->CreateViewEx(g_mod, &viewDesc, &g_view); r != Result::Ok || !g_view) {
            SKSE::log::error("WebUI: CreateViewEx -> {} ({}), path {}", static_cast<int>(r), g_ml->GetLastErrorMessage(g_mod), page);
            return;
        }
        g_ml->SetViewOrder(g_view, 10);

        g_ml->RegisterJSListenerEx(g_view, "onStart", OnPageCall, reinterpret_cast<void*>(&HandleStart));
        g_ml->RegisterJSListenerEx(g_view, "onCancel", OnPageCall, reinterpret_cast<void*>(&HandleCancel));

        KeyHandler::RegisterSink();
        SKSE::log::info("WebUI: menu hotkey follows leash.controls.hotkey");
    }

    void SetGameReady() {
        g_gameReady = true;
        g_openPending = false;  // the host resets UI mode on every load
        SKSE::log::info("WebUI: game ready");
    }

    void Open() {
        {
            std::lock_guard lock(g_openMutex);
            g_openRequest = {};
        }
        Show();
    }

    void OpenFor(RE::Actor* a_leashed, std::string_view a_layout, std::string_view a_verb) {
        {
            std::lock_guard lock(g_openMutex);
            g_openRequest.presetLeashed = a_leashed ? a_leashed->GetFormID() : 0;
            g_openRequest.layout = a_layout == "vertical" ? "vertical" : "";
            g_openRequest.verb = std::string{a_verb};
        }
        if (auto* tasks = SKSE::GetTaskInterface()) {
            tasks->AddTask([]() { Show(); });
        } else {
            Show();
        }
    }
}
