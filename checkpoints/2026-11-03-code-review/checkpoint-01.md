Repo: `c:\Skyrim\dev\mods\SkyrimNet_Leashed` / branch `main` / baseline `46a1a8f` (changes uncommitted)

# Full code review

## Goal

Review the whole mod (Papyrus, SKSE, both UI pages, action YAML, packaging, docs) and fix what it finds.

## Status

**Fixes compiled and built, none tested in game.** `compile: papyrus` exited 0 (3 files). The Release DLL built and was copied to `SKSE/Plugins/` (805,888 bytes, 22:58). No commit, no release.

Estimated confidence that the fixes are correct: about 90%.

## Fixes

Line numbers refer to the edited files. Findings come from reading code. None was reproduced in game.

| ID | Severity | Problem | Fix |
|---|---|---|---|
| F1 | High | `GetCurrentRealTime()` restarts each launch, but saved values are compared against it. `IsSuppressed` stayed true, so a real `OnUnleash` was ignored; pull cooldowns and downed timers also stalled until the new clock caught up. | [`ResetSavedClocks`](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L67), run from `RegisterLeashEvents` on every load |
| F2 | High | `RepushCachedPairs` restored saved pairs without asking Leash Framework (LF), so a stale record returned on every load. | [`ReconcileCachedPair`](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L128). [`OnLeashFrameworkLeash`](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L2546) ignores a leash that has already ended |
| F3 | High | Pending speakers, which can be in other cells, landed in every speaker's nearby lists and added to the refresh cost. | [`nearby`](../../SKSE_Source/src/SkyrimNet/StateCache.cpp#L346) excludes them; they keep their own snapshot and still count for pairs |
| F4 | Medium | Changing a neck or waist leash to wrists left the old mesh equipped for good. | [`UnequipStaleBodyArmor`](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L1328) |
| F5 | Medium | On a cache miss, a pull recorded the pair with no holder, even when LF reported one. | [`ConsumePullCooldown`](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L2460) uses `EnsureLeashedSlot` |
| F6 | Medium | After `UnleashSpeakerExecute`, the downed watch kept running, so drag injuries could hit an unleashed actor. | [`UnequipTrackedFor`](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L1364) uses `ForgetPair`; [`TickDowned`](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L998) needs `IsLeashed` for drag effects |
| F7 | Medium | Legacy `leashed_escape*.yaml` lacked the `speaker_is_down_or_pulled` gate. | Ran `tools/sync_legacy_skyrimnet_content.py`; `diff -r` now shows no differences (actions and prompts) |
| F8 | Medium | Typing `\` in the console opened the panel. | [`IsTypingText`](../../SKSE_Source/src/WebUI/WebUI.cpp#L391) |
| F9 | Low | `0x88840` (High Elf vampire) was used as the Khajiit vampire, giving High Elf vampires "Khajiit fur" lines. | [`BeastRaceLabel`](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L274) uses `0x88845` |
| F10 | Low | Line-of-sight checks could run on actors with no 3D. Defensive; no crash was shown. | [`CanSee`](../../SKSE_Source/src/SkyrimNet/StateCache.cpp#L108) |
| F11 | Low | The `queued refresh ran on thread` warning was false (see Findings). | [`NoteRefreshThread`](../../SKSE_Source/src/SkyrimNet/StateCache.cpp#L293) compares against the plugin-load thread |
| F12 | Low | Stale pins in `llms.txt`, `AGENTS.md` and `KNOWLEDGE.md`, and a no-op block in `NarratePull`. | Updated and removed |

`KNOWLEDGE.md` and the 0.4.5 sections of all three changelogs are updated. The player changelog states the F1 and F2 results as fact, so revisit it if testing disagrees.

## Findings

- **F1 and F2 may explain the stale-leash report.** [has-leash-events checkpoint-02](../has-leash-events-desipite-no-leash/checkpoint-02.md) describes state B: thoughts mention a leash, the framework shows none, and it persists across sessions. F1 gives it a way to start and F2 a way to persist. This is inferred from the code. That checkpoint's state A is not addressed.
- **Reconcile timing is safe, from LF source.** LF restores in its co-save `Load` callback ([LeashSerialization.cpp:129](../../Skyrim-Leash-Framework/src/Serialization/LeashSerialization.cpp#L129)), and `IsLeashed` reads that table directly ([LeashManager.cpp:346](../../Skyrim-Leash-Framework/src/Leash/LeashManager.cpp#L346)). I am relying on `Load` running before Papyrus `OnPlayerLoadGame`; an in-game save/load test confirms it. The source is the submodule, which may not match the LF version players run.
- **Refresh thread (F11).** In [SkyrimNet_Leashed.log](logs/2026-11-03-22-56-15/SkyrimNet_Leashed.log), thread 62572 ran plugin load (L1-3) and the queued refresh (L28-29). Thread 6068 ran the `kDataLoaded` work (L5-27). So refresh is on the plugin-load thread, very likely the main thread, which answers the open question in [alt-tab checkpoint-02](../alt-tab-hanging/checkpoint-02.md) for this machine. It does not explain the worker-thread bursts in the user's profile.
- **That log is startup only.** It ends at `game ready` (L30) with no `refresh took` line. It comes from the build before these fixes, and it does not show that the freeze or hitch is gone.
- **Runtime API.** The same log shows SkyrimNet API v14 (L5), while the pin is v12.

## Reported, not changed

- **State A (leash armor unequipped, or a leash loaded with no mesh):** needs the two decisions listed in has-leash-events checkpoint-02.
- **A real `OnUnleash` inside the normal 2 s suppress window:** a re-check would race with the explicit unleash flows. F1 removes the long-lasting version.
- **Third-party leashes:** when another mod's held leash ends, `NarrateUnleash` can treat it as dangling and equip our armor. Whether we should narrate or touch those at all is a design choice.
- **Decorator flicker:** a pending speaker gets a snapshot only every other refresh.
- **Corpses in the `\` panel:** could be intentional (dragging a body).
- **LF drift:** a log in has-leash-events shows LF 1.1.7 and the submodule's CMake says 1.1.6, but the pin is 1.1.4. Needs a `dependency_drift` run, as does SkyrimNet API v14.
- **`tools/` is gitignored,** yet the README and `KNOWLEDGE.md` tell people to run the sync script from it, so a fresh clone doesn't have it. This is probably how F7 drifted.
- **`UnforgivingDevices` gitlink has no `.gitmodules` entry,** so `git submodule status` exits 128 after printing the others. The Skyrim-Leash-Framework and SkyrimNet-GamePlugin SHAs did not move during this build.

## Not yet tested

Everything in game, including whether held, tied and dangling leashes survive save/load after F2.

## Logs

[logs/2026-11-03-22-56-15/](logs/2026-11-03-22-56-15/) with [MANIFEST.txt](logs/2026-11-03-22-56-15/MANIFEST.txt). It holds only `SkyrimNet_Leashed.log`.

## Next

Make sure the mod manager loads the new DLL and `.pex` files. F1 needs real elapsed time, so play several minutes before the save.

1. **F2 regression first:** make one held, one tied and one dangling leash, save, quit, relaunch, load. All three should remain, with no `Forgetting` line in `Papyrus.0.log`.
2. **F1:** before that save, yank a held NPC off their feet (walk away until the ragdoll pull), so a pull time is saved. After the relaunch, yank them again. The first pull should narrate, with no `ragdoll pull skipped: cooldown` line in `Papyrus.0.log`. If another installed mod ends leashes through LF, also end one soon after the relaunch: the record and armor should clear. The LF menu has no unleash option.
3. **F2:** leave a leash dangling, unequip the rope, save and reload. `0409` should no longer list it in `openrouter_input.log`.
4. **F3:** with Game Data Explorer open, far-away actors stay out of `get_nearby_*`, and `refresh took` lines stay small.
5. **F4:** with an NPC on a neck leash, trigger a new leash on their wrists (LLM action), then unleash. Nothing stays equipped.
6. **F6:** unleash while an actor is down. Drag lines stop.
7. **F8:** `\` in the console does not open the panel. The hotkey still opens and closes it in normal play.
8. **F11:** no `queued refresh ran on thread` warning in `SkyrimNet_Leashed.log`.
9. **Alt-tab:** the tests in alt-tab checkpoint-02 are still open.
10. Answer the state A decisions, then decide on commit and release.
