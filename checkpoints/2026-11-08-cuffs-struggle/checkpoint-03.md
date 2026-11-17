# 03 — 0.5.1 committed and packed

Repo / branch / baseline: SkyrimNet_Leashed `main` @ `f87e94c` (tag `0.5.0` is at `dc4baed`). This file is the only uncommitted change. Supersedes [checkpoint-02.md](checkpoint-02.md).

Logs: none. Packaging needed no logs and `logs/` does not exist, so nothing was moved or copied.

## Goal

The user reported the checkpoint-02 fixes verified and asked for git add, git commit, and `make release 0.5.1`.

## Results

- Commit `f87e94c`, `Release 0.5.1: wrist struggle uses DD cuffs event, wrists pickable`. Not pushed.
- `versions/SkyrimNet_Leashed 0.5.1.7z` (314,361 bytes). `make release` exited 0.
- Version 0.5.1 in all six sources: `Makefile`, `SKSE_Source/CMakeLists.txt`, `SKSE_Source/vcpkg.json`, `FOMOD/info.xml`, `manifest.yaml`, `manifest.json`.
- Release DLL rebuilt for the version only (no C++ change). FileVersion 0.5.1.
- `CHANGELOG.md`, `CHANGELOG-player.md`, `CHANGELOG-developer.md` and `release-checkpoint.xml` moved from `Unreleased` to 0.5.1 (`base_tag` 0.5.0).

## Decisions

- **Bumped all six version sources and rebuilt the DLL**, as `dc4baed` did for 0.5.0. `pack_release.ps1` packs `SKSE/Plugins/SkyrimNet_Leashed.dll`, so the old DLL would have shipped as 0.5.0 inside a 0.5.1 archive.
- **Changelog wording changed to meet `release-guide.md`.** Player bullets now fit 100 characters (the first was split in two). `CHANGELOG.md` H3 `SexLab TargetMenu` is now `SKSE / WebUI`. The developer block has two bullets, under the guide's 5–12, because the release has two fixes.
- **Packed after the commit**, in the order the user gave. `FOMOD/info.xml` was bumped by hand first, so `make release` left the tree clean.

## Findings

- Archive listing has the `.pex`, DLL, `panels/leashed.js`, both ESPs, `FOMOD/info.xml` and `Optional/DDNG_LeashCollars/`, and no `.pdb`.
- Extracted from the archive: `FOMOD/info.xml` says 0.5.1, the DLL is FileVersion 0.5.1, and `SkyrimNet_Leashed_Actions.pex` is hash-identical to `Scripts/`.
- `git status` was empty after `make release`, so ESP deserialization changed no tracked bytes.

## Not yet tested

- The packed 0.5.1 archive has not been installed or played. Verification of the fixes was reported by the user against the repo tree; the 0.5.1 DLL differs from that build only in its version.
- `release-checkpoint.xml` records "verified in game by the maintainer", taken from the user's "verified". Which of checkpoint-02's steps were run was not stated.

## Next

1. Install `versions/SkyrimNet_Leashed 0.5.1.7z` in MO2 and do a quick playtest.
2. When happy: `git push`, tag `0.5.1`, GitHub Release, Nexus. None of these have been done.
3. Reply to the reporter.
