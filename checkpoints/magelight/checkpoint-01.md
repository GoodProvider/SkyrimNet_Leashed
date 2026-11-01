# Checkpoint 01: move the leash panel from PrismaUI to Magelight UI

Repo: `c:\Skyrim\dev\mods\SkyrimNet_Leashed` | Branch: `main` | Baseline: `39d1628` (Release 0.4.4), clean tree | Date: 2026-11-01

**Status: planning only. No code has changed.** This note is meant to stand alone: a new session can do the port from it without earlier context. Line numbers are as of the baseline commit.

## Goal

The `\` leash panel runs on Magelight UI instead of PrismaUI, with the same behaviour for the player:

- `WebUI.cpp` uses only the Magelight API. All PrismaUI C++ is removed.
- The page moves to `Data/Magelight/SkyrimNet_Leashed/views/panel/index.html`.
- One PrismaUI-path file stays (see "The trap" below).

Sources for Magelight, a local clone that is untracked in the SexLab repo:

- `C:\Skyrim\dev\mods\SkyrimNet_SexLab\extern\MageLightUI` at `05d05d8`, version 1.7.1
- Read first: `docs/PRISMAUI_MIGRATION.md`, `docs/CPP.md` ("The rules that bite"), `docs/DISTRIBUTION.md`, and the threading contract at the top of `api/MagelightUI_API.h`

Closest working example: `C:\Skyrim\dev\mods\SkyrimNet_SexLab\SKSE_Source\src\UiEngine.cpp`, plus the Escape fix in SexLab's `KNOWLEDGEBASE.md:3-8`. Its page path logic and its dual-host design should not be copied (see Decisions 1 and 4).

## The trap: `leashed.js` must not move

[`PrismaUI/views/SkyrimNet_SexLab/panels/leashed.js`](../../PrismaUI/views/SkyrimNet_SexLab/panels/leashed.js) is an overlay into **SexLab's** view, not part of Leashed's own panel. SexLab reads that folder under both of its UI hosts (`UiEngine.cpp:26-38`), and `0700_leashed_panel.json` loads it through `panelScript`. So:

- Keep the file where it is, and keep the `PrismaUI` folder in the FOMOD, `pack_release.ps1` and the CMake copy.
- Only `PrismaUI/views/SkyrimNet_Leashed/` moves.
- A blanket "delete PrismaUI" breaks the leash option in SexLab's Target Menu.
- The script uses no PrismaUI API, so it needs no edits.

## Decisions (defaults; the maintainer can override them)

The request said "change from PrismaUI to Magelight", so these defaults take it literally.

1. **Replace, no PrismaUI fallback.** Leashed has one view and few host calls, so SexLab's dual-host adapter isn't worth its cost. Players with only PrismaUI lose the `\` panel, and YAML actions still work.
2. **Require Magelight, do not bundle it** (`DISTRIBUTION.md` Option A). Bundling would make this mod a distributor of Ultralight's runtime. Keep it a soft dependency, as PrismaUI is today: a null API logs a warning and disables the panel. Do not add a FOMOD `moduleDependencies` entry.
3. **`minHostVersion` = 0.30.1**, the version that added `PostGameTask` and the same floor SexLab uses. Bump it if a later API is adopted.
4. **Vendor the headers.** Copy `api/MagelightUI_API.h` and `api/MagelightUI_Mod.h` from 1.7.1 into `SKSE_Source/lib/Magelight/api/`. `extern/` is an untracked clone in another repo, so never build against it. SexLab's `lib/Magelight` copy is older (its header differs from 1.7.1), so copy from `extern`. `git add` the new files, because a fresh checkout won't compile without them.
5. **Use raw `MagelightApi4`**, like `UiEngine.cpp`. The port needs `onEvent` for UIModeEntered, Exited and Refused. The `MagelightMod` wrapper may not expose those; only its signatures were checked.

## Findings: what breaks in a straight API swap

Pages and listeners carry over (`window.onStart`, `InvokeJS`). The page is one inline script with no `requestAnimationFrame`, `resize`, `localStorage`, `fetch` or external assets, so the "hidden page pauses" and storage differences don't affect it. These do need work:

| # | Problem | Where | Fix |
|---|---|---|---|
| 1 | **JS listeners run on the page or render thread, not the game thread.** `HandleStart` → `DispatchStart` does `LookupByID` and `LeashState::IsLeashed` directly. On flat 1.7.0+ that is a race with the game. | `WebUI.cpp:512-520`, `:448-456` | Copy the argument into a heap object, `PostGameTask`, and do everything on the game thread. Example: `UiEngine.cpp:75-86`. |
| 2 | **Escape closes the whole panel, skipping cascade peeling.** The host handles Escape in UI mode, and the C++ sink never sees it (confirmed in game by SexLab, `KNOWLEDGEBASE.md:6`). | `index.html:684-689`, `WebUI.cpp:546` | The page captures Escape and peels, then closes (sketch below). Remove the C++ Escape registration. |
| 3 | **`\` no longer closes the panel.** Game input is suspended in UI mode, so `KeyHandler` won't see the hotkey. This is inferred from the Escape finding and not tested. | `WebUI.cpp:129-131`, `:381-385` | Send the VK in the open payload. The page closes on a matching `keyCode` (not `code`, per migration difference 7). `Config::HotkeyVk()` already exists. |
| 4 | **Leaving UI mode doesn't hide the view** (`TROUBLESHOOTING.md:139`). PageUp or a load boundary would leave a panel on screen with no cursor. | new | `onEvent` on `UIModeExited` for our view: `ShowView(false)`. Must be idempotent, since `Hide()` also triggers it. |
| 5 | **Opening is gated on DOM ready.** Magelight loads hidden pages one per frame after a save, so an early open is refused. The docs say never to gate an open on it, and calls made before DOM ready are delivered after it. | `WebUI.cpp:174-176`, `:186`, `:346-349` | Drop `g_domReady` from `IsReady` and `Invoke`. Log `ViewLoadFailed` for missing pages. |
| 6 | **UI mode now has one owner across mods.** SexLab prefers Magelight, so both mods now share a host. `RequestUIMode` can return `Busy`, or lose a race and report `UIModeRefused`. The likeliest case is `EventSend_LeashedOpen` (`SkyrimNet_SexLab_Menu.psc:424-428`), which closes SexLab's overlay and at once sends the event that calls `OpenPanel`. | `WebUI.cpp:372-373`, `:555-561` | Request `kUIModeFlagPause \| kUIModeFlagQueue`. `ShowView(true)` on `UIModeEntered`, guarded by a pending flag. On `UIModeRefused` or a non-Ok result, clear the flag and log. A visible panel then always has the cursor. |
| 7 | **`AddTask` is the wrong post inside host callbacks** (can hang on VR). | `WebUI.cpp:410` | Once #1 runs on the game thread, `DispatchPapyrus` can dispatch directly. `OpenFor`'s `AddTask` (`:570`) comes from a Papyrus native and can stay on SE. |
| 8 | **Page path.** The host needs an absolute path. | `WebUI.cpp:531` | Use `GetModuleFileNameW` and a UTF-8 `u8string()` (`CPP.md:39-49`). Do not copy SexLab's `current_path()` / `.string()`. |

Smaller points:

- `onCancel` (`WebUI.cpp:543`) is registered but the page never calls it. The Escape sketch below puts it to use.
- `IsHidden`: use `GetViewInfo` with `desc.size` set; `startVisible` there means current visibility (API header line 578). Pattern at `UiEngine.cpp:226-232`.
- `Hide`: `ReleaseUIMode(g_mod)` then `ShowView(false)`. Release also withdraws a queued request, and must clear the pending flag.
- `SetOrder(10)`: `Layer::Panel` plus `SetViewOrder(view, 10)`.
- `MagelightUI_API.h` uses `HMODULE`, and `PCH.h` has no `Windows.h`. `UiEngine.cpp:3` includes `<Windows.h>` first; expect to need the same.
- `RegisterMod` stays at `kDataLoaded`, where `Init()` already runs (`plugin.cpp:30-33`).
- SkyrimNet's own dashboard and chat are PrismaUI, and hosts can't see each other's focus (`PRISMAUI_MIGRATION.md`, "Running both"). Leashed polls no focus state, so no code is needed; just test it.

### Sketches (not compiled)

Page, replacing `index.html:684-689`:

```js
const hasHost = window.magelight && typeof window.magelight.send === 'function';
if (hasHost) window.magelight.send('__escapecapture', '1');
document.addEventListener('keydown', (e) => {
    if (e.repeat) return;
    if (e.key === 'Escape') {
        e.preventDefault();
        if (!peelCascade() && typeof window.onCancel === 'function') window.onCancel('');
    } else if (state.hotkeyVk && e.keyCode === state.hotkeyVk) {
        e.preventDefault();
        if (typeof window.onCancel === 'function') window.onCancel('');
    }
});
```

Also set `state.hotkeyVk` in `openLeashPanel`, and add `std::uint32_t hotkeyVk{}` to `OpenPayload`, set to `Config::HotkeyVk()` only when `HotkeyEnabled()`.

`__escapecapture` must be `'1'` before the first Escape. SexLab calls it at script top level and that works in game.

Native, showing the shape of `Show` and the events:

```cpp
// Show(), after the payload is built:
g_ml->InvokeJS(g_view, ("openLeashPanel(" + json + ");").c_str());
g_openPending = true;
if (const auto r = g_ml->RequestUIMode(g_view, kUIModeFlagPause | kUIModeFlagQueue); r != Result::Ok) {
    g_openPending = false;
    SKSE::log::warn("WebUI: RequestUIMode -> {}", static_cast<int>(r));
}

