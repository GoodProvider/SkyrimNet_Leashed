# Checkpoint: Leash Framework 1.1.4

**Status:** Reviewed 2026-09-27 against `../Leash Framework` (`LeashFramework 1.1.4.7z`, meta.ini `1.1.4.0`). Not a git checkout. DLL FileVersion still reports **1.1.2**. Previous: [leashframework-1.1.3.md](leashframework-1.1.3.md).

## Drift

None. `Headers/LeashFramework.psc` differed from upstream only by the appended new native, and is now byte-identical to `Source/Scripts/LeashFramework.psc`. Mod events (`LeashFramework_OnLeash`, `_OnUnleash`, `_OnActorPulled`, `_OnActorRagdollPulled`), factions (`lf_leashed` 0xD6A, `lf_leasher` 0xD6B) and Leash.esm armor FormIDs we load are unchanged. The ESM also has keyword `lf_leash` 0xD6C, which is undocumented and unused by us.

## New native

`Bool SetPreventOverstretchOverride(Actor leashed, Int mode = -1)`: -1 follows LF's "Prevent holder overstretch" MCM, 0 lets the holder move freely, 1 stops the holder once the rope is taut (with LF's "Holder stretch allowance"). Actor-held leashes only. Lasts until the leash is replaced or disconnected, and is saved only with persistent leashes.

**Adopted:** `ApplyToHolder` always calls `SetPreventOverstretchOverride(leashed, 0)` (give slack) after a successful actor-held apply. There is no grip UI, LLM action, or plugin setting. The `speaker_holds_leash` decorator and `get_speaker_held_actors` payload are still registered but unused.

**Not adopted:** `SetRagdollOverride` / `SetTeleportOverride` stay unused (see 1.1.3).
