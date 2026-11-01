# Checkpoint 02: struggle ends on every pull, built but not tested in game

Repo: `c:\Skyrim\dev\mods\SkyrimNet_Leashed` | Branch: `main` | Baseline: `33f1da4` (Magelight port), with this work uncommitted on top | Date: 2026-11-01

**Status: implemented. `compile: papyrus` and the SKSE Release build both succeed, and the DLL is copied to `SKSE/Plugins/`. Nothing has been tested in game. Not committed, no version bump, no release.** Supersedes [checkpoint-01.md](checkpoint-01.md), which holds the full log timeline. Logs: [`logs/2026-11-01-08-19-30/`](logs/2026-11-01-08-19-30/MANIFEST.txt). No new logs this session.

## Goal

A leashed actor being pulled or ragdolled must not be struggling. Struggle ends when a pull or ragdoll starts, and it can't start during one.

## Decision

**Struggle ends on every pull**, chosen by the user. LF starts a pull as soon as an actor-held NPC begins following, so **an NPC can now only struggle while the holder stands still.** This reverses the 1.1.3 rule ([leashframework-1.1.3.md:95](../leashframework-1.1.3.md)).

## What changed

Papyrus, in [`SkyrimNet_Leashed_Actions.psc`](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc):
- `OnLeashFrameworkPulled`: any pull ends struggle and sends `StruggleStoppedEvent`. `yank` now only decides `NarratePull`.
- `OnLeashFrameworkRagdollPulled`: sends `StruggleStoppedEvent` when a struggle ended.
- `OnUpdate`: ends struggle when `IsKnockedDown` (with `EndStruggleQuiet`) or `IsBeingPulled` (with `EndStruggle`).
- `StruggleExecute`: refuses both states, before any narration.
- `PlayStruggleAnim`: re-checks `IsKnockedDown` and the struggle slot immediately before sending.

SKSE:
- [`LeashState`](../../SKSE_Source/src/Leash/LeashState.cpp): a `ModCallbackEvent` sink tracks pulls, `PruneBeingPulled` drops actors that are gone or have no 3D, and `IsKnockedDown` moved here from `Bridge.cpp`.
- New native `IsBeingPulled`, and flag decorator `speaker_is_down_or_pulled`.

Actions: both [`leashed_escape_struggle.yaml`](../../SKSE/Plugins/SkyrimNet/external/goodprovider.leashed/actions/leashed_escape_struggle.yaml) and [`leashed_escape.yaml`](../../SKSE/Plugins/SkyrimNet/external/goodprovider.leashed/actions/leashed_escape.yaml) require the decorator to be `unavailable`.

Docs: README, KNOWLEDGE, AGENTS, and `Unreleased` in all three changelogs.

### Differences from the checkpoint-01 plan

- **Pull tracking is native, not Papyrus.** LF stopped and restarted a pull 16 ms apart ([LeashFramework.log:19-20](logs/2026-11-01-08-19-30/LeashFramework.log)), and Papyrus handlers can lag by about 0.6 s with no guaranteed order. No new Papyrus event registration was needed.
- **Pruning added.** LF sends no stop event when the follower can't be resolved (`LeashInstance.cpp:98-99`, `PullController.cpp:406`).
- **Three additions:** the `OnUpdate` being-pulled branch, the slot check in `PlayStruggleAnim`, and the stop event on the ragdoll path.
- **`leashed_escape.yaml` also gated**, because AGENTS.md requires the category to match its child.

## Verified

- Papyrus compiled all 3 scripts with no errors. `IsBeingPulled` is present in `Actions.pex` and `Native.pex`.
- SKSE Release build succeeded. `SKSE/Plugins/SkyrimNet_Leashed.dll` is dated 2026-11-01 08:38.
- Skyrim was not running during the build.

## Not yet tested

Everything in game, including:
- Whether the sink receives LF's events. Look for `speaker_is_down_or_pulled` and `IsBeingPulled` behaviour in tests 2 and 4, since the sink has no log line of its own.
- Whether `IdleForceDefaultState` at the start of a pull disturbs LF's direct locomotion. It's the same call the old yank path made, but it now fires on every follow-start. **This is the main risk: watch for an NPC who doesn't follow, or who hitches, at the start of a walk (test 2).**
- The Magelight port, which the 08:16 playtest didn't run (it logged `PrismaUI API acquired`).

## Open questions

- Finding 5 (the idle blocks the pull) and finding 6 (what looked wrong on screen) remain unproven. Neither affects the fix now. If you want to know, test 2 with LF Debug > Diagnostics > Pull diagnostics on would have to be run on the old build.
- LF references are to the in-repo `Skyrim-Leash-Framework`. The game ran LF 1.1.7, and I did not check that the two match.

## Next

1. Enable the dev copy (`SkyrimNet_Leashed`) in the mod manager and disable `SkyrimNet Leashed`. Confirm the log shows Magelight rather than `PrismaUI API acquired`.
2. In-game tests, on a neck leash with an NPC:
   1. **Stand still:** holder stands still and NPC struggles. Struggle runs to the 7 s cap.
   2. **Walk away slowly:** struggle ends at the first pull, the stop event is sent, and she follows cleanly.
   3. **Sprint away:** struggle ends at the first pull, no `PlayStruggleAnim` follows, and she ragdolls and gets up.
   4. **During a drag:** the action isn't offered. If it's called anyway, the log shows `StruggleExecute skipped`.
   5. **After a pull:** once she settles, she can struggle again, so the flag isn't sticking.
   6. **Wrist leash:** after a ragdoll, the bind reapplies on `GetUpEnd`.
3. Save the logs from those runs, and start the next session from `checkpoint-02.md`.
4. If the tests pass, commit. Release only when you ask for one.
