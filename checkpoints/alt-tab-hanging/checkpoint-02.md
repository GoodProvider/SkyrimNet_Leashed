Repo: `c:\Skyrim\dev\mods\SkyrimNet_Leashed` / branch `main` / baseline `324e8e2` (changes uncommitted, on top of the existing uncommitted 0.4.5 version bump)

# Alt-tab freeze and 750 ms hitch

## Goal

Assess the user report in [checkpoint-01.md](checkpoint-01.md) and fix what it found: a 12–25 s freeze after alt-tab and a 55–70 ms frame every 750 ms, both gone with only this mod disabled.

## Status

**Compiled, not tested in game.** The Release build succeeded on 2026-11-03 and the DLL is in `SKSE/Plugins/` (804,864 bytes). No game run, no PresentMon, no WPR yet. No commit, no release.

## Findings

The reporter's binary is on this machine: `C:\Skyrim\dev\mods 1.7\SkyrimNet_Leashed\dist\package\SKSE\Plugins\SkyrimNet_Leashed.dll`. Size (743,424) and PDB GUID `431E9FE0-7367-4936-9F8C-941998BA626D`, age 6, match the report. Its PDB was not found, so these come from disassembly. The reported RVAs look like 16-byte buckets, since several fall mid-instruction.

| RVA | Identified as | Confidence |
|---|---|---|
| `+0x36AA0`, `+0x36A90`, `+0x36AC0` (about 75%) | In the function at `+0x36A50`: walks the `TESFile` list, `strlen` on `fileName` (+0x58), `_strnicmp`, then checks `compileIndex` (+0x478). `LookupForm`'s by-name scan. | Confirmed |
| `+0x1720` (8.5%) | `LeashState::DisplayName` (the 6-char `"anchor"` null branch, then `GetDisplayFullName`) | High |
| `+0x628A0` (2%) | `operator new` | High |
| `+0x23C90` (3%) | A `vector<Actor*>::push_back`, probably `BuildSnapshot` | Medium |

Causes:

1. **Hitch: confirmed.** `GetLeashedFaction` / `GetLeasherFaction` ran `LookupForm` by name on every call, and `BuildSnapshot` called them for every speaker × nearby actor.
2. **Freeze: inferred from source, not measured.** `Start()` queued a `Refresh` every 750 ms with no check, so tasks could pile up while the game is unfocused and run back to back on return. At the reported ~30 ms each, a 21 s freeze is about 700 refreshes, or about 9 minutes tabbed out. The report does not say how long the game was unfocused, and its sentence about DLL use while unfocused is cut off.

## What changed

- [LeashConstants.cpp](../../SKSE_Source/src/Leash/LeashConstants.cpp): the faction getters cache the form in an atomic and never cache a miss.
- [StateCache.cpp](../../SKSE_Source/src/SkyrimNet/StateCache.cpp):
  - `g_refreshQueued` allows one queued refresh at a time, cleared before the refresh runs.
  - `RefreshTable` holds per-actor values for one refresh. Payload order and contents should be unchanged.
  - `NoteRefreshThread` and `NoteRefreshCost` add the new log lines.
- `KNOWLEDGE.md` and all three changelogs. The changelog entries say "fixed", so revisit them if testing disagrees.

Differences from the approved plan:

- **Labels stay lazy.** UUID and name are fetched only for actors in a payload, because `FormIDToUUID` is SkyrimNet's function and its side effects are unknown. The plan had them in the up-front record.
- **`Reset()` also clears `g_refreshQueued`.** Added so refreshes can't stop for good if a load ever drops a queued task. I could not check SKSE's source.
- **Thread logging logs the first off-thread refresh too,** not only the first refresh, because the first refresh could land on the main thread and hide later ones.

## Not yet tested

- Whether the freeze scales with time away, and whether it is gone.
- Whether the 750 ms rhythm is gone, and how much a refresh still costs. Payload string building is still N².
- Decorator output and leash behaviour on the new build.

## Open questions

- **Which thread runs `Refresh`?** The report says engine worker threads for the periodic bursts; `StateCache.h` says "Main thread only". The report's stack lines are cut off, and SKSE's source is not under `C:\Skyrim\dev\mods`. If it is off the main thread, the game calls in `Refresh` (`HasLineOfSight` and others) need a separate fix. Overlapping refreshes would also become possible, because the queued flag clears before the refresh runs.
- **"Started with SkyrimNet rc8 → rc9"**: the reporter is unsure, and nothing here explains it.
- **Overall FPS:** the reporter's average only moved from 39.0 to 41.0 with the mod off, so this fixes stutter and not their low frame rate.

## Logs

None used and no `logs/` folder exists here; this session's evidence is the report and the disassembly. Snapshot logs after the in-game test into `logs/<timestamp>/` with a `MANIFEST.txt`.

## Next

Run in a busy cell such as the Solitude market. Make sure the mod manager loads the new DLL.

1. **Baseline, old DLL:** with `dist\package\SKSE\Plugins\SkyrimNet_Leashed.dll` (0.4.4, 790,016 bytes), alt-tab for 1 minute and for 5 minutes, timing the freeze. Expect about 5× longer. If it does not scale, the backlog explanation is wrong.
2. **New DLL, freeze:** repeat both alt-tabs. Expect an immediate return.
3. **New DLL, hitch:** 60 s of PresentMon. Compare gaps of 0.70–0.80 s (report: 58 on, 1 off) and frames over 50 ms (68 on, 8 off).
4. **Log:** in `SkyrimNet_Leashed.log`, find `StateCache: first queued refresh`, any `queued refresh ran on thread` warning, and any `refresh took` line.
5. **Behaviour:** leash an NPC. In Game Data Explorer check `speaker_is_leashed`, the leashed/unleashed/collared lists and `visible_pairs` for the leashed NPC, the holder and a bystander. Then check unleash, the `\` panel, and save/load.
6. **Reporter:** ask them to retest and send the full stack lines. Their ETL is only needed if the retest fails.
7. Decide on commit and release.
