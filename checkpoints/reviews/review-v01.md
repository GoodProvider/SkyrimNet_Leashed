# Code review: Injury Engine integration (review-v01)

**Date:** 2026-10-16  
**Scope:** Re-review of [review-v00.md](review-v00.md) and its working-tree fixes; release prep for **0.4.3** (base tag 0.4.2).  
**Reference:** [injury-engine-handoff.md](../../injury-engine-handoff.md); Injury Engine API `../Injury_Engine/Scripts/Source/Injury_Engine_Main.psc` (`External_Damage`, `Damage`).

## Summary

review-v00's code and doc fixes are correct. One v00 claim was wrong: it said the DLL was the only build output that mattered and that the compile step was a formality. In fact the shipped `.pex` files were from 2026-10-09 16:00, older than commits 10e8854, 878aa88 and ae95d83, and still called `WhippedInjure`, a native the current DLL no longer registers. Shipping that tree would have broken fall and drag injuries. This pass recompiled Papyrus, bumped the version to 0.4.3, rebuilt the DLL, and wrote the 0.4.3 changelogs and checkpoint.

## Verdicts on review-v00

| v00 item | Verdict |
| -------- | ------- |
| `InjuryEngineInjure(..., text, text != "")` on ragdoll pull | **Correct.** `External_Damage` → `Damage` narrates only when `narrate` is true (`Injury_Engine_Main.psc` 1393, 1551). A pull skipped by cooldown lands `falling` x2 silently. The fallback `Narrate` runs only when dispatch fails and `text != ""`. With fall damage off, `NarratePull` narrates itself and returns `""`, so nothing is narrated twice. |
| C++ dispatch signature | **Correct.** `InjuryEngine.cpp` passes `(attacker, victim, cause, body_part="", damage_type, damage_amount, narrate)`, which matches `External_Damage`. |
| Drag tick `attacker = None` | **Correct.** `Damage` / `HitPart` accept None. |
| Docs (`Native.psc`, `CHANGELOG-developer.md`, `KNOWLEDGE.md`) | **Correct**, and they match the code. |
| "DLL unchanged; no rebuild needed" | **True for v00 alone**: the DLL already had `Injury_Engine.esp` / `External_Damage` / `InjuryEngineInjure`. The 0.4.3 bump changes `Version.rc.in`, so this pass rebuilt the DLL. |
| "run `compile: pyro`" (listed as a follow-up) | **Understated.** This was a release blocker, not a follow-up. See F1. |

## New findings

| ID | Severity | Issue | Resolution |
| -- | -------- | ----- | ---------- |
| F1 | **Blocker** | `Scripts/SkyrimNet_Leashed_Actions.pex` / `_Native.pex` were stale (2026-10-09 16:00) and referenced `WhippedInjure`; the DLL registers only `InjuryEngineInjure` / `InjuryEngineFallDamage`. | Recompiled. Both `.pex` files now reference `InjuryEngine*`, with zero `Whipped` strings. |
| F2 | Tooling | The `compile: pyro` task points at `joelday.papyrus-lang-vscode-2024.578.1412/pyro/pyro.exe`, which is not installed. `compile: papyrus` (`c:/Skyrim/tools/papyrus-compiler/papyrus.exe`) lacks the SkyrimNet import, and adding the vanilla import shadows SKSE's `Actor`. Its output was also about 17 KB smaller than pyro's. | Compiled with the official `PapyrusCompiler.exe` (see Build) using `skyrimse.ppj`'s imports and flags. Not fixed in `tasks.json`; the maintainer should update the pyro path or add a task for the official compiler. |
| F3 | Note | When dispatch succeeds but Injury Engine returns -1 (not ready, victim dead), the yank line is lost, because `DispatchStaticCall` reports success before the call runs. | Accepted. This is rare, and a dead victim needs no line. |
| F4 | Note | `SKSE/Plugins/SkyrimNet_Leashed.pdb` (gitignored, not packed) still holds `Whipped` symbol records because MSVC appends to the existing PDB during incremental builds. The DLL has none. | Harmless. A clean `SKSE_Source/build/release` rebuild would clear it. |

## Changes applied (this pass)

