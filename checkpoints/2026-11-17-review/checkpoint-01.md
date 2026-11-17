# 01 — Release 0.5.2

Repo / branch / baseline: SkyrimNet_Leashed `main` @ `bc8ef41` (unleash / let hang work), base tag for the release `0.5.1` (untagged; latest git tag is `0.5.0`). Supersedes [checkpoint-00](checkpoint-00.md).

Logs: no new snapshot. The maintainer verified checkpoint-00's tests in game; the work needed no log reading. Previous snapshot: [logs/2026-11-17-10-16-20/](logs/2026-11-17-10-16-20/).

## Goal

The maintainer reported checkpoint-00's Next list verified and asked for `make release` 0.5.2.

## Results

- Version 0.5.2 in `Makefile`, `SKSE_Source/CMakeLists.txt`, `SKSE_Source/vcpkg.json`, `FOMOD/info.xml` (also stamped by `make release`), `manifest.yaml`, `manifest.json`.
- Changelog headers: `CHANGELOG.md` "## [0.5.2] — since [0.5.1]", `CHANGELOG-developer.md` 0.5.2 URL line, `CHANGELOG-player.md` "## [0.5.2]". The developer "Let hang review fixes" bullet now says verified in game 2026-11-17.
- `release-checkpoint.xml`: base_tag 0.5.1, next_version 0.5.2, version_status and a 2026-11-17 note.
- Release SKSE DLL rebuilt (`cmake --build --preset build-release`) so FileVersion is 0.5.2; copied to `SKSE/Plugins/`.
- `make release` packed `versions/SkyrimNet_Leashed 0.5.2.7z` (316,548 bytes, 84 files).
- Committed. Not tagged, not pushed, no GitHub Release.

## Verified

- Per the maintainer: checkpoint-00 Next steps 2–6 (single let hang line, struggle ends, tied let hang wording, untracked DOM collar, unleash regressions).

## Still open

- Native `LeashStatus` ([Bridge.cpp:211](../../SKSE_Source/src/Papyrus/Bridge.cpp#L211)) returns `world` for the plugin's own dangling record, so the TargetMenu offers a no-op `let hang`. Needs a DLL change.
- Magelight hotkey panel and decorators do not see an untracked collar.
- Tags 0.5.1 and 0.5.2 do not exist yet.

## Next

1. Install `versions/SkyrimNet_Leashed 0.5.2.7z` in MO2 and smoke-test if wanted.
2. Tag and publish (0.5.1, 0.5.2) when ready: maintainer step.
3. Decide whether to fix native `LeashStatus` returning `world` for a dangling record.
