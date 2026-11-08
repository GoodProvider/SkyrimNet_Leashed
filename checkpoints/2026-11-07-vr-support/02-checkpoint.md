# 02 — 0.5.0 committed and packed (SE verified, VR untested)

Repo / branch / baseline: SkyrimNet_Leashed `main` @ `e195b79`. This session commits the 0.5.0 bump plus the Leashed VR work from [01-checkpoint.md](01-checkpoint.md). SkyrimNet_SexLab and SkyrimNet_DOM were not touched and are still uncommitted in their own repos.

Logs: none. The work needed no logs, and `logs/` had no loose files to move.

## Goal

The user verified the build on SE and asked to commit and release. VR is untested, so it ships for users to test.

## Results

- **SE: verified in game by the user.** This covers the Magelight panel, the review fixes, and the `GetActiveTarget()` crosshair change.
- **VR: untested.** The player-facing docs say so:
  - `CHANGELOG-player.md`: "Skyrim VR (untested, please report)".
  - `README.md`: "Skyrim VR (experimental, untested — please report how it works)".
  - `CHANGELOG.md`: "Untested on VR."
  - `CHANGELOG-developer.md` and `KNOWLEDGE.md` already called it untested.
- `release-checkpoint.xml` version_status: SE verified, VR untested, packed, not tagged.
- Committed on `main`. Ran `make release`, which repacked `versions/SkyrimNet_Leashed 0.5.0.7z`. The 09:26 pack was older than the VR DLL (09:59). No rebuild was needed: the DLL is newer than every SKSE source and the .pex files are newer than every .psc.
- No tag, no push, no GitHub/Nexus release.

## Not yet tested

Everything on Skyrim VR: item 2 of [01-checkpoint.md](01-checkpoint.md#next).

## Next

1. Play the 0.5.0 7z in MO2 if you want a final SE check, then tag 0.5.0 and publish when ready.
2. Collect VR reports from users:
   - Grip + Stick click opens the panel, there is no VR keyboard, and B / Y closes it.
   - The log shows `VR controller chord bound: ...`.
   - Ask for `SkyrimNet_Leashed.log` and `Magelight.log`.
3. Deferred VR `AddTask` audit: `SKSE_Source/src/SkyrimNet/StateCache.cpp:506` (refresh worker). This matters if VR users report hangs.
4. SexLab and DOM VR changes: commit in those repos after testing them.
