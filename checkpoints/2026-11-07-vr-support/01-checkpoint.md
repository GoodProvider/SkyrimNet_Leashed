# 01 — VR support for the Magelight UIs (Leashed, SexLab, DOM)

Repos / branches / baseline: SkyrimNet_Leashed `main` @ `e195b79` · SkyrimNet_SexLab `main` @ `56cb6082` · SkyrimNet_DOM `main` @ `993911f`. All changes are uncommitted and sit on top of the user's own uncommitted work in each repo (Leashed: the 0.5.0 bump; SexLab: scene work; DOM: vendored CommonLib). Nothing has been committed or released.

Logs: none. This session was code-only with no in-game run, so no `logs/` snapshot was taken and the folder had no loose logs to move.

## Goal

Copy how `../Injury_Engine` added VR support into SkyrimNet_Leashed, SkyrimNet_SexLab and SkyrimNet_DOM, with the focus on Magelight UI.

How Injury_Engine did it:
1. One SE+AE+VR DLL on the MIT MinLL/CommonLibVR 4.39.5 port (`0da667d`).
2. `REL::Module::IsVR()` branches where runtimes differ (`279caa4`: vtable slot 0x104 vs 0x106). On VR, call hooks it hasn't verified are skipped and log `VR: unverified`.
3. No SKSE `AddTask` inside Magelight host callbacks (its `KNOWLEDGEBASE.md:119`).

## Decisions (user)

- VR controller chords are **settings with distinct defaults**: SexLab Grip + A, Leashed Grip + Stick click. They are active while each mod's keyboard hotkey is enabled.
- The VR `AddTask` hang fix covers **UI paths only**. A full audit is in Next.

## Results (built clean, untested in game)

Step 1 (cross-runtime CommonLib) was already in place in all three repos: Leashed has the same vcpkg port as Injury_Engine, SexLab and DOM vendor MinLL 4.39.5 (DOM's copy is the user's uncommitted work). No build files were touched.

### SkyrimNet_Leashed (`SKSE/Plugins/SkyrimNet_Leashed.dll` rebuilt 09:59)
- `SKSE_Source/src/WebUI/WebUI.cpp`:
  - `CrosshairActor` → `CrosshairPickData::GetActiveTarget()`.
  - `Show` adds `kUIModeFlagNoTextEntry`.
  - New `BindVrChord` (`BindVRHotkeyCallback`, only when `QueryCapability("vr") == 1`, from `Init` and `SetGameReady`) with `OnVrChord` → `PostGameTask(OnMenuHotkey)`.
  - New `PostFromEntryPoint`, used by `OpenFor`.
- `SKSE_Source/src/WebUI/Config.{h,cpp}`: `VrButton` / `VrModifier` / `VrHand`.
- `manifest.yaml`: `leash.controls.vrButton` (stick) / `vrModifier` (grip) / `vrHand` (either).
- Docs: `KNOWLEDGE.md` "Leash panel on Skyrim VR", `CHANGELOG.md` / `-player` / `-developer`, `README.md`.

### SkyrimNet_SexLab (`SKSE/Plugins/SkyrimNet_SexLab.dll` rebuilt 10:02)
- `include/UiEngine.h`, `src/UiEngine.cpp`: `BindVrChord(button, modifier, hand, body)` (Magelight only).
- `src/WebUI.cpp`:
  - The menu hotkey lambda is lifted into `ToggleMenuFromHotkey`.
  - New `WebUI_SetMenuVrChord` + `BindMenuVrChord` (also re-bound after `CreateWebUIView`).
  - `include/WebUI.h` declares it.
