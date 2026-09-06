#include "WebUI.h"

#include "Config.h"
#include "Leash/LeashState.h"
#include "PrismaUI_API.h"

#include <glaze/glaze.hpp>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <functional>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <vector>

namespace SkyrimNetLeash::WebUI {
    struct ActorJson {
        std::uint32_t formId{};
        std::string name{};
        bool isPlayer{false};
        bool isLeashed{false};
        std::uint32_t holderFormId{};
        std::string holderName{};
    };

    struct OpenPayload {
        std::vector<ActorJson> actors{};
        std::uint32_t player{};
        std::uint32_t crosshair{};
        std::string distance{};
        std::string leashType{};
        std::string tiePoint{};
    };

    struct StartPayload {
        std::string action{};
        std::uint32_t subject{};
        std::uint32_t leashed{};
        std::uint32_t holder{};
        std::string style{};
        std::string distance{};
        std::string leashType{};
        std::string tiePoint{};
    };

    namespace {
        constexpr std::uint32_t kEscapeDx = 0x01;
        constexpr std::uint32_t kQuestLocalFormID = 0x800;
        constexpr const char* kQuestPlugin = "SkyrimNet_Leash.esp";
        constexpr const char* kActionsScript = "SkyrimNet_Leash_Actions";
        constexpr float kNearbyRadius = 1024.f;

        void OnMenuHotkey();
        void OnEscape();
        void Hide();

        PRISMA_UI_API::IVPrismaUI1* g_prismaUI = nullptr;
        PrismaView g_view = 0;
        std::atomic<bool> g_gameReady{false};
        std::atomic<bool> g_domReady{false};

        using KeyCallback = std::function<void()>;

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

            void Register(std::uint32_t a_dxScanCode, KeyCallback a_callback) {
                std::unique_lock lock{_mutex};
                _callbacks[a_dxScanCode] = std::move(a_callback);
            }

            RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_eventList, [[maybe_unused]] RE::BSTEventSource<RE::InputEvent*>* a_eventSource) override {
                if (!a_eventList) {
                    return RE::BSEventNotifyControl::kContinue;
                }

                std::vector<KeyCallback> toRun;
                bool menuHotkey = false;
                {
                    std::shared_lock lock{_mutex};
                    for (auto* event = *a_eventList; event; event = event->next) {
                        if (event->eventType != RE::INPUT_EVENT_TYPE::kButton) {
                            continue;
                        }
                        const auto* btn = event->AsButtonEvent();
                        if (!btn || btn->GetDevice() != RE::INPUT_DEVICE::kKeyboard || !btn->IsDown()) {
                            continue;
                        }
                        const auto code = btn->GetIDCode();
                        auto it = _callbacks.find(code);
                        if (it != _callbacks.end()) {
                            toRun.push_back(it->second);
                        }
                        if (code != kEscapeDx && Config::HotkeyEnabled() && code == Config::HotkeyDx()) {
                            menuHotkey = true;
                        }
                    }
                }
                for (const auto& cb : toRun) {
                    cb();
                }
                if (menuHotkey) {
                    OnMenuHotkey();
                }
                return RE::BSEventNotifyControl::kContinue;
            }

        private:
            KeyHandler() = default;
            std::map<std::uint32_t, KeyCallback> _callbacks;
            std::shared_mutex _mutex;
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
            return g_prismaUI && g_domReady.load() && g_prismaUI->IsValid(g_view);
        }

        bool IsHidden() {
            if (!g_prismaUI || !g_prismaUI->IsValid(g_view)) {
                return true;
            }
            return g_prismaUI->IsHidden(g_view);
        }

        void Invoke(const std::string& a_script) {
            if (!g_prismaUI || !g_domReady.load()) {
                return;
            }
            g_prismaUI->Invoke(g_view, a_script.c_str());
        }

        void Hide() {
            if (!g_prismaUI || !g_prismaUI->IsValid(g_view)) {
                return;
            }
            g_prismaUI->Unfocus(g_view);
            g_prismaUI->Hide(g_view);
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

        OpenPayload BuildOpenPayload() {
            OpenPayload payload;
            payload.distance = Config::Distance();
            payload.leashType = Config::LeashType();
            payload.tiePoint = Config::TiePoint();

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
            if (!g_prismaUI) {
                return;
            }
            if (!g_gameReady.load()) {
                SKSE::log::info("WebUI: blocked — no game loaded");
                return;
            }
            if (!IsReady()) {
                SKSE::log::error("WebUI: overlay not ready (missing PrismaUI/views/SkyrimNet_Leash/index.html?)");
                return;
            }

            const auto payload = BuildOpenPayload();
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
            Invoke("openLeashPanel(" + json + ");");
            g_prismaUI->Show(g_view);
            g_prismaUI->Focus(g_view, true);
        }

        void ToggleOrOpen() {
            if (!g_gameReady.load()) {
                SKSE::log::info("WebUI: hotkey blocked — no game loaded");
                return;
            }
            if (!IsHidden()) {
                SKSE::log::info("WebUI: hide overlay");
                Hide();
                return;
            }
            Show();
        }

        void OnEscape() {
            if (IsHidden()) {
                return;
            }
            SKSE::log::info("WebUI: Escape hide overlay");
            Hide();
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

        void DispatchStart(StartPayload a_payload) {
            const auto style = NormalizeToken(std::move(a_payload.style), "normally", {"forcefully", "normally", "gently"});
            const auto distance = NormalizeToken(std::move(a_payload.distance), "middle", {"tight", "short", "middle", "long"});
            const auto leashType = NormalizeToken(std::move(a_payload.leashType), "rope", {"chain", "rope", "magic"});
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
                addString(distance);
                addString(leashType);
                addString("neck");
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
                addString("neck");
            }

            SKSE::GetTaskInterface()->AddTask([functionName, items]() {
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

                auto* raw = new DynamicArgs();
                raw->items = items;
                RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
                vm->DispatchMethodCall(scriptObject, RE::BSFixedString(functionName.c_str()), raw, callback);
                SKSE::log::info("WebUI: dispatched {}::{} ({} args)", kActionsScript, functionName, items.size());
            });
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
    }

    void Init() {
        g_prismaUI = PRISMA_UI_API::RequestPluginAPI<PRISMA_UI_API::IVPrismaUI1>();
        if (!g_prismaUI) {
            SKSE::log::warn("WebUI: PrismaUI API not available; leash panel hotkey disabled");
            return;
        }
        SKSE::log::info("WebUI: PrismaUI API acquired");

        g_view = g_prismaUI->CreateView("SkyrimNet_Leash/index.html", [](PrismaView) {
            g_domReady = true;
            SKSE::log::info("WebUI: DomReady");
        });
        if (!g_prismaUI->IsValid(g_view)) {
            SKSE::log::error("WebUI: CreateView returned invalid view — ensure Data/PrismaUI/views/SkyrimNet_Leash/index.html exists");
            return;
        }
        g_prismaUI->Hide(g_view);

        g_prismaUI->RegisterJSListener(g_view, "onStart", [](const char* value) { HandleStart(value); });
        g_prismaUI->RegisterJSListener(g_view, "onCancel", [](const char*) { Hide(); });

        KeyHandler::RegisterSink();
        KeyHandler::GetSingleton()->Register(kEscapeDx, []() { OnEscape(); });
        SKSE::log::info("WebUI: Escape registered; menu hotkey follows leash.controls.hotkey");
    }

    void SetGameReady() {
        g_gameReady = true;
        SKSE::log::info("WebUI: game ready");
    }
}
