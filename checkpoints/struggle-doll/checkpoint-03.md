# Checkpoint 03: struggle ends on every pull, verified and committed

Repo: `c:\Skyrim\dev\mods\SkyrimNet_Leashed` | Branch: `main` | Baseline: `324e8e2` (this work) | Date: 2026-11-01

**Status: committed as `324e8e2`, not pushed. The user reports the work as verified in game. No version bump, no release.** Supersedes [checkpoint-02.md](checkpoint-02.md), which holds the implementation detail; [checkpoint-01.md](checkpoint-01.md) holds the original log timeline. Logs: [`logs/2026-11-01-08-45-44/`](logs/2026-11-01-08-45-44/MANIFEST.txt), from the 08:41 playtest.

## Goal

A leashed actor being pulled or ragdolled must not be struggling. Struggle ends when a pull or ragdoll starts, and it can't start during one.

## Decision

**Struggle ends on every pull**, so an actor-held NPC can only struggle while the holder stands still. This reverses the 1.1.3 rule. The change is described in [checkpoint-02.md](checkpoint-02.md) and is unchanged since then.

## Results

### Shown in the logs

- **The dev copy runs on Magelight:** `WebUI: Magelight host 1.7.1 acquired` ([SkyrimNet_Leashed.log:25](logs/2026-11-01-08-45-44/SkyrimNet_Leashed.log)). `speaker_is_down_or_pulled` registered (line 15).
- **Struggle ends at the pull:** LF sent `OnActorPulled` at 08:42:56 at distance 205 ([LeashFramework.log:17](logs/2026-11-01-08-45-44/LeashFramework.log)), and `StruggleStoppedEvent` was logged the same second ([Papyrus.0.log:1150](logs/2026-11-01-08-45-44/Papyrus.0.log)). The last `PlayStruggleAnim` came just before it (line 1147), and none followed until the next struggle.
- **Struggle works again after a pull:** the pull stopped at 08:43:19 (LeashFramework.log:22) and `StruggleExecute` ran at 08:43:47 (Papyrus.0.log:1273), so the flag doesn't stick.

### Verified by user report, not shown in these logs

- The 7 s cap. The log ends at 08:43:53, 6 s into the second struggle.
- Sprint away and ragdoll. This log has no ragdoll pull event.
- The action being withheld or refused during a drag. No `StruggleExecute skipped` line appears.
- The wrist-leash bind reapplying on `GetUpEnd`.
- Whether `IdleForceDefaultState` at the start of a pull disturbs following (the main risk in checkpoint-02). The logs can't show this; it would only be visible on screen.

These may have been checked in a run these logs don't cover, or by watching the game. If one of them regresses later, this list is where to look first.

## Commit

`324e8e2`: 22 files, the 20 modified source, script and doc files plus checkpoints 01 and 02.

Left out of the commit:
- `checkpoints/struggle-doll/logs/` stays on disk, untracked. No checkpoint log has been committed before.
- This file, because it cites the commit hash. It's untracked until a later commit.
- The DLL, which is gitignored.

## Open questions

- Finding 5 (the idle blocks the pull) and finding 6 (what looked wrong on screen) remain unproven. Neither affects the fix. Checking them would need the old build with LF Debug > Diagnostics > Pull diagnostics on.
- LF references are to the in-repo `Skyrim-Leash-Framework`. The game ran LF 1.1.7, and the two haven't been compared.

## Next

1. Push `324e8e2` when ready.
2. Commit this checkpoint with the next change.
3. Release only when you ask for one. The fix sits under `Unreleased` in all three changelogs, alongside the Magelight port.