// onEvent (CallbackThread::GameThread), filtered on ev->view == g_view:
// UIModeEntered: if (g_openPending.exchange(false)) ShowView(view, true)
// UIModeExited:  ShowView(view, false)
// UIModeRefused: g_openPending = false; log detail
// ViewLoadFailed: log detail (names the resolved path)
```

`ToggleOrOpen` should ignore a press while `g_openPending` is set, so a queued request isn't doubled.

## Change list

**C++**
- [`SKSE_Source/src/WebUI/WebUI.cpp`](../../SKSE_Source/src/WebUI/WebUI.cpp): everything above. `WebUI.h`, `Bridge.cpp` and the Papyrus natives keep their signatures.
- [`SKSE_Source/CMakeLists.txt`](../../SKSE_Source/CMakeLists.txt): line 36 include becomes `lib/Magelight/api`. After line 58 add a `copy_directory` for `../Magelight`, and **keep line 58** for `leashed.js`.
- Delete `SKSE_Source/lib/PrismaUI/PrismaUI_API.h`. Add `lib/Magelight/api/*`.

**Files**
- `git mv PrismaUI/views/SkyrimNet_Leashed/index.html Magelight/SkyrimNet_Leashed/views/panel/index.html`, then apply the page edit.
- No `manifest.json`: the DLL creates the view, as in `examples/Magelight.CppPanel`.

**Packaging**
- [`FOMOD/ModuleConfig.xml`](../../FOMOD/ModuleConfig.xml) line 11: add `<folder source="Magelight" destination="Magelight"/>` and keep the PrismaUI line.
- [`python_scripts/pack_release.ps1`](../../python_scripts/pack_release.ps1) lines 1, 73, 91: stage and pack `Magelight` as well.

**Text that names PrismaUI as the panel host** (Papyrus edits are comments only, but `compile: papyrus` after touching `.psc`)
- [`README.md`](../../README.md): 25 (requirement row, link Magelight UI, Nexus mod 193303 per `meta.ini`; confirm the URL), 45, 129, 186. Line 43 is about SexLab's menu and its path stays accurate.
- [`AGENTS.md`](../../AGENTS.md): 49 (game-lock list: add `Magelight/`, keep `PrismaUI/views/SkyrimNet_SexLab/`), 72.
- [`CLAUDE.md`](../../CLAUDE.md) 20, [`.cursorrules`](../../.cursorrules) 20, [`llms.txt`](../../llms.txt) 40: game-lock wording.
- [`release-guide.md`](../../release-guide.md): 43, 109.
- [`manifest.yaml`](../../SKSE/Plugins/SkyrimNet/config/plugins/SkyrimNet_Leashed/manifest.yaml) line 12.
- [`SkyrimNet_Leashed_Native.psc`](../../Scripts/Source/SkyrimNet_Leashed_Native.psc): 146-147, 153.
- [`KNOWLEDGE.md`](../../KNOWLEDGE.md): add the `leashed.js` trap and the Escape and hotkey behaviour.
- Add new changelog entries. Do not rewrite old ones or bump the version unless asked.

**Leave alone**
- `SkyrimNet-GamePlugin/`: SkyrimNet's own PrismaUI use.
- Historical `checkpoints/`, `escape-*.md`, `handoff-checkpoint.md`, `release-checkpoint.xml`.
- `SkyrimNet_SexLab`: its `docs/players/hotkeys.md:16,28` will read out of date. Mention it to the maintainer; that is another repo.

## Rules for this work

- If Skyrim is running, do not write the DLL, `.pex`, ESP or view HTML; ask the user to quit (`AGENTS.md`, Game lock). It was not running at the time of writing; check again.
- `SKYRIM_MODS_FOLDER` is `C:\Skyrim\dev\mods`, so the CMake "deploy" folder is this repo. MO2 serves the repo as the mod, and a moved page has no stale copy elsewhere to clean up.
- Build with **`CMake: Configure (Release)`** and **`CMake: Build SKSE (Release)`**. Debug `/MDd` breaks the SkyrimNet decorator ABI.
- Stop at build and compile. Do not run `make release`, tag or commit unless asked.
- SE is the target. VR is untested and must not be claimed.
- `AGENTS.md` asks for a confidence figure before game changes, target ≥ 90%. Current estimate for this plan: about 85%, held back by the first two items below.

## Not yet tested or verified

Nothing has been built or run. Specifically unverified:

1. Whether `KeyHandler` truly misses `\` in UI mode, and whether the closing press leaks to the game and reopens the panel. If it reopens, ignore the hotkey for about 250 ms after a hide.
2. Queued UI mode on the SexLab → Leashed handoff (`EventSend_LeashedOpen`).
3. Whether `MagelightUI_API.h` compiles against this project's PCH and CommonLib without changes.
4. Whether `Magelight UI` is enabled in the active MO2 profile. It is installed in the mods folder: `VERSION.txt` says 1.7.1, while `meta.ini` has an odd `version=16.0.0.0` (trust `VERSION.txt`).
5. Where `Magelight.log` is written. It is expected alongside the other SKSE logs; the docs name the file, and the folder wasn't checked.
6. Text rendering. Magelight's Ultralight 2.0 uses gamma 2.2, so the panel may look slightly different from PrismaUI's.

## Next

1. Vendor the headers, then port `WebUI.cpp` and the CMake include. Build Release.
2. Move and edit the page. Update FOMOD, pack script and text. Run `compile: papyrus`.
3. Test in game, watching `SkyrimNet_Leashed.log` and `Magelight.log`:
   - `\` opens, and `\` closes without reopening
   - Escape peels one column per press, then closes
   - PageUp closes the panel with no ghost left on screen
   - Open immediately after loading a save: works, with no error
   - Start on each verb (leash, leash to, tie to, give to, unleash), with the game unpaused afterwards
   - Crosshair preset, and `OpenPanelFor` vertical layout
   - SexLab Target Menu **leash** panel: `leashed.js` loads and starts a leash
   - SexLab SkyMessage **leash** → Leashed panel opens with a cursor
   - SkyrimNet dashboard (PrismaUI) open at the same time: no stuck input
   - Remove Magelight from the MO2 profile: warning in the log, no crash, YAML actions work
4. Update the player docs if the tests change behaviour. Write `checkpoint-02.md`.
