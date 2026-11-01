# Checkpoint 01: struggle keeps running through pulls and ragdoll, fix planned, nothing changed yet

Repo: `c:\Skyrim\dev\mods\SkyrimNet_Leashed` | Branch: `main` | Baseline: `39d1628` (Release 0.4.4), with the uncommitted Magelight port in the tree | Date: 2026-11-01

**Status: log review and plan only. No code has changed.** Logs are in [`logs/2026-11-01-08-19-30/`](logs/2026-11-01-08-19-30/MANIFEST.txt).

## Goal

A leashed actor being pulled or ragdolled must not be struggling. Struggle ends when a pull or ragdoll starts, and it can't start during one.

Reported in the 08:14 to 08:16 playtest: the struggle animation prevented the ragdoll.

## Findings

Times marked `LF` are from [LeashFramework.log](logs/2026-11-01-08-19-30/LeashFramework.log). `P:n` is a line in [Papyrus.0.log](logs/2026-11-01-08-19-30/Papyrus.0.log), which only has whole seconds.

| Time | Event | Source |
|---|---|---|
| 08:16:13 | `StruggleExecute Coralyn` | P:1160 |
| 13.385 | Pull starts at distance 225.1, below max, so struggle continues | LF:17 |
| 13.818 | Passes max by 0.75 | LF:18 |
| 08:16:14 | `PlayStruggleAnim` | P:1161 |
| 14.819 | First pull stops. No reason is logged | LF:19 |
| 14.835 | Second pull starts at 253.6, also below max, so struggle continues | LF:20 |
| 15.835 | Forced recovery starts at 761.6, which matches 2 x the default 380 max | LF:22 |
| 15.868 | `OnActorRagdollPulled` sent at 779.8 | LF:24 |
| 08:16:16 | `PlayStruggleAnim` **sends the idle after the ragdoll event** | P:1177 |
| 08:16:16 | Handler runs: `ragdoll pull Coralyn` | P:1179 |
| 08:16:25 | **`StruggleExecute` starts a new struggle during the drag** | P:1321 |
| 08:16:26 to :32 | 7 ticks of `skipped: knocked down`, each still sending "continues to struggle" | P:1399 to P:1419 |
| 08:16:33 | Ends only by reaching the 7 s cap | P:1421 |

### Confirmed

1. **Pull detection only looks at the distance when the pull starts.** `OnLeashFrameworkPulled` ends struggle only if `numArg >= maxLen` ([Actions.psc:2529](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L2529)). Both pulls started below max, and LF sends no further event as the distance grows. So struggle ran from 225 to 780 units. No `taut pull` line appears anywhere in the log.
2. **An animation event went out after the ragdoll started.** `PlayStruggleAnim` checks `IsKnockedDown` once, at the top ([Actions.psc:2010](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L2010)). `AnimationController`, `StruggleBodyPart` and `ActorIsLocomoting` all run before it sends. Nothing ends the struggle until the mod event handler runs, and the handler's first native log line is at 16.454, up to about 0.6 s after LF sent the event ([SkyrimNet_Leashed.log:29](logs/2026-11-01-08-19-30/SkyrimNet_Leashed.log)).
3. **Struggle can start while the actor is down.** `StruggleExecute` has no knockdown check ([Actions.psc:2179](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L2179)), and the action's eligibility rules don't test for it either. The animation gate held, but the LLM was told she was struggling while she was being dragged.
4. **The struggle loop never ends on knockdown.** The `OnUpdate` branch ([Actions.psc:2155](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L2155)) only skips the animation, and keeps sending pulses.

### Likely, not proven

