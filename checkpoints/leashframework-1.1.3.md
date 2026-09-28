# Checkpoint: Leash Framework 1.1.3

**Status:** SkyrimNet_Leashed **0.3.0** reviewed and adapted against installed Leash Framework 1.1.3. Next LF bump: skill `dependency_drift`, then a new `checkpoints/leashframework-<version>.md`.

**Repo:** `c:\Skyrim\dev\mods\SkyrimNet_Leashed`  
**Compared:** this repo 0.3.0 vs `../LeashFramework` (`LeashFramework 1.1.3.7z`). DLL FileVersion **1.1.2**. GitHub `asdasdduck/Skyrim-Leash-Framework` `master` **`bf3b33c`**.

Read first: [KNOWLEDGE.md](../KNOWLEDGE.md), [AGENTS.md](../AGENTS.md), `.cursor/skills/dependency_drift/SKILL.md`.

---

## Header pin

[`Headers/LeashFramework.psc`](../Headers/LeashFramework.psc) matches 1.1.3 comments and natives. **Do not compile or ship** `LeashFramework.pex`. Submodule `Skyrim-Leash-Framework` is at `bf3b33c`.

## Natives

**We call:** `ApplyLeashToHand`, `ApplyHolderOwnedLeashToBone`, `ApplyLeashAtPosition`, `DisconnectLeash`, `UnleashAll`, `IsLeashed`, `GetLeashHolder`, `GetMaxLeashLength`.

**Declared, unused:** `ApplyLeash`, `ApplyLeashToBone`, `IsLeashHolder`, `GetLeashedActors`, `GetMinLeashLength`, `SetMinLeashLength`, `SetMaxLeashLength`, **`SetRagdollOverride`**, **`SetTeleportOverride`**.

Do not call the override natives unless wrist `GetUpEnd` rebind fails after the ragdoll rewrite. **2026-09-10 playtest:** wrist pose came back; leave unused.

## Behavior delta (1.1.1 → 1.1.3)

- `minLength` = settle when the holder stops. `maxLength` = catch-up bound.
- Actor-held: follow can start before maxLength; moving gap is **40%** from min to max (`movingFollowGap = 0.4`).
- World-tied: pull still starts only past maxLength, stops at minLength.
- `OnActorPulled` fires for that earlier follow as well as classic taut.

## Pull policy we implemented

`OnLeashFrameworkPulled` in [`SkyrimNet_Leashed_Actions.psc`](../Scripts/Source/SkyrimNet_Leashed_Actions.psc):

- Ignore dead senders.
- **Follow** (no `EndStruggle`, no taut narrate): holder exists and `numArg < GetMaxLeashLength`.
- **Yank** (end struggle + taut): no holder (world-tie) or `numArg >= maxLength`.
- Ragdoll pull unchanged (yank + wrist `GetUpEnd` rebind). Unclip / stop still end struggle.

## Distance defaults (0.3.0)

Per-token settle (`leash.settle.*`) and catch-up (`leash.distance.*`). Native `DistanceMin`.

| Token | Settle | Catch-up | Follow gap |
| --- | --- | --- | --- |
| tight | 50 | 120 | ~78 |
| short | 90 | 220 | ~142 |
| middle | 150 | 380 | ~242 |
| long | 240 | 580 | ~376 |

Existing WebUI overrides of `leash.distance.*` keep old catch-up numbers until reset.

## Wrist meshes

Type stays chain. Distance picks Leash.esm slot 39:

| Token | EDID | FormID |
| --- | --- | --- |
| tight / short | `Leash_hand_chain` | `0xD69` |
| middle | `Leash_hand_chain_long` | `0x1` |
| long | `Leash_hand_chain_xlong` | `0x3` |

Missing form → `0xD69`. Detect/unequip treat all three as wrists chain. Holder-worn `0xD69` is still legacy `holder_shield`.

Neck/waist IDs unchanged: `0x804` / `0x806` / `0x2CE` / `0x800`.

## Caller map

All LF contact is Papyrus natives + faction mirror. This plugin does not call into `LeashFramework.dll` from C++.

| Native / event | Where |
| --- | --- |
| `ApplyLeashToHand` | `SkyrimNet_Leashed_Actions.psc` actor-held apply (left hand) |
| `ApplyHolderOwnedLeashToBone` | same, `holder_shield` / holder-worn mesh |
| `ApplyLeashAtPosition` | world-tie |
| `DisconnectLeash` / `UnleashAll` | take / give / unleash / dangling |
| `IsLeashed` / `GetLeashHolder` / `GetMaxLeashLength` | detect, pull heuristic, cache, eligibility |
| `LeashFramework_OnLeash` / `OnUnleash` | register in Actions; cache + narrate |
| `LeashFramework_OnActorPulled` | `OnLeashFrameworkPulled` (follow vs yank) |
| `LeashFramework_OnActorRagdollPulled` | `OnLeashFrameworkRagdollPulled` (yank + wrist `GetUpEnd`) |

Armor form IDs vs `Leash.esm`: wrists `0xD69` / `0x1` / `0x3`; neck/waist `0x804` / `0x806` / `0x2CE` / `0x800`. Holder-worn `0xD69` remains legacy `holder_shield`.

## Out of scope / open

- Calling `SetRagdollOverride` / `SetTeleportOverride` on apply
- VR parity (Framework added VR; this bridge is SE-only)
- FOMOD DLL version gate
- Git tag / GitHub Release / `make release`

## Playtest (in-game, Release SKSE)

Maintainer 2026-09-10: looked fine. No `SetRagdollOverride` / `SetTeleportOverride` on apply.

- [x] Actor-held middle: holder walks while collared NPC struggles → struggle continues; no “snaps taut”; follow ~240
- [x] tight / short / long: settle vs follow distinct; long does not collapse to hip range when stopped
- [x] Wrists meshes: tight/short `0xD69`, middle `0x1`, long `0x3`; type pulldown chain-only; distance swap re-equips
- [x] Hard yank / ragdoll → struggle ends; wrists rebind on `GetUpEnd`
- [x] World-tie past catch-up → struggle ends + taut line
- [x] Dead-body pull: no taut/struggle on a corpse
- [x] Apply / take / give / tie / unleash still succeed
