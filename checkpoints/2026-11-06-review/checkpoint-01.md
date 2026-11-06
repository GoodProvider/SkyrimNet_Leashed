Repo: `c:\Skyrim\dev\mods\SkyrimNet_Leashed` / branch `main` / baseline `a05acf5`

# Full code review

## Goal

Review the mod as it stands at `a05acf5` (the 2026-11-03 fixes, committed this morning) and record what is still wrong. No code changes this session.

## Status

Read-only. Confidence in the findings below is about 85%. The only runtime evidence is a startup log from 07:59–08:00: the 0.4.5 DLL reached game ready, Leash Framework restored 0 leashes, and Papyrus logged nothing from this mod. None of the 2026-11-03 fixes have been exercised in play.

`a05acf5` itself is consistent with its commit message. `ResetSavedClocks`, `ReconcileCachedPair`, `EnsureLeashedSlot`, `ForgetPair` from `UnequipTrackedFor`, the wrist-mesh unequip, the Khajiit vampire id `0x88845`, the `CanSee` 3D guard, the hotkey typing guard, and the escape YAML gates all match the comments and [AGENTS.md](../../AGENTS.md). What follows is what that pass left open, plus what this morning's log changes.

## Findings

Line numbers are `a05acf5`. The log citations are [logs/2026-11-06-08-08-32/SkyrimNet_Leashed.log](logs/2026-11-06-08-08-32/SkyrimNet_Leashed.log).

| ID | Severity | Problem |
|---|---|---|
| F1 | High | A queued refresh ran off the plugin-load thread, and the queue allows a second refresh to start before the first returns. |
| F2 | Medium | The first refresh of a session is a direct call from `kDataLoaded`, which this log shows is a different thread, and that call is not checked. |
| F3 | Medium | Recorded leashes outside the process lists are still copied into every speaker's nearby payloads. |
| F4 | Medium | The first downed tick after a load can treat a position change as a drag. |
| F5 | Medium | Load reconciliation deletes a saved pair when its check fails, and that deletion is what gets saved next. |
| F6 | Low | `release-checkpoint.xml` still describes the tree as 0.4.4. |

### F1. Queued refresh left the plugin-load thread