| File | Change |
| ---- | ------ |
| `Makefile`, `SKSE_Source/CMakeLists.txt`, `SKSE_Source/vcpkg.json`, `FOMOD/info.xml`, `SKSE/Plugins/SkyrimNet/config/plugins/SkyrimNet_Leashed/manifest.yaml`, `SKSE/Plugins/SkyrimNet/external/goodprovider.leashed/manifest.json` | Version 0.4.2 → 0.4.3. |
| [CHANGELOG.md](../../CHANGELOG.md) | New `0.4.3 — since 0.4.2` section: Papyrus, SKSE / WebUI, Install / FOMOD, Docs. |
| [CHANGELOG-developer.md](../../CHANGELOG-developer.md) | `## Unreleased` → 0.4.3 release link (file style). |
| [CHANGELOG-player.md](../../CHANGELOG-player.md) | `## Unreleased` → 0.4.3 link; bullets rewritten under the 100-character limit. |
| [release-checkpoint.xml](../../release-checkpoint.xml) | base_tag 0.4.2, next_version 0.4.3, version_status, a 2026-10-16 note, Open list. |
| `Scripts/*.pex` | Recompiled (all 3). |
| `SKSE/Plugins/SkyrimNet_Leashed.dll` | Rebuilt as Release, FileVersion 0.4.3. |

v00's uncommitted edits (`Actions.psc`, `Native.psc`, `CHANGELOG-developer.md`, `KNOWLEDGE.md`) are kept as they were.

## Build

- **Skyrim:** not running (checked with `tasklist`).
- **DLL:** `cmake --preset release && cmake --build --preset build-release`, then copied to `SKSE/Plugins/`. Exit 0. FileVersion **0.4.3**, 2026-10-16 00:06. Contains `Injury_Engine.esp` and `External_Damage`; no `Whipped`.
- **Papyrus:** `"C:\Skyrim\dev\skyrim\Papyrus Compiler\PapyrusCompiler.exe" Scripts\Source -all -f=TESV_Papyrus_Flags.flg -output=Scripts -import=Scripts\Source;Headers;SkyrimNet-GamePlugin\Source\Scripts;C:\Skyrim\dev\mods\SKSE64\Scripts\Source;C:\Skyrim\dev\skyrim\Data\Source\Scripts`. Result: 3 succeeded, 0 failed, 0 warnings. `Actions.pex` is 78295 B (old 78258 B), `Native.pex` 1838 B, `PlayerAlias.pex` 711 B (same size as before).
- **Packaging:** `pack_release.ps1` copies all of `SKSE/`, so `SKSE/Plugins/Injury_Engine/injuries/leash_dragging.json` ships. `make release` was **not run**.

## Release readiness (0.4.3)

| Check | State |
| ----- | ----- |
| Version sources agree (6 files) | 0.4.3. No `0.4.2` left in them. |
| Changelogs cover 0.4.3 | Yes (all three). |
| README covers player-visible changes | Yes. The Dragging events and Injury Engine (optional) sections were already present. |
| DLL / `.pex` current | Yes (2026-10-16). |
| Rename grep | Clean, apart from the handoff, the reviews and the gitignored PDB (F4). |
| Not done (maintainer) | Commit, MO2 playtest, `make release`, tag 0.4.3, GitHub Release, Nexus. |

## In-game verification

With Injury Engine (renamed `Injury_Engine.esp`) installed and **Falling hurts (Injury Engine)** on:

0. **Load:** the Papyrus log has no unbound-native errors for `SkyrimNet_Leashed_Native`. `SkyrimNet_Leashed.log` shows `InjuryEngine: installed=true`.
1. **Ragdoll yank (narrated):** one combined line (yank cause + fall injury) or event; the log shows `falling x2 ... narrate=true`.
2. **Ragdoll yank (cooldown):** a second yank inside the pull cooldown still applies `falling`, with no narration (`narrate=false`).
3. **Drag:** while the actor is down and dragged, every qualifying 1 s tick logs `leash_dragging x1 ... narrate=false`. The drag events pulse per **Dragging event interval**.
4. **Setting off:** with `leash.injuryengine.fallDamage` false, nothing is dispatched and the yank line uses the normal `Narrate` path.
5. **Without Injury Engine:** the yank narrates normally, there are no errors, and drag events still fire.

## Grep sanity

Excluding `injury-engine-handoff.md`, `checkpoints/`, build directories and `*.pdb`, there are no hits for `SkyrimNet_Whipped`, `WhippedInjure`, `WhippedFallDamage`, `toWhipped`, `leash.whipped` or `External_Injure`.
