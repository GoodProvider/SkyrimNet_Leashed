---
name: release
description: >-
  Align SkyrimNet_Leashed release docs and version metadata for next_version.
  Use when preparing a release, writing CHANGELOG.md or CHANGELOG-user.md,
  bumping Makefile VERSION, updating release-checkpoint.xml, documenting the
  delta since base_tag, or when the user asks to ship, tag-prep, or run a
  release-doc pass. Does not create git tags, GitHub Releases, or run make
  release unless asked.
---

# Release documentation

Shared facts (freshness matrix, version sources, doc inventory, packaging): [release-guide.md](../../../release-guide.md). Session state: [release-checkpoint.xml](../../../release-checkpoint.xml). On conflict, the markdown wins for facts; this file wins for agent procedure.

## Cold start

Read, in order:

1. `release-guide.md`
2. `release-checkpoint.xml`
3. `llms.txt`

Then:

1. Take `base_tag` and `next_version` from the checkpoint (fallback: `git describe` / latest version tag). `base_tag` of `none` means first public version.
2. Reconcile with Makefile `VERSION`, `SKSE_Source/CMakeLists.txt` project version, `SKSE_Source/vcpkg.json`, `FOMOD/info.xml`, `SKSE/Plugins/SkyrimNet/config/plugins/SkyrimNet_Leashed/manifest.yaml` `plugin.version`, `SKSE/Plugins/SkyrimNet/external/goodprovider.leashed/manifest.json` `version`, and the latest version tag. **If they disagree, stop and ask. Do not invent a version.**
3. Diff `base_tag...HEAD` (or the full shipping tree when `base_tag` is `none`) plus uncommitted files that will ship.

Ignore the `llms.txt` denylist unless the maintainer says those paths ship (`SKSE_Source/build/`, `versions/`, `dist/`, `z-*`, `UnforgivingDevices/`, submodule internals, crash logs).

## Hard stops

- Do not create the git tag or GitHub Release.
- Do not run `make release`, pack the `.7z`, or trigger Actions unless asked.
- Do not do feature work, refactors, or review fixes.
- Do not invent changelog bullets or README claims.
- Do not commit or push unless the maintainer explicitly asks.
- Do not promise VR-specific behavior unless verified. SE ≠ VR.
- Do not change the CRT to `/MT`.
- Before any game/script/ESP fix that sneaks into this pass: state confidence ≥ 90% and assumptions.

## Workflow

Copy and track:

```
Release-doc progress:
- [ ] 1. Establish delta
- [ ] 2. Clarify before writing
- [ ] 3. Rewrite CHANGELOG.md
- [ ] 4. Rewrite CHANGELOG-user.md
- [ ] 5. Align docs (freshness matrix)
- [ ] 6. Makefile VERSION if missing
- [ ] 7. Update checkpoint
- [ ] 8. Hand-off
```

### 1. Establish delta

Collect user- and author-visible changes since `base_tag`: scripts, action YAMLs, prompts, SKSE/WebUI, FOMOD, docs. Skip denylist noise.

Preflight (report only; do not compile or pack): whether `Scripts/*.pex` (`SkyrimNet_Leashed_Actions`, `_PlayerAlias`, `_Native`), Release SKSE DLL, and `Spriggit/` JSON look current.

### 2. Clarify before writing

Stop and ask if any of these are unclear:

- `next_version` / `base_tag` (Makefile vs CMake vs vcpkg.json vs FOMOD vs manifest vs checkpoint vs tags)
- which working-tree changes ship
- CHANGELOG themes vs docs-only updates
- deliberate omissions

Do not draft `CHANGELOG*`, `README.md`, or other freshness-matrix files until answers avoid invention.

### 3. Rewrite CHANGELOG.md

Title when `base_tag` is a real tag:

```markdown
## [VERSION](https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/VERSION) — since [BASE](https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/BASE)
```

When `base_tag` is `none`, omit the “since BASE” clause.

Use only these H3 themes, and only when the group has content:

- Actions
- Papyrus
- SKSE / WebUI
- Escape / HUD
- Install / FOMOD
- Docs

Prefer concrete identifiers (YAML names, decorator IDs, prompt files, manifest keys). Every bullet must be verifiable in git history or the shipping working tree.

### 4. Rewrite CHANGELOG-user.md

First line:

```text
https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/VERSION
```

Then 5–12 plain-English bullets. No claims missing from `CHANGELOG.md`.

### 5. Align docs

Apply the freshness matrix in `release-guide.md` — only touched rows. Prefer pointers over duplication. Keep `README.md` the player front door.

### 6. Version prep

If `next_version` is missing from Makefile, update Makefile `VERSION` only. `make release` refreshes FOMOD — do not run it unless asked. ESP source of truth is `Spriggit/`, not hand-edited binaries. Do not bump CMake / vcpkg.json / manifest unless the maintainer says those should move with `next_version`.

### 7. Checkpoint

Update `release-checkpoint.xml`: `updated`, `base_tag`, `next_version`, `version_status`, status notes, `related_artifacts`, `doc_files`.

### 8. Hand-off

Report:

- files touched
- version mismatches left
- preflight notes
- suggested maintainer ship steps: commit → `compile: pyro` if needed → CMake Release if needed → `make release` **or** Actions `workflow_dispatch` artifact → MO2 playtest → git tag → GitHub Release → Nexus

## Done-when

- Changelogs cover `next_version`
- Freshness-matrix files match the shipping delta
- Checkpoint is current
- Hand-off lists leftover mismatches

Packaging is not part of done.
