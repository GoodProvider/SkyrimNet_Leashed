Repo: `c:\Skyrim\dev\mods\SkyrimNet_Leashed` / branch `main` / baseline `a05acf5`

# Full code review: verification pass

## Goal

Check every finding in [checkpoint-01](checkpoint-01.md) against the code at `a05acf5` and the 08:00 startup log. Correct what is wrong and fix the test plan. No code changes this session.

## Status

Read-only. No rebuild, no commit, no release. Skyrim was not running. The live logs have not changed since the 08:08 snapshot, so this pass reuses it and adds no new log folder.

Of the six findings, four stand as written (F2, F3, F4, F6). F1 stands, but the test planned for it can't detect what it is looking for. F5 stands as a risk, but its `IsEquipped` argument is backwards. Confidence about 85%. Everything is still inferred from code plus one startup log.

Note: the copy of checkpoint-01 pasted into this session was the 2026-11-03 text ([../2026-11-03-code-review/checkpoint-01.md](../2026-11-03-code-review/checkpoint-01.md)). This pass reviewed the file on disk in this folder, which is the read-only `a05acf5` review.

## Results

Line numbers are `a05acf5`. Log citations refer to [logs/2026-11-06-08-08-32/](logs/2026-11-06-08-08-32/).

| ID | Checkpoint-01 says | Verdict |
|---|---|---|
| F1 | Queued refresh ran off the plugin-load thread; two refreshes can overlap | **Confirmed, with a gap in the test plan** (below) |
| F2 | `Start()` refreshes directly on the `kDataLoaded` thread, unchecked | **Confirmed**, low impact |
| F3 | Recorded leashes outside the process lists count as nearby for every speaker | **Confirmed** |
| F4 | First downed tick after a load can narrate a drag | **Confirmed** |
| F5 | A failed reconcile is saved; dangling relies on `IsEquipped` | **Risk confirmed, argument corrected** (below) |
| F6 | `release-checkpoint.xml` is a version behind | **Confirmed** |

### F1. Confirmed; the planned test can't detect repeats

