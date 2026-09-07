# AGENTS.md

Agent guidance for SkyrimNet_Leash (SkyrimNet ↔ Leash Framework bridge).

## What this is

Bridge mod between SkyrimNet (LLM) and Leash Framework.

## Key paths

| Path | Role |
| --- | --- |
| `Scripts/Source/` | Papyrus source |
| `Scripts/` | Compiled `.pex` |
| `skyrimse.ppj` | Pyro project |
| `SKSE_Source/` | C++ SKSE plugin |
| `SKSE/Plugins/SkyrimNet/config/actions/` | LLM action YAML |
| `SKSE/Plugins/SkyrimNet/prompts/` | Prompt overlays |
| `Spriggit/SkyrimNet_Leash/` | ESP source of truth |

Repo root: `c:\Skyrim\dev\mods\SkyrimNet_Leash`.

## Compile

- Papyrus: VS Code/Cursor task **`compile: pyro`** only (unless the maintainer asks otherwise).
- SKSE: tasks **`CMake: Configure (Debug|Release)`** then **`CMake: Build SKSE (Debug|Release)`** (cwd `SKSE_Source`).
- In-game testing must use the **Release** SKSE build. Debug `/MDd` breaks SkyrimNet’s `std::string` decorator ABI (SEH on every decorator callback).

## Commit messages

First ~72 characters summarize the commit. Prefer a multi-line body with concrete bullets (paths, YAML names, decorator IDs).

## Standing rules

- **Nexus first:** Leash Framework is [Nexus 187303](https://www.nexusmods.com/skyrimspecialedition/mods/187303).
- **SE ≠ VR** — never assume parity.
- Do not vendor or recompile `LeashFramework.psc`; call the runtime natives.

## SkyrimNet action YAML

- **Category descriptors always carry `intent`.** A category parent file (`customCategory` + `name`, no `scriptName` / `executionFunctionName`, e.g. `leash_leash_.yaml`) includes a `parameterMapping` with a single `dynamic` param named `intent`. SkyrimNet consumes it during the category-selection stage; there is no execution function to validate it against, so this is correct and not a defect. Precedent: OStimNet `tton_CategoryStart.yaml` / `tton_CategoryManage.yaml`.
- **Category eligibility must not be stricter than its children.** A parent gate hides every child, so never gate a category on a condition a child does not need — `is_leash_available` means "alive, out of combat", which would hide Unleash mid-combat even though `is_unleash_available` allows it.
- Child actions are positional: map every Papyrus parameter, using empty `static` values where Papyrus detects the real value at runtime.

## Safety / confidence

State confidence 0–100% before game, script, or ESP changes. Target ≥ 90%.

## Log files 

SkyrimNet: C:\Users\bhuff\OneDrive\Documents\my games\Skyrim Special Edition\SKSE\SkyrimNet.log
SkyrimNet_Leash: C:\Users\bhuff\OneDrive\Documents\my games\Skyrim Special Edition\SKSE\SkyrimNet_Leash.log
LeashFramework: C:\Users\bhuff\OneDrive\Documents\my games\Skyrim Special Edition\SKSE\LeashFramework.log
SkyrimNet_SexLab: C:\Users\bhuff\OneDrive\Documents\my games\Skyrim Special Edition\SKSE\SkyrimNet_SexLab.log
LeashFramework: C:\Users\bhuff\OneDrive\Documents\my games\Skyrim Special Edition\SKSE\LeashFramework.log
