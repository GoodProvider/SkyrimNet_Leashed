---
name: dependency-drift
description: >-
  Diffs SkyrimNet_Leashed against an updated named dependency, writes
  checkpoints/<dependency-name>-<version>.md, and updates KNOWLEDGE.md.
  Use when the user asks to review current code against a dependency update
  (Leash Framework, SkyrimNet, PrismaUI, or another), ../LeashFramework,
  Nexus 187303, or version drift.
---

# Dependency drift

Review this bridge against a new version of a named dependency. Do not vendor or compile `LeashFramework.psc`. SE ≠ VR.

## Cold start

Read, in order:

1. [KNOWLEDGE.md](../../../KNOWLEDGE.md)
2. [AGENTS.md](../../../AGENTS.md)
3. [llms.txt](../../../llms.txt)
4. The **latest** `checkpoints/<dependency-name>-*.md` for that dependency (Leash Framework: `checkpoints/leashframework-*.md`)

## Checkpoint path

`checkpoints/<dependency-name>-<version>.md` — lowercase, no spaces (`leashframework-1.1.3`). One file per dependency version; keep older files.

## Workflow

1. Name the dependency and version. Default LF install: `../LeashFramework`.
2. Diff public API / assets / events against what this repo pins (`Headers/`, YAML, C++, form IDs).
3. Re-map every caller. For LF: every `LeashFramework.*` native and `LeashFramework_*` mod event in `Scripts/Source/`, plus armor IDs vs `Leash.esm`.
4. Report additive APIs, signature breaks, behavior/event semantics, new meshes/IDs. Do not assume SE = VR.
5. Propose a plan (header refresh, caller changes, docs). After implementation or if review-only, write the new checkpoint using `checkpoints/leashframework-1.1.3.md` as the template.
6. Update [KNOWLEDGE.md](../../../KNOWLEDGE.md) so that dependency’s pointer names the new latest checkpoint.

## Hard stops

- Do not create git tags or GitHub Releases.
- Do not bump Makefile `VERSION` unless the maintainer asks in that review.
- Do not call new APIs unless the review says this bridge must.
- Do not compile or ship Framework `.pex` / DLL from the submodule.

## Done-when

- Checkpoint exists for `<dependency-name>-<version>`
- KNOWLEDGE.md points at it
- Caller map and open items are in the checkpoint