- [SkyrimNet_Leashed.log](logs/2026-11-06-08-08-32/SkyrimNet_Leashed.log) L1–3 (plugin load), L28 (first queued refresh) and L29 (`game ready`) are on thread 29628. LF's co-save `Load` is on that thread too ([LeashFramework.log](logs/2026-11-06-08-08-32/LeashFramework.log) L13). So 29628 is the main thread. At L30, 371 ms after game ready, a queued refresh ran on 42624.
- [NoteRefreshThread](../../SKSE_Source/src/SkyrimNet/StateCache.cpp#L293) uses `g_loggedOffMainThread.exchange(true)`, so it warns **once per process**. Checkpoint-01's Next step 1 says to look for "the L30 warning" after alt-tabbing. It will always be there, from startup, and can't show whether off-thread refreshes keep happening during play.
- Overlap: the task clears `g_refreshQueued` before `Refresh()` ([StateCache.cpp:487](../../SKSE_Source/src/SkyrimNet/StateCache.cpp#L487)). [Reset](../../SKSE_Source/src/SkyrimNet/StateCache.cpp#L504) also clears it on `kPreLoadGame` while a task may still be queued. Both paths allow two queued refreshes. They only run at the same time if two threads drain the queue. The real hazard is off-thread game calls (`ProcessLists`, `HasLineOfSight`), with or without overlap.
- **Don't fix this by skipping off-thread refreshes yet.** Only the first refresh is known to have run on 29628, and that was during the load. If every refresh after `game ready` runs on 42624, a skip guard would silently stop all decorators. The next step is instrumentation: keep a count of off-main-thread refreshes and log it with the thread id every 60 s, using the same throttle as `NoteRefreshCost`. Decide the fix from that.

### F2. Confirmed, low impact

[Start](../../SKSE_Source/src/SkyrimNet/StateCache.cpp#L470) calls `Refresh()` from [OnDataLoaded](../../SKSE_Source/src/plugin.cpp#L11), which runs on thread 26312 (log L5–27). No save is loaded yet, so the process lists are close to empty. The fix is one line: drop the direct call and let the first tick queue it 750 ms later. Decorators already fall back to `g_pending` when a speaker has no snapshot.

### F3. Confirmed

[CollectNearby](../../SKSE_Source/src/Leash/LeashState.cpp#L125) appends every `g_byLeashed` actor (L143–148). [Refresh](../../SKSE_Source/src/SkyrimNet/StateCache.cpp#L346) copies that into `nearby` before it adds the pending speakers. Fix: have `CollectNearby` return the process-list actors and the recorded actors separately. `nearby` gets only the first set, and `actors` (speakers and pairs) gets both.

The holder's own `visible_pairs` still lists a far-away pair through the endpoint check ([StateCache.cpp:233](../../SKSE_Source/src/SkyrimNet/StateCache.cpp#L233)). That is intended: a speaker should know about their own leash. Recorded leashes are few, so the cost is small. The main problem is wrong `get_nearby_*` content.

### F4. Confirmed

[ResetSavedClocks](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L67) resets `DownedStarted` and `DownedLastDragEvent`. It leaves `DownedLastX/Y/Z`, `DownedStuckTime` and `DownedStage`. [TickDowned](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L1019) compares against the saved position, and a squared distance of 1600 or more counts as a drag (L1026). [PulseDragEvent](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L1059) then fires, because `DownedLastDragEvent` is 0. Fix: in the `if DownedActors[i]` branch, seed `DownedLastX/Y/Z` from the actor's current position and zero `DownedStuckTime` and `DownedStage`. The other option is to clear downed slots on load, since a ragdoll doesn't survive a save in a known state.

### F5. Risk stands; the `IsEquipped` argument is backwards

- The code matches checkpoint-01's description ([ReconcileCachedPair](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L128)): if LF says not leashed, a held or tied pair gets `UnequipPairArmor` and `ForgetPair`. A dangling pair survives only if `IsEquipped(ArmorForKind(...))` is true.
- Correction: [KNOWLEDGE.md](../../KNOWLEDGE.md) L32–53 says `IsEquipped` can be **true** while no mesh is bound (a false positive). That error would *keep* a dangling record, which is the safe direction. It does not explain a false `IsEquipped` that wrongly drops one. A false negative would need the armor to really be gone on load, for example an outfit reset or another mod stripping it. In that case forgetting the record is the right outcome.
- What remains is the ordering assumption: LF co-save `Load` runs before Papyrus `OnPlayerLoadGame`. This boot supports it (LF L13 is on the main thread at 08:00:47.078, and Papyrus has 0 `SkyrimNet_Leashed` lines), but no live pair existed. `RegisterLeashEvents` also runs from `OnInit` ([Actions.psc:42](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L42)). On a first install the cache is empty, so that path is harmless.
- Severity stays Medium only because the path deletes data. The save/load test in Next step 2 is the check.

### F6. Confirmed

[release-checkpoint.xml](../../release-checkpoint.xml) L15–18: `updated` 2026-11-01, `version_status` says 0.4.4. Makefile, CMake and vcpkg are 0.4.5. It needs updating before tag prep, not before testing.

## Checked and left as-is

The "Checked and left as-is" list in checkpoint-01 still applies: escape YAML gates, hotkey guard, Papyrus noise from other mods, decorator miss behaviour, corpses in the panel, third-party unleash, dependency pins (LF 1.1.7 against the 1.1.4 pin, SkyrimNet API v14 against v12), `tools/` gitignored, and state A.

## Proposed changes (not made)

| For | Change | Size |
|---|---|---|
| F1 | Off-main-thread refresh counter, logged every 60 s with the thread id | SKSE, small |
| F2 | Remove `Refresh()` from `Start()` | SKSE, 1 line |
| F3 | Split recorded actors out of `CollectNearby`; `nearby` = process lists only | SKSE, small |
| F4 | Reseed downed position and stuck counters in `ResetSavedClocks` | Papyrus, 5 lines |
| F6 | Rewrite `release-checkpoint.xml` `version_status` | docs |

Also fix [KNOWLEDGE.md](../../KNOWLEDGE.md) L91 and the `NoteRefreshThread` bullet in [CHANGELOG.md](../../CHANGELOG.md). Both still say the warning was a false positive. [CHANGELOG-player.md](../../CHANGELOG-player.md) states the alt-tab freeze and hitch fixes as fact.

## Not yet tested

Everything in play. The only runtime evidence is the 07:59–08:00 startup with 0 Framework leashes.

## Logs

Reused [logs/2026-11-06-08-08-32/](logs/2026-11-06-08-08-32/) ([MANIFEST.txt](logs/2026-11-06-08-08-32/MANIFEST.txt)). The live `SkyrimNet_Leashed.log`, `LeashFramework.log` and `Papyrus.0.log` have the same sizes and timestamps as the copies (08:00:47, 08:00:47, 08:04:15). The newest crash log is still 2026-10-30.

## Next

1. **Decide:** make the five changes above now, and build and compile before testing, or test the current 0.4.5 build first. Recommended: make the F1 counter, F2 and F4 changes first. Without the F1 counter, step 3 can't answer the thread question.
2. **F5 + F4, before anything else saves:** one held, one tied and one dangling leash. Yank a held NPC into a ragdoll, then save, quit, relaunch and load. All three leashes should remain, with no `Forgetting` line in `Papyrus.0.log`. A downed actor should get no `PulseDragEvent` line from the load itself. The first yank afterwards should narrate, with no `ragdoll pull skipped: cooldown`.
3. **F1 (needs the counter build):** in a busy cell, play a few minutes and alt-tab for 1 minute and for 5 minutes. Read the periodic off-thread count in `SkyrimNet_Leashed.log`. Zero after `game ready` means the 42624 run was a one-off during the load. A count that keeps growing means the queue drains on worker threads during play, and Refresh needs a different dispatch.
4. **F3:** leave a leashed NPC in another cell. In Game Data Explorer, `get_nearby_*` for someone in the current cell lists them now and shouldn't after the fix.
5. **Hotkey:** `\` in the console does nothing; the hotkey still toggles the panel.
6. Before a release: `dependency_drift` (LF 1.1.7, API v14), F6, and the KNOWLEDGE and changelog wording. No commit or release until steps 2–3 have been played.
