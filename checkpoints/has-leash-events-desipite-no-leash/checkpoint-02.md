# Checkpoint 02: leash events with no visible leash

Repo `SkyrimNet_Leashed` / branch `main` / baseline `324e8e2`. Supersedes [checkpoint-01.md](checkpoint-01.md). Written 2026-11-01.

**Status: diagnosis only. No code changed, nothing built or tested in game.**

## Goal

Stop these symptoms from Faustus's report (full text in checkpoint-01):

1. The player is teleported back to the NPC after unequipping the leash item and running too far.
2. NPC thoughts mention a leash or collar when the player has no item and the player sees no active leash in the framework.
3. It persists across sessions ("permanent"). Clicking **unleash** in the UI appeared to clear it.

## Findings

The report describes two separate stale states. Each one fits some of the symptoms.

### A. Framework leash is active, but the mesh is gone (confirmed in the log)

Log: [LeashFramework.log](logs/2026-11-01-08-51-07/LeashFramework.log)

- L13-14: a persistent leash `0004D247->00000014` (the player is leashed) is restored from the save with reason `loaded`.
- L15: `Unable to bind ... found 0 child bone(s) containing 'Leash1' under 'NPC Spine1 [Spn1]'`. Spine1 means a waist leash, and `MeshOwnerFor` makes the leashed actor the mesh owner for that kind ([Actions.psc:344-356](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L344-L356), [:396-401](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L396-L401)). So the player's rope mesh was missing at that point.
- There is no `Bound ...` line anywhere in the log, which runs to 04:43. `Bound` is logged on every successful bind ([EquippedRope.cpp:46](../../Skyrim-Leash-Framework/src/Leash/EquippedRope.cpp#L46)). The rope appears never to have bound that session, though I did not read the retry path in `LeashInstance::Tick`.
- L16, L19, L48, L51: `Teleported follower 00000014 to leader 0004D247; reason=excessive distance`. This is symptom 1. The teleport check has no bind or mesh condition ([LeashTeleportController.cpp:105-148](../../Skyrim-Leash-Framework/src/Leash/LeashTeleportController.cpp#L105-L148)), so an invisible leash still teleports.

Why our mod does not catch it:

- Nothing watches for the leash armor being unequipped. There is no `OnObjectUnequipped` or `OnUnequipped` in `Scripts/Source`, and [PlayerAlias.psc](../../Scripts/Source/SkyrimNet_Leashed_PlayerAlias.psc) only handles `OnPlayerLoadGame`.
- The mesh is only checked when a leash is applied (`WaitForLeashMesh`). `OnLeashFrameworkLeash` pushes a `loaded` leash to native with no mesh check ([Actions.psc:2518-2526](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L2518-L2526)).
- The leash is applied with `persistent = true` ([Actions.psc:1532-1534](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L1532-L1534)), so Framework keeps restoring it on every load.

### B. Our records say leashed, but Framework has no leash (from reading code; not reproduced)

This is the closest match to symptoms 2 and 3: thoughts mention a leash, the framework shows none, and unleash clears it.

- `LeashState::IsLeashed` returns true if the actor is in `g_byLeashed`, even when not in the Framework faction ([LeashState.cpp:80-90](../../SKSE_Source/src/Leash/LeashState.cpp#L80-L90)).
- `StateCache::Refresh` builds its pairs from that check ([StateCache.cpp:261-284](../../SKSE_Source/src/SkyrimNet/StateCache.cpp#L261-L284)). The pairs feed the `leashframework_visible_pairs` decorator, and [0409_leashframework.prompt](../../SKSE/Plugins/SkyrimNet/external/goodprovider.leashed/prompts/submodules/character_bio/0409_leashframework.prompt) renders them for `full`, `thoughts` and `transform`. That is how a stale record reaches NPC thoughts.
- **Why it persists:** `g_byLeashed` is cleared on load ([plugin.cpp:35-43](../../SKSE_Source/src/plugin.cpp#L35-L43)), but `RepushCachedPairs` restores it from the saved Papyrus cache without asking Framework whether the leash still exists ([Actions.psc:85-103](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L85-L103)). A stale entry therefore comes back on every load.
- **Why unleash clears it:** `UnleashSpeakerExecute` wipes the cache and calls `NotifyUnleash` even when Framework has nothing to disconnect ([Actions.psc:2342-2348](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L2342-L2348), `UnequipTrackedFor` at [:1314-1339](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L1314-L1339)).

Candidate ways to end up in state B (none proven):

| # | Path | Where |
| --- | --- | --- |
| 1 | Dangling is Papyrus-only by design. If the player then unequips the rope, nothing clears the record, and thoughts keep saying "wears a dangling leash". | [Actions.psc:2398-2408](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L2398-L2408), [:1572-1595](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L1572-L1595) |
| 2 | A real `OnUnleash` inside the 2s suppress window is ignored: no `NotifyUnleash`, no `ForgetPair`. | [Actions.psc:2533](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L2533), [:2386-2388](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L2386-L2388) |
| 3 | A late `OnLeash` after the leash has ended: the handler does not check `IsLeashed`, so a `None` holder writes a `tied` record to native. Native-only, so it clears on the next load. | [Actions.psc:2518-2524](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L2518-L2524) |
| 4 | Framework drops a leash on load (`!IsValid` / `!FormsExist`) and sends no `OnUnleash`, so the Papyrus cache keeps it. | [LeashManager.cpp:623-626](../../Skyrim-Leash-Framework/src/Leash/LeashManager.cpp#L623-L626) |

### Open questions about the evidence

- **Log and report do not line up.** The report says the framework had no active leash, but this log shows one in use through 04:43, with no `Disconnected` or `Unleashed` line. Faustus's timestamps are 3:23-3:40 PM and the log runs 02:57-04:43 with no date. It may be a different session, or end before the unleash click.
- **Log owner is assumed.** I treated these as Faustus's logs because they were supplied in this folder and differ from the live logs on this machine. Nothing in them says so.
- **`0004D247` is unidentified.** Its name would help confirm the session.
- **Version drift.** The log shows LeashFramework **1.1.7** and SkyrimNet_Leashed **0.4.4** (PrismaUI). The submodule's CMake says 1.1.6, and [KNOWLEDGE.md](../../KNOWLEDGE.md) pins 1.1.4. All Framework line references above are from the submodule, so they may differ from what Faustus ran.

## Proposed fix (not implemented)

Estimated confidence: about 75% that this covers the report, because state B is not reproduced.

1. **Reconcile on load (state B).** In `RepushCachedPairs`, if `!LeashFramework.IsLeashed(leashed)`, keep the entry only if it is a dangling leash whose armor is still equipped. Otherwise `ForgetPair` it and do not push it. Tied entries need the same Framework check; do not drop tied leashes by mistake.
2. **Guard the late event (B3).** In `OnLeashFrameworkLeash`, skip `NotifyLeash` when `!LeashFramework.IsLeashed(leashed)`.
3. **Check the suppress window (B2).** After a suppressed `OnUnleash`, re-check `IsLeashed` and the cache so that a real disconnect is not lost.
4. **Handle a lost mesh (state A).** Add `OnObjectUnequipped` for tracked leash armor on the player, and a delayed mesh check after a `loaded` event. See decision 1 below.

Risks to design around:

- `HasLeashBones` is false when 3D is not loaded, in first-person cases, and for armor hidden by DD. A load-time check must not unleash in those cases (KNOWLEDGE: "IsEquipped is not Framework-bind ready", "Devious Devices bound NPCs").
- Our own `EquipTypeArmor` and `UnequipStaleBodyArmor` unequip leash armor during swaps, so an unequip handler must ignore them, possibly through `IsSuppressed`.
- `OnObjectUnequipped` on the player alias does not cover NPC mesh owners, including a `holder_shield` holder.

## Decisions for the maintainer

1. **What should unequipping a leash item do?** Options: end the leash (recommended; matches what the player expects, and fixes the teleport), re-equip it, or treat it as a struggle. This also affects whether a removal gets narration.
2. **What should happen when a leash loads with no mesh?** Options: re-equip the armor once and retry (recommended, then disconnect if it still fails), or disconnect straight away.

## Not yet tested

- Everything above. None of it has been reproduced in game.
- Whether a Framework leash binds again after its armor is unequipped and re-equipped.
- Whether `0409` is the only source of the "collar" wording. `collared_nearby` and event history in SkyrimNet could also produce it, so old narration may keep mentioning a collar after the records are clean.

## Logs

[logs/2026-11-01-08-51-07/](logs/2026-11-01-08-51-07/) with [MANIFEST.txt](logs/2026-11-01-08-51-07/MANIFEST.txt). It holds the two supplied logs, moved from `logs/`. No live logs were copied, since they are from this machine and not the reported session. There is no `Papyrus.0.log` from the reporter, so `[SkyrimNet_Leashed]` traces and the saved cache contents are not visible.

## Next

1. Ask Faustus: is this log from the session with the problem, which screen showed "no active leash", which leash type was on when it started, and who is `0004D247`? A `Papyrus.0.log` and a save that shows the bug would help most.
2. Maintainer answers the two decisions above.
3. Reproduce state A: leash the player, unequip the item from inventory, run past max length, and expect the `excessive distance` teleport. Save and reload, and look for `Unable to bind`.
4. Reproduce state B: put the player on a dangling leash, unequip the rope, save and reload, and check whether `0409` still lists the leash in `openrouter_input.log`.
5. Implement fixes 1-4 once the decisions are in, then run `compile: papyrus`. No SKSE change is expected so far.
6. Run the `dependency_drift` skill against Framework 1.1.7 and write `checkpoints/leashframework-1.1.7.md`.
