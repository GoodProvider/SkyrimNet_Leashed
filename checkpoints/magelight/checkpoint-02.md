# Checkpoint 02: leash panel ported to Magelight UI, built, not yet tested in game

Repo: `c:\Skyrim\dev\mods\SkyrimNet_Leashed` | Branch: `main` | Baseline: `39d1628` (Release 0.4.4), changes uncommitted | Date: 2026-11-01

**Status: the port is done and compiles. Nothing has run in game.** This supersedes [checkpoint-01.md](checkpoint-01.md), which still has the reasoning behind each decision and the findings table. No logs were needed, so there is no `logs/` folder.

## Goal

The `\` leash panel runs on Magelight UI instead of PrismaUI, with the same behaviour for the player. `PrismaUI/views/SkyrimNet_SexLab/panels/leashed.js` stays, because SexLab's Target Menu reads it.

## Results

Verified:

- `CMake: Build SKSE (Release)` steps succeed. `SKSE/Plugins/SkyrimNet_Leashed.dll` written 08:02.
- `compile: papyrus` passes, 3 files, exit 0. The `.psc` change is comments only; the three `.pex` show as modified.
- No PrismaUI API remains in `WebUI.cpp`. `MagelightUI_API.h` compiled against the PCH with no changes (checkpoint-01 unverified item 3).

Not checked: the edited page script has had no syntax check, because `node` isn't installed. The first in-game open is the first time it runs.

## What changed

All decisions from checkpoint-01 were kept: replace with no fallback, soft dependency, `minHostVersion` 0.30.1, vendored headers, raw `MagelightApi4`.

- [`WebUI.cpp`](../../SKSE_Source/src/WebUI/WebUI.cpp): the port. Findings 1 to 8 are all addressed. The Escape callback map in `KeyHandler` went with the Escape hook, since nothing else used it.
- Vendored `SKSE_Source/lib/Magelight/api/` (1.7.1, `05d05d8`; staged). Removed `lib/PrismaUI`.
- Moved the page to [`Magelight/SkyrimNet_Leashed/views/panel/index.html`](../../Magelight/SkyrimNet_Leashed/views/panel/index.html) with Escape capture and hotkey close.
- `CMakeLists.txt`, `FOMOD/ModuleConfig.xml` and `pack_release.ps1` carry `Magelight/`, and keep `PrismaUI/`.
- Text: README, AGENTS, CLAUDE, `.cursorrules`, llms, release-guide, settings manifest, `.psc` comments, KNOWLEDGE, and an `Unreleased` section in all three changelogs. No version bump.

### Added beyond checkpoint-01

These came from reading the host docs while porting:

1. **Stale `UIModeEntered` gives the mode back.** `CPP.md` ("A quick close and reopen") says `ReleaseUIMode` does not withdraw an entry that hasn't run. Without this, a late entry would leave a hidden view holding the player's controls.
2. **`Show` skips the request when the panel is already open or queued.** The docs don't promise a `UIModeEntered` for re-requesting the view that holds the mode, so `g_openPending` could have stuck and blocked the hotkey. `Show` still refreshes the page in this case.
3. **`g_openPending` also clears on `UIModeExited` and in `SetGameReady`,** so a lost event can't block the hotkey past the next exit or load.
4. **`Show` refuses at the main menu.** Since 1.7.1, asking for UI mode there opens the view, and this page needs a world. `CPP.md` says to guard it. `g_gameReady` alone doesn't cover returning to the main menu from a game.

The 250 ms hotkey guard from checkpoint-01 was **not** added. It stays conditional on test 2 below.

## Findings

**The checkpoint-01 note that "MO2 serves the repo as the mod" is wrong for the current profiles.** `SkyrimNet_Leashed` (this repo) is disabled in every profile. The enabled entry is `SkyrimNet Leashed`, a separate folder holding the installed 0.4.4 release, with the old DLL (790016 bytes, 10-26) and the old PrismaUI page. Testing without switching would silently run the old build.

Magelight is enabled only in the **SkyrimNet SexLab** profile (`+Magelight UI`). That is the only profile that also has `+SkyrimNet Leashed` and Magelight together, so it is the one to test in. The `SkyrimNet_Leash` profile has no Magelight UI line at all.

**`git submodule update --init --recursive` fails** with `No url found for submodule path '../UnforgivingDevices' in .gitmodules`. There is a gitlink `UnforgivingDevices` (`ab055b9`) with no `.gitmodules` entry. It predates this work. The `CMake: Configure (Release)` task chains that command with `&&`, so the task as written likely stops there. This session ran `cmake --preset release` separately. Left alone: not part of this port.

`Magelight.log` is at `My Games\Skyrim Special Edition\SKSE\Magelight.log` per `CPP.md` (checkpoint-01 item 5, from the docs, not yet seen on disk).

## Not yet tested

Everything in game. The riskiest:

1. Whether `KeyHandler` sees `\` in UI mode, and whether the closing press reopens the panel. If it reopens, ignore the hotkey for about 250 ms after a hide.
2. Queued UI mode on the SexLab → Leashed handoff (`EventSend_LeashedOpen`).
3. `RegisterJSListenerEx` and `CreateViewEx` at `kDataLoaded` with no manifest. It matches SexLab and `examples/Magelight.CppPanel`, but this mod hasn't run it.
4. Text rendering differences (Ultralight 2.0 gamma).

Confidence for the `AGENTS.md` figure: about 85%, unchanged, held back by items 1 and 2. Only in-game tests can raise it.

## Next

Before the game, in MO2 on the **SkyrimNet SexLab** profile: enable `SkyrimNet_Leashed`, disable `SkyrimNet Leashed`. Check that the repo's two ESPs load in the same order.

In game, watching `SkyrimNet_Leashed.log` and `Magelight.log`:

1. `\` opens the panel (`leash.controls.hotkeyEnabled` defaults to off).
2. `\` closes it without reopening.
3. Escape peels one column per press, then closes.
4. PageUp closes it with no ghost left on screen.
5. Open immediately after loading a save: works, with no error.
6. Start on each verb (leash, leash to, tie to, give to, unleash), with the game unpaused afterwards.
7. Crosshair preset, and `OpenPanelFor` vertical layout.
8. SexLab Target Menu **leash** panel: `leashed.js` loads and starts a leash.
9. SexLab SkyMessage **leash** → Leashed panel opens with a cursor.
10. SkyrimNet dashboard (PrismaUI) open at the same time: no stuck input.
11. `\` after returning to the main menu: nothing opens.
12. Remove Magelight from the profile: warning in the log, no crash, YAML actions work.

Then:

- Fix what the tests find and write `checkpoint-03.md`. Update player docs if behaviour changed.
- Commit and release only when asked. In `SkyrimNet_SexLab`, `docs/players/hotkeys.md:16,28` will read out of date; that is another repo.
