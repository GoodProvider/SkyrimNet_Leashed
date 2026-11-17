# 03 — unleash / let hang verified in game and committed

Repo / branch / baseline: SkyrimNet_Leashed `main` @ `bc8ef41` (on top of 0.5.1 `f87e94c`). This file is the only uncommitted change. Supersedes [02-checkpoint.md](02-checkpoint.md).

Logs: [logs/2026-11-17-08-55-46/](logs/2026-11-17-08-55-46/) (`MANIFEST.txt`, SkyrimNet_Leashed, LeashFramework, SkyrimNet_SexLab, Papyrus.0), from the 2026-11-17 08:42–08:53 verification session.

## Goal

The user reported the 01 and 02 changes verified in game and asked for git add + commit.

## Results

- Commit `bc8ef41`, `Unleash removes the collar; add let hang; handle untracked collars`. Not pushed. It includes checkpoints 01–02, both log snapshots, and the leftover `checkpoints/2026-11-08-cuffs-struggle/checkpoint-03.md`.
- `make release` was not run, so the version is still 0.5.1 and the changelog entries stay under `Unreleased`.

## Findings

- The verification session (Bob holding Coralyn) is in the logs:
  - [LeashFramework.log](logs/2026-11-17-08-55-46/LeashFramework.log): leash applied 08:42:47, `disconnected` 08:42:56 (unleash); applied again 08:43:06, `disconnected` 08:43:15.
  - [Papyrus.0.log:1299](logs/2026-11-17-08-55-46/Papyrus.0.log#L1299): `ApplyDangling leashed=Coralyn` at 08:43:15, from let hang.
  - No errors in SkyrimNet_Leashed.log or LeashFramework.log. `NarrateUnleash late event ignored` does not appear, so the 2 s suppression-window hypothesis from 01 is still unconfirmed. The fix works whether or not it is the cause.

## Not yet tested

- The wrist leash and the player unleashing themselves (01 Next step 1) are not shown in the logs. The user's "verified" covers them.

## Next

1. Release when asked (`make release 0.5.2`, plus the version bump in all six sources).
2. Decide whether the Magelight hotkey panel and the LLM decorators should also see untracked collars. That needs a worn-armor check in `LeashState.cpp`.