[StateCache.h](../../SKSE_Source/src/SkyrimNet/StateCache.h) says `Refresh` is main-thread only, because it calls into the game (`ProcessLists`, faction checks, `HasLineOfSight`). [NoteMainThread](../../SKSE_Source/src/plugin.cpp#L58) records the `SKSEPluginLoad` thread as that thread.

This morning, that thread was 29628: plugin load (log L1–3), the first queued refresh (L28), and `WebUI: game ready` (L29). At 08:00:47.517, 371 ms later, the warning fired (L30):

`StateCache: queued refresh ran on thread 42624, plugin load ran on thread 29628`

[NoteRefreshThread](../../SKSE_Source/src/SkyrimNet/StateCache.cpp#L293) logs that once, so later mismatches are invisible. The interval is 750 ms ([kRefreshInterval](../../SKSE_Source/src/SkyrimNet/StateCache.cpp#L46)), and the task clears `g_refreshQueued` before `Refresh` returns ([StateCache.cpp](../../SKSE_Source/src/SkyrimNet/StateCache.cpp#L485)). The comment there says a tick during a refresh may queue the next one. On one thread the SKSE queue would still run them in order. On two threads both can be inside game calls together. This log does not prove an overlap; it proves the two-thread case can happen, on the build that was supposed to make the warning a false positive.

[KNOWLEDGE.md](../../KNOWLEDGE.md) ("Refresh thread") and [CHANGELOG.md](../../CHANGELOG.md) (the `NoteRefreshThread` bullet) say the warning was false because `kDataLoaded` was the wrong reference. That was true of the 2026-11-03 startup log. It is not true of this one. [CHANGELOG-player.md](../../CHANGELOG-player.md) states the alt-tab freeze and the 750 ms hitch as fixed. Both are still untested, and this is the open question in [alt-tab checkpoint-02](../alt-tab-hanging/checkpoint-02.md).

No crash followed. The newest crash log is `crash-2026-10-30-19-39-39.log`, from before this DLL. Leash Framework had 0 persistent leashes, so `HasLineOfSight` had no pairs to test.

### F2. `Start()` refreshes on the `kDataLoaded` thread

[Start](../../SKSE_Source/src/SkyrimNet/StateCache.cpp#L470) calls `Refresh()` on the caller, then starts the 750 ms thread. The caller is [OnDataLoaded](../../SKSE_Source/src/plugin.cpp#L11). In this log that callback is thread 26312 (decorator registration through "registered with SkyrimNet", L5–24). `NoteRefreshThread` only runs inside the queued task, so this direct call is never compared to the plugin-load thread.

26312 is the same role the 2026-11-03 review decided was the wrong reference for "main thread". The queued path was moved off it. The startup path was not.

### F3. Recorded leashes still count as nearby

[CollectNearby](../../SKSE_Source/src/Leash/LeashState.cpp#L125) appends every actor in `g_byLeashed` after the process lists. [Refresh](../../SKSE_Source/src/SkyrimNet/StateCache.cpp#L346) copies that whole vector into `nearby` and only then adds pending speakers. Pending speakers stay out of other speakers' lists. A recorded leash in another cell does not: it is in `get_nearby_*` for every speaker, and the holder's `visible_pairs` includes it through the endpoint exception even when [CanSee](../../SKSE_Source/src/SkyrimNet/StateCache.cpp#L108) returns false.

The player changelog's "NPCs far away no longer show up as nearby leash targets" describes the pending-speaker change. It does not describe this path. Each of those actors is also an extra speaker in the N² snapshot build, which is the cost the 750 ms hitch work was reducing.

### F4. Load can narrate a drag that did not happen

[ResetSavedClocks](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L67) zeros `DownedLastDragEvent` and, for anyone still downed, sets `DownedStarted` to now. It leaves `DownedLastX/Y/Z`, `DownedStuckTime`, and `DownedStage`. The next [TickDowned](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L998) treats a 40-unit move (squared distance 1600) as a drag. Because the event timestamp is 0, [PulseDragEvent](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L1057) does not treat it as inside the interval. A ragdoll whose position settles across the load gets `leash_dragging` and a drag line on the first tick. A stuck watch that was near 8 s or 14 s continues from the saved counter.

### F5. A failed reconcile is written into the save

[ReconcileCachedPair](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L128) runs from `OnPlayerLoadGame`. If `LeashFramework.IsLeashed` is false:

- holder or tied: unequip, `ForgetPair`, trace `Forgetting leash`
- dangling: keep the pair only when `IsEquipped` is true for `ArmorForKind`; otherwise `ForgetPair` and leave whatever mesh is there

`ForgetPair` clears the Papyrus record. The next save stores that. The usual SKSE order (co-save `Load`, then Papyrus `OnPlayerLoadGame`) makes `IsLeashed` the right question for a held or tied leash, and this boot cannot contradict it: [LeashFramework.log](logs/2026-11-06-08-08-32/LeashFramework.log) L13 is `Loaded 0 persistent leash(es)`, and Papyrus has no `Forgetting` line. It is still the destructive path, and it has not been tested with a live pair.

Dangling depends on `IsEquipped` at that moment. This mod's own notes say `IsEquipped` is not the same thing as a bound mesh. A false result here drops the dangling record permanently.

[OnLeashFrameworkLeash](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L2546) now returns before `NotifyLeash` when `IsLeashed` is already false. That matches the "event arrived after the leash ended" case. A `loaded` event that arrives early is also dropped; `RepushCachedPairs` is the path that is supposed to put the pair back, and it uses the same `IsLeashed` answer.

### F6. Release checkpoint is a version behind

[release-checkpoint.xml](../../release-checkpoint.xml) (`updated` 2026-11-01, `version_status`) still says Makefile, CMake, vcpkg, FOMOD, and both manifests are 0.4.4. They are 0.4.5. The three changelogs already have a 0.4.5 section. The DLL log line is `SkyrimNet_Leashed v0-4-5-0`.

## Checked and left as-is

- **Escape YAML.** `leashed_escape` and `leashed_escape_struggle` (canonical and the legacy copies committed in `a05acf5`) gate on `is_struggle_enabled`, `speaker_is_leashed`, not struggling, and not down or pulled. That matches the category rule in AGENTS.md.
- **Hotkey.** `IsTypingText` covers the console menu and `textEntryCount`. Untested in game.
- **Papyrus log.** [Papyrus.0.log](logs/2026-11-06-08-08-32/Papyrus.0.log) has no `SkyrimNet_Leashed` line. The `OnGameReload` / `Setup` errors at L232, L236, L243, and L293 are Precision, OSex, TrueHUD, and SexLab UDNG.
- **Decorator miss for actors outside the process lists.** A miss inserts `g_pending` and returns false or `[]`. The next refresh publishes a snapshot, and the refresh after that drops it, because `g_snapshots` is replaced and `CollectNearby` will not see them. Every other query fails. [KNOWLEDGE.md](../../KNOWLEDGE.md) already gives this as the reason start-leash categories have no eligibility rules.
- **Corpses in the `\` panel.** [BuildOpenPayload](../../SKSE_Source/src/WebUI/WebUI.cpp#L264) keeps any non-deleted actor inside 1024 units. Still looks intentional for dragging a body.
- **Third-party unleash.** `NarrateUnleash` can still turn another mod's ended leash into our dangling armor. Design choice, unchanged.
- **Dependency pins vs this boot.** Leash Framework log L1 is v1.1.7; the pin is 1.1.4. SkyrimNet log L5 is API v14; the pin is v12. Both were already called out on 2026-11-03. `tools/` is gitignored, so the legacy YAML sync script is not in a fresh clone.
- **State A** (mesh and record disagreeing across a load) still needs the decisions in [has-leash-events checkpoint-02](../has-leash-events-desipite-no-leash/checkpoint-02.md). F5 is the code path that can create it.

## Not yet tested

Everything in play. This boot loaded a save with no Framework leashes and exited before any leash, struggle, yank, panel, or alt-tab.

## Logs

[logs/2026-11-06-08-08-32/](logs/2026-11-06-08-08-32/) with [MANIFEST.txt](logs/2026-11-06-08-08-32/MANIFEST.txt). Copied `SkyrimNet_Leashed.log` (2146 bytes, 08:00:47), `Papyrus.0.log` (198702 bytes, 08:04:15), and `LeashFramework.log` (792 bytes, 08:00:47). Newest crash log is 2026-10-30 and was not copied.

## Next

The 0.4.5 DLL and `.pex` are already what the game loaded. No rebuild unless F1–F5 get a code change.

1. **F1, in a busy cell:** play a few minutes, alt-tab for 1 minute and for 5 minutes. In `SkyrimNet_Leashed.log`, the L30 warning means another off-thread refresh. Time the return. A freeze that still grows with time away means the backlog account is incomplete. Any `refresh took` line is the remaining hitch.
2. **F5, before anything else that saves:** one held, one tied, and one dangling leash. Save, quit, relaunch, load. All three should remain. Papyrus should have no `Forgetting` line. Then yank once after that load (F4 / the clock reset): the first pull should narrate, with no `ragdoll pull skipped: cooldown`, and a downed actor should not get a drag line from the load itself.
3. **F3:** with a leashed NPC left in another cell, Game Data Explorer `get_nearby_*` for someone in the current cell should not list them.
4. **Hotkey:** `\` in the console does nothing. The hotkey still opens and closes the panel in play.
5. Alt-tab steps 1–3 in [alt-tab checkpoint-02](../alt-tab-hanging/checkpoint-02.md) are still open. Dependency pins (Leash Framework 1.1.7, SkyrimNet API v14) need a `dependency_drift` pass before a release. `release-checkpoint.xml` needs a rewrite before tag prep. No commit and no release until 1–2 have been played.
