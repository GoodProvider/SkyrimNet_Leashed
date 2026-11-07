Repo: `c:\Skyrim\dev\mods\SkyrimNet_Leashed` / branch `main` / baseline `3d6c89e` (changes uncommitted)

# Full code review: fixes from checkpoint-02

## Goal

Commit the review folder, then make the five changes [checkpoint-02](checkpoint-02.md) proposed (F1 counter, F2, F3, F4, F6) and the doc corrections.

## Status

**Committed:** `3d6c89e` adds this folder (checkpoint-01, checkpoint-02 and both log snapshots).

**Built, not committed, not tested in game:** Release DLL built and copied to `SKSE/Plugins/` (807,424 bytes, 09:06). `compile: papyrus` (papyrus.exe) exited 0, 3 files (Actions.pex 66,302 bytes, 09:06). No release.

## In-game result from 08:31 (before these fixes)

The user played the `a05acf5` build. Citations refer to [logs/2026-11-06-09-04-16/](logs/2026-11-06-09-04-16/).

- [LeashFramework.log](logs/2026-11-06-09-04-16/LeashFramework.log) L14–41 shows three leashes: 29000D95 dangling (holder 00000000), 0001B5DF moved to an NPC holder FF000EF1 (`replaced`, L31–34), and FF000F16 held by the player. L42–45: after a **same-process** load at 08:33:41, `Loaded 3 persistent leash(es)`, all `loaded`.
- [Papyrus.0.log](logs/2026-11-06-09-04-16/Papyrus.0.log) has no `Forgetting` line, so `ReconcileCachedPair` kept all three. That covers 11-03 F2 and 11-06 F5 for a same-process load.
- Papyrus L1168 `ragdoll pull`, L1174 `PulseDragEvent` 2 s later, and [SkyrimNet_Leashed.log](logs/2026-11-06-09-04-16/SkyrimNet_Leashed.log) L32–33 (Injury Engine `falling x2`, `leash_dragging x1`): a real drag in play.
- Not exercised: the real-time clock reset (no relaunch), and F4 (nobody was downed at save time).
- Mod log L28 shows the first queued refresh on 70868 (plugin load). L30 shows the off-thread warning again, 203 ms after game ready, on 68852. This is the second boot with that pattern. After the 08:33 reload there was no second warning, because it was once per process. That is why the counter below was added.

## Changes

| For | Change |
|---|---|
| F1 | [NoteRefreshThread](../../SKSE_Source/src/SkyrimNet/StateCache.cpp#L296) counts every queued refresh that runs off the plugin-load thread. It logs `StateCache: N queued refresh(es) ran off the main thread since the last report (last on thread X, plugin load thread Y)` at most once per 60 s, then resets. The first-run info line stays. `g_loggedOffMainThread` is removed. |
| F2 | [Start](../../SKSE_Source/src/SkyrimNet/StateCache.cpp) no longer calls `Refresh()` on the `kDataLoaded` thread. The first tick queues it after 750 ms. |
| F3 | [CollectNearby](../../SKSE_Source/src/Leash/LeashState.cpp#L125) returns recorded leashes (`g_byLeashed`) in a separate vector. In `Refresh`, `nearby` is now the player plus the process lists only. Recorded and pending actors are added to `actors` afterwards, so they still get snapshots and pairs. |
| F4 | [ResetSavedClocks](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L67) also reseeds the downed position from the actor and zeroes `DownedStuckTime` and `DownedStage`, as `WatchDowned` does. |
| F6 | [release-checkpoint.xml](../../release-checkpoint.xml) `updated` is 2026-11-06 and `version_status` is 0.4.5 (not packed, not tagged). |
| Docs | [KNOWLEDGE.md](../../KNOWLEDGE.md) "Refresh thread" and "not nearby" bullets now match the code. [CHANGELOG.md](../../CHANGELOG.md) drops the false-positive claim and adds F1–F4. The developer changelog is updated. In [CHANGELOG-player.md](../../CHANGELOG-player.md), the alt-tab line now says "Should fix", and there are new lines for far-away leashed NPCs and the load drag. |

## Findings

- F3 also drops recorded leashes from the N² speaker/nearby build. They still count as speakers, but `nearby` for every speaker no longer includes them.
- After F2 there is no snapshot for about 750 ms after `kDataLoaded`. That is before any save loads, so it has no player-visible effect.
- The holder's own `visible_pairs` still lists a far-away pair through the endpoint check, as intended.

## Not yet tested

Everything in this change, in game. The F1 thread question is still open until the counter lines appear.

## Logs

[logs/2026-11-06-09-04-16/](logs/2026-11-06-09-04-16/) ([MANIFEST.txt](logs/2026-11-06-09-04-16/MANIFEST.txt)): `SkyrimNet_Leashed.log` 2413 B, `Papyrus.0.log` 281725 B, `LeashFramework.log` 4161 B. Crash log skipped (newest 2026-10-30).

## Next

Make sure the mod manager loads the 09:06 DLL and `.pex`.

1. **Relaunch test (clock reset + F4 + F5):** make a held, a tied and a dangling leash. Yank a held NPC into a ragdoll and save while they are down. **Quit Skyrim**, relaunch and load. All three remain, with no `Forgetting` line, no `PulseDragEvent` from the load itself, and the first yank afterwards narrates (no `ragdoll pull skipped: cooldown`).
2. **F1 counter:** in a busy cell, play a few minutes and alt-tab for 1 minute and then 5 minutes. Read the `ran off the main thread since the last report` lines in `SkyrimNet_Leashed.log`. One line right after game ready and nothing after it means a one-off at load. Lines that keep coming mean the queue drains on worker threads, and Refresh needs a different dispatch.
3. **F3:** leave a leashed NPC in another cell. In Game Data Explorer, `get_nearby_*` for someone in the current cell does not list them.
4. **Hotkey:** `\` in the console does nothing; the hotkey still toggles the panel.
5. Then commit these fixes. Before a release, run `dependency_drift` (LF 1.1.7, SkyrimNet API v14).