- `src/Config.cpp` `ApplyMenuHotkey` reads `sexlab.editor.vr_button` (a) / `vr_modifier` (grip) / `vr_hand` (either). It is applied live.
- New `include/TaskPost.h` (`TaskPost::FromEntryPoint`), used by `Call_ProcessHotkey` (`src/Papyrus_WebUI.cpp`) and `Hud::DispatchMenuKey` (`src/Hud.cpp`).
- `manifest.yaml` settings; `KNOWLEDGEBASE.md` "Skyrim VR: the WebUI opens with a controller chord"; `CHANGELOG-player.md` / `-developer.md` under Unreleased.
- The build has one C4864 warning at `Hud.cpp:350`, in existing code (`OpenPanel`'s `.get<bool>()`), not from this change.

### SkyrimNet_DOM (`SKSE/Plugins/SkyrimNet_DOM.dll` rebuilt 10:04)
- `src/Gestures/GestureWatch.cpp` `Install` logs `(VR: unverified)` on VR.
- Docs: `docs/players/requirements.md` "Skyrim VR", `README.md`, `KNOWLEDGEBASE.md` "DOM on Skyrim VR", all three CHANGELOGs under Unreleased.
- DOM has no UI code. Its menus are SexLab overlays, so they open with SexLab's chord.

## Findings

- **Crosshair layout bug (Leashed, all runtimes).** In the SE+AE+VR build, `CrosshairPickData` uses VR's per-device arrays (`target[3]` @0x04, `targetActor[3]` @0x10; `CrosshairPickData.h`). Leashed's old `#if EXCLUSIVE_SKYRIM_FLAT` branch never applied, so on SE/AE `targetActor[0]` read `collisionPoint`. It mostly fell through to `target[0]` (correct offset), so the visible effect was small. `GetActiveTarget()` picks the layout at run time. SexLab already used it.
- **`AddTask` on VR** (`MagelightUI_API.h:32-44`, `extern/MagelightUI/.claude/agents/review-threading.md` item 5). SKSE VR drains its task queue on job threads while holding the lock, so an `AddTask` from a main-thread entry point (input sink, Papyrus native, engine hook) can wait forever. A post from inside an SKSE task is safe. Magelight host callbacks must use `PostGameTask`.
- **Magelight VR defaults are enough for placement and pointing.** Panels lazy-follow, the HUD is head-locked, and the laser works. B / Y is Magelight's built-in close. Consumers only need a controller binding to open views, and `kUIModeFlagNoTextEntry` on views with no text fields.
- SexLab's `AnimSpeed` hook (Actor slot 0x7D) is below the 0xA6 point where VR's slots shift. DOM's `GestureWatch` (`VTABLE_Character[3]`, slot 0x1) has a VR address in CommonLib. Both are expected to work on VR but neither has been verified there.

## Not yet tested

Everything in game: no SE or VR run.

## Next

1. **SE regression** (deploy all three DLLs; Skyrim must be closed while building):
   - Leashed `\` opens the panel with the crosshair NPC preselected.
   - SexLab's hotkey still toggles its menu, and the HUD keys still work.
   - Logs show `WebUI: not VR, controller chord skipped` (Leashed) and `UiEngine: not VR, controller chord skipped.` (SexLab).
2. **Skyrim VR** (Magelight UI with VR presenter, both hotkeys enabled):
   - Grip + Stick click opens the Leashed panel with no VR keyboard, and B / Y closes it.
   - Grip + A opens SexLab's menu, and DOM's TargetMenu entries appear there.
   - Logs show `VR controller chord bound: ...` and DOM's `NotifyAnimationGraph hook (VR: unverified)`.
   - Capture `SkyrimNet_Leashed.log`, SexLab's log, `SkyrimNet_DOM.log`, `Magelight.log`.
3. **Full VR `AddTask` audit** (deferred by decision):
   - SexLab: about 60 sites (`grep AddTask SKSE_Source/src`). Classify each by calling thread (input sink, Papyrus native or hook vs. inside a task), then move the main-thread ones to `TaskPost::FromEntryPoint`.
   - Leashed: `SKSE_Source/src/SkyrimNet/StateCache.cpp:506`.
   - DOM: `SKSE_Source/src/Gestures/GestureWatch.cpp:87`.
4. Commit each repo when you're happy (not done; no release unless asked).
