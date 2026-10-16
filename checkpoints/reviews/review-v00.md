# Code review: Injury Engine integration (review-v00)

**Date:** 2026-10-15  
**Scope:** SkyrimNet_Whipped → Injury Engine rename plus fall/drag injury path (`InjuryEngine.cpp`, Papyrus, config, changelogs).  
**Reference:** [injury-engine-handoff.md](../../injury-engine-handoff.md) (rename spec; not edited in this pass).

## Summary

The rename is complete in source: `Injury_Engine.esp`, `Injury_Engine_Main`, `leash.injuryengine.fallDamage`, natives `InjuryEngineFallDamage` / `InjuryEngineInjure`, and `SKSE/Plugins/Injury_Engine/injuries/leash_dragging.json`. VM dispatch matches SkyrimNet_SexLab: `External_Damage(attacker, victim, cause, body_part, damage_type, damage_amount, narrate)` from [SKSE_Source/src/InjuryEngine/InjuryEngine.cpp](../../SKSE_Source/src/InjuryEngine/InjuryEngine.cpp). No `Whipped` identifiers remain outside the handoff doc.

This review fixed one behavior bug and aligned docs that still described a non-existent `External_Injure` API or obsolete `WouldDirectNarrate` / `Strike` narration path.

## Findings (pre-fix)

| Severity | Issue |
| -------- | ----- |
| **Bug** | `OnLeashFrameworkRagdollPulled` called `InjuryEngineInjure(..., text, true)` even when `NarratePull` returned `""` on pull cooldown. Injury Engine could narrate `falling` with no yank cause. |
| **Docs** | `SkyrimNet_Leashed_Native.psc` and `CHANGELOG-developer.md` referenced `External_Injure`. |
| **Docs** | `KNOWLEDGE.md` described `WouldDirectNarrate` + `Strike` for ragdoll pulls; implementation always returns the yank line to Injury Engine when fall damage is enabled. |
| **OK** | Drag ticks: `InjuryEngineInjure(who, "leash_dragging", 1, None, "", false)` — holder omitted on purpose (silent per-tick abrasions; JSON `by_none`). |
| **OK** | 40-unit slide OR holder beyond max length for drag detection (includes first tick after yank when position delta is zero but holder is beyond range). |
| **OK** | No C++ changes required for this review. |

## Changes applied

| File | Change |
| ---- | ------ |
| [Scripts/Source/SkyrimNet_Leashed_Actions.psc](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc) | `InjuryEngineInjure(..., text, text != "")` so cooldown-skipped pulls apply `falling` x2 silently. |
| [Scripts/Source/SkyrimNet_Leashed_Native.psc](../../Scripts/Source/SkyrimNet_Leashed_Native.psc) | Doc: `External_Injure` → `External_Damage`. |
| [CHANGELOG-developer.md](../../CHANGELOG-developer.md) | Unreleased bullet documents `External_Damage` signature and `narrate = (cause != "")`. |
| [KNOWLEDGE.md](../../KNOWLEDGE.md) | Ragdoll pull bullet matches current `NarratePull` + `External_Damage` flow. |

## Build / ship

- **DLL:** unchanged; no rebuild needed for this pass.
- **Papyrus:** run Cursor task **`compile: pyro`** (`pyro skyrimse.ppj`) so `Scripts/SkyrimNet_Leashed_Actions.pex` picks up the `narrate` flag. Skyrim was not running at review time; pyro was not on the agent PATH (see `.vscode/tasks.json` for the extension path).
- **Deploy:** ship with Injury Engine under the new names; an old Leashed DLL still looks up `SkyrimNet_Whipped.esp`.

## In-game verification

With Injury Engine installed and **Falling hurts (Injury Engine)** on:

1. **Ragdoll yank (narrated):** One combined line (yank cause + fall injury) or Injury Engine event rules; `SkyrimNet_Leashed.log` shows `InjuryEngine: installed=true` and `falling x2` with `narrate=true` when cause is non-empty.
2. **Ragdoll yank (cooldown):** Second yank within pull cooldown — `falling` still applies, no extra narration (`narrate=false`, empty cause).
3. **Drag:** While down and dragged, each qualifying 1 s tick adds silent `leash_dragging` x1; dragging events still pulse per `leash.dragEventInterval`.
4. **Setting off:** `leash.injuryengine.fallDamage` false — no injury dispatch; yank line uses normal `Narrate` path.

## Grep sanity (exclude `injury-engine-handoff.md`, build dirs)

Expect no hits for: `SkyrimNet_Whipped`, `WhippedInjure`, `WhippedFallDamage`, `toWhipped`, `leash.whipped`, `External_Injure` (in Leashed source/docs except historical handoff).
