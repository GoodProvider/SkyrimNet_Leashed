# Release guide

Align docs and version metadata with what will ship as `next_version`. Packaging, git tags, and GitHub Releases are a separate maintainer step.

Agents: use the project `release` skill (`.cursor/skills/release/SKILL.md`). Session state: [release-checkpoint.xml](release-checkpoint.xml).

## Who does what

| Actor | Owns | Does not do unless asked |
|-------|------|--------------------------|
| Agent | Changelogs, freshness-matrix docs, Makefile `VERSION` if missing, checkpoint | `compile: pyro`, CMake, `make release`, git tag, GitHub Release, feature work |
| Maintainer | Compile, package, playtest, tag, publish | Inventing changelog bullets the tree does not support |

**Agent done-when:** `CHANGELOG.md` and `CHANGELOG-user.md` cover `next_version`; freshness-matrix files match the delta; checkpoint is current; hand-off lists leftover version mismatches. Packaging is not part of done.

## Version sources

One release version. If these disagree, stop and ask — do not invent.

| Source | Field | Role |
|--------|-------|------|
| [Makefile](Makefile) | `VERSION` | Authoritative for `make release` package name |
| [SKSE_Source/CMakeLists.txt](SKSE_Source/CMakeLists.txt) | `project(... VERSION)` | DLL resource via `Version.rc.in` |
| [SKSE_Source/vcpkg.json](SKSE_Source/vcpkg.json) | `version-string` | vcpkg manifest |
| [FOMOD/info.xml](FOMOD/info.xml) | `Version` | From `FOMOD_source` during `make release` |
| [SKSE/Plugins/SkyrimNet/config/plugins/SkyrimNet_Leashed/manifest.yaml](SKSE/Plugins/SkyrimNet/config/plugins/SkyrimNet_Leashed/manifest.yaml) | `plugin.version` | SkyrimNet plugin menu |
| git tag matching `VERSION` | — | Created only when the maintainer ships |

Also reconcile checkpoint `base_tag` / `next_version` and the latest version tag.

Releases URL: `https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/{VERSION}`

## Freshness matrix

Update only the rows the delta touches.

| When the delta includes | Update |
|-------------------------|--------|
| Action YAML | `README.md` action tables if names or eligibility changed; `CHANGELOG.md`; `CHANGELOG-user.md` if player-visible |
| Prompts / DirectNarration copy | `README.md` if player-visible; changelogs |
| Papyrus execute / natives | changelogs; `README.md` only if player-facing behavior changed |
| SKSE decorators, WebUI, PrismaUI HUD | `README.md` hotkey / HUD / escape sections if user-visible; changelogs |
| Escape / weakness / strengthen | `README.md` Escape section; changelogs |
| Install / FOMOD / requirements | `README.md` Play / install; `FOMOD/`; changelogs |
| Doc layout or agent map | `llms.txt`; `AGENTS.md` pointers; `release-checkpoint.xml` `doc_files` |

## Doc inventory

| Role | Path |
|------|------|
| End-user front door | `README.md` |
| Agent router | `llms.txt` |
| Agent policy | `AGENTS.md` |
| Changelog (technical) | `CHANGELOG.md` |
| Changelog (player) | `CHANGELOG-user.md` |
| This guide | `release-guide.md` |
| Agent skill | `.cursor/skills/release/SKILL.md` |
| Session state | `release-checkpoint.xml` |

## Changelog rules

Ground truth is the git delta since `base_tag` plus working-tree files that will ship. No invented features. Every bullet must be verifiable.

### `CHANGELOG.md`

Title when a previous version tag exists:

```markdown
## [VERSION](https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/VERSION) — since [BASE](https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/BASE)
```

First public version (empty `base_tag`): omit the “since BASE” clause.

Use only these H3 themes, and only when the group has content:

- Actions
- Papyrus
- SKSE / WebUI
- Escape / HUD
- Install / FOMOD
- Docs

Prefer concrete identifiers (YAML names, decorator IDs, prompt files, manifest keys).

### `CHANGELOG-user.md`

First line = GitHub releases URL for `next_version` (or `Unreleased` while there is no tag). Then 5–12 plain-English bullets. No claims missing from `CHANGELOG.md`.

### Writing bar

- Short technical bullets; one H1 per markdown file; sequential H2/H3; pure Markdown (no HTML).
- Do not promise VR-specific behavior unless verified; SE ≠ VR.
- Ask before drafting when version, ship set, or omissions are unclear.

## Noise (do not changelog)

Ignore the [llms.txt](llms.txt) denylist unless the maintainer says those paths ship: `SKSE_Source/build/`, `versions/`, `dist/`, `z-*`, `UnforgivingDevices/`, submodule internals, crash logs.

## Packaging (maintainer)

Prereqs: `Scripts/*.pex` current (`compile: pyro`); Release SKSE DLL (`/MD`, not Debug `/MDd`); `Spriggit/` JSON is ESP source of truth (not hand-edited binaries). Do not switch the CRT to `/MT`.

`make release` stamps `FOMOD/info.xml` from `VERSION`/`NAME`, deserializes `SkyrimNet_Leashed.esp` from `Spriggit/`, stages ESP + `SKSE/` + `Scripts/` + `PrismaUI/` with `*.pdb` excluded, packs `versions/SkyrimNet_Leashed ${VERSION}.7z`.

GitHub Actions (`workflow_dispatch` on `.github/workflows/package.yml`) builds Release SKSE, packs the same zip, and uploads a private artifact plus a separate PDB artifact. Play the artifact in MO2 before any tag.

Ship order: commit → `compile: pyro` if needed → CMake Release if needed → `make release` **or** download the Actions artifact → MO2 playtest → git tag → GitHub Release → Nexus.