5. **The idle may stop the pull from moving her.** `PullController` won't drive an actor while `IsAnimationDriven()` or `IsAllowRotation()` is true ([PullController.cpp:272](../../Skyrim-Leash-Framework/src/Leash/PullController.cpp#L272)), and struggle resends the idle every second. That would leave her standing while the holder walks away, reaching the ragdoll threshold sooner. After 14.835 the distance grows without closing, but a fast holder would look the same. PullDiag was off, so this needs a test (test 3).
6. **The late idle may be what looked wrong on screen.** The logs show LF did get a ragdoll: it only sends that event after `PullRagdoll` succeeds ([ForcedRecoveryController.cpp:341-354](../../Skyrim-Leash-Framework/src/Recovery/ForcedRecoveryController.cpp#L341-L354)). So "prevented" could mean delayed, or a pose fighting the ragdoll. Logs can't show which.

### Other observations

- **The playtest did not run the 08:02 repo DLL.** The log says `WebUI: PrismaUI API acquired` ([SkyrimNet_Leashed.log:24](logs/2026-11-01-08-19-30/SkyrimNet_Leashed.log)), and the Magelight build has no PrismaUI code. `C:\Skyrim\dev\mods\SkyrimNet Leashed` (with a space, DLL dated 10/26) is the likely copy, but that isn't confirmed. **The Magelight port is still untested too.**
- No forced recovery completion or timeout appears after 15.868, though drag ticks run until 08:16:41. She may have stayed in the pull, or the LF log may not have flushed.
- The earlier struggle at 08:15:42 had no LF pull activity and ran normally to the cap (P:1116 to P:1145).
- LF line references are to the in-repo `Skyrim-Leash-Framework` source. The game ran LF 1.1.7, and I did not check that the two match.

## Plan

All in [`SkyrimNet_Leashed_Actions.psc`](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc) unless noted.

1. **End struggle on every pull of the leashed actor.** In `OnLeashFrameworkPulled`, end it for any pull, and keep the `yank` test only for the taut narration. Call `StruggleStoppedEvent` when a struggle actually ended, since the LLM otherwise thinks it's still going. This reverses the 1.1.3 rule, see Decision below.
2. **End struggle on knockdown in `OnUpdate`.** Add an `IsKnockedDown` branch before the animation: trace, `EndStruggleQuiet`, `StruggleStoppedEvent`. This also covers engine ragdolls that send no LF event.
3. **Refuse to start.** `StruggleExecute` returns before any narration if the actor is knocked down or being pulled.
4. **Track pulls for step 3.** LF has no "is being pulled" query, so:
   - Register `LeashFramework_OnActorStopPull`, which isn't registered today.
   - Keep a session-only flag in native `LeashState`, next to `NotifyStruggle`. Clear it on stop-pull and unleash. It must not survive a save, because LF doesn't always send a stop event and a stale flag would block struggle for good.
5. **Hide the action from the LLM.** Add a `StateCache` flag and decorator for "down or being pulled", and add it to the rules in `external/goodprovider.leashed/actions/leashed_escape_struggle.yaml`. The cache lags, so step 3 remains the real gate.
6. **Narrow the send race.** Re-check `IsKnockedDown` immediately before the `SendAnimationEvent` calls in `PlayStruggleAnim`. Step 1 removes this case when a pull comes first. This covers a ragdoll with no pull before it. It can't fully close the gap from Papyrus; if test 2 still shows it, move the check and send into one native call.
7. **Docs.** Fix the comment at [Actions.psc:2527](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L2527), and update README's Escape section, KNOWLEDGE, and the `Unreleased` section of all three changelogs. No version bump or release.

### Decision: end struggle on every pull

Recommended, and it's what was asked for, but it has a visible cost. LF starts a "pull" as soon as an actor-held NPC begins following, before max length. So **an NPC could then only struggle while the holder stands still.** 1.1.3 tested and ticked the opposite: "holder walks while collared NPC struggles → struggle continues" ([leashframework-1.1.3.md:95](../leashframework-1.1.3.md)).

The alternative keeps struggling while walking, and ends it only once the distance passes max, checked each tick. It leaves finding 5 in place if the idle really does block the pull. Test 3 settles this.

## Not yet tested

Everything. Nothing has been built or compiled for this.

## Next

1. Enable the dev copy (`SkyrimNet_Leashed`) in the mod manager and disable `SkyrimNet Leashed`. Confirm the log shows Magelight rather than `PrismaUI API acquired`.
2. Implement the plan, then run `compile: papyrus` and the CMake SKSE build, with Skyrim closed.
3. In-game tests, on a neck leash with an NPC:
   1. **Stand still:** holder stands still and NPC struggles. Struggle runs to the cap, as at 08:15:42.
   2. **Sprint away:** struggle ends at the first pull, the stop event is sent, no `PlayStruggleAnim` follows, and she ragdolls and gets up.
   3. **Evidence for finding 5:** before the fix, or with step 1 temporarily off, turn on LF Debug > Diagnostics > Pull diagnostics and repeat test 2. Look for `waiting for direct locomotion to stabilize` with `animationDriven=true`.
   4. **During a drag:** the action isn't offered, and calling it logs `StruggleExecute skipped`.
   5. **Wrist leash:** after a ragdoll, the bind reapplies on `GetUpEnd`.
4. Save the logs from those runs, and start the next session from `checkpoint-02.md`.
