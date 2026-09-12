# CLAUDE.md

Guidance for Claude Code / Cursor agents on SkyrimNet_Leashed.

**Primary agent doc:** [AGENTS.md](AGENTS.md). **Router:** [llms.txt](llms.txt). **Quirks:** [KNOWLEDGE.md](KNOWLEDGE.md).

## What this is

SkyrimNet ↔ Leash Framework bridge.

## Key paths

- Repo: `c:\Skyrim\dev\mods\SkyrimNet_Leashed`
- Papyrus: `Scripts/Source/` → `Scripts/`; project `skyrimse.ppj`
- SKSE: `SKSE_Source/`

## Compile / commits / safety

Same as [AGENTS.md](AGENTS.md): `compile: pyro`; CMake SKSE tasks; commit summary in first 72 chars; SE ≠ VR. Release docs: [release-guide.md](release-guide.md). If Skyrim is running, do not write PrismaUI HTML, DLL, `.pex`, or ESP — ask the user to quit first.
