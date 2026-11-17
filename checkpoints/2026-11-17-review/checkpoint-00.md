# 00 — Code review of the uncommitted unleash / let hang / untracked-collar work

Repo / branch / baseline: SkyrimNet_Leashed `main` @ `f87e94c` (0.5.1). Everything is uncommitted: the work from [2026-11-14-unleash-let-hang 01–02](../2026-11-14-unleash-let-hang/02-checkpoint.md) plus this review's fixes. Plan: `C:\Users\bhuff\.claude\plans\do-code-review-squishy-lake.md`.

Logs: [logs/2026-11-17-10-16-20/](logs/2026-11-17-10-16-20/) (`MANIFEST.txt`, SkyrimNet_Leashed, LeashFramework, SkyrimNet_SexLab, Papyrus.0). That snapshot is from a 10:06–10:13 session with no leash activity. The 08:42–08:43 session cited below was read in place and is preserved in [../2026-11-14-unleash-let-hang/logs/2026-11-17-08-55-46/](../2026-11-14-unleash-let-hang/logs/2026-11-17-08-55-46/).

## Goal

Review the uncommitted diff, ask the open questions, and fix what the review turned up.

The diff covers:
- `SkyrimNet_Leashed_Actions.psc`
- `WebUI.cpp`
- the Magelight panel
- `leashed.js`
- the changelogs, KNOWLEDGE.md and README.md

## Findings

### In game (08:42–08:43 session, current 01+02 build)

- **Unleash:** [LeashFramework.log](../2026-11-14-unleash-let-hang/logs/2026-11-17-08-55-46/LeashFramework.log) shows Coralyn `29000D95` leashed by `00000014` at 08:42:47. SexLab dispatched `UnleashTargetExecute` (SexLab log line 506), and the Framework disconnected at 08:42:56.
- **Let hang:** Coralyn was leashed again at 08:43:06. SexLab dispatched `LetLeashHang` (SexLab log line 837). Papyrus logged `ApplyDangling leashed=Coralyn kind=rope body=neck` at 08:43:15.
- No errors. No `late event ignored` trace.
- So unleash and let hang have each worked once, on a held, tracked leash. The untracked DOM collar path has not been exercised.
- The 10:06 session in the new snapshot had 0 Framework leashes and no `[SkyrimNet_Leashed]` traces. It tells us nothing about this work.

### Review: fixed

1. **Late OnUnleash after let hang repeated the narration.**
   - `ApplyDangling` suppresses for only 2 s. A later `OnUnleash` reached `NarrateUnleash`, found a dangling record and worn armor, ran `ApplyDangling` again, and spoke "comes loose and now dangles" a second time.
   - The 01 late-event guard covered only unleash, where nothing is worn and nothing is recorded.
2. **Let hang did not end a struggle.** The watchdog ([psc ~L2296](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc)) keeps a recorded actor struggling, so a dangling NPC struggled until the max duration.
3. **Let hang narration on a tied leash:**
   - `tied` required `frameworkLeashed`, so a record cached as tied but already dropped by the Framework said "lets go of".
   - The tied line said "from their neck", and "their" reads as the subject.

### Review: not fixed (open)

- Native `LeashStatus` ([Bridge.cpp:211](../../SKSE_Source/src/Papyrus/Bridge.cpp#L211)) returns `world` for the plugin's own dangling record. The TargetMenu then shows "is tied to world" and offers `let hang`, which does nothing (it traces `already hangs`). A fix needs a DLL change: return `dangling` when there is no holder and the record is not tied.
- The Magelight hotkey panel and the decorators still don't see an untracked collar. This was already out of scope in 02.
- `WearsLeashArmorGlobal` repeats the id lists in `ActorWearsWristChain` / `DetectKind`. Cosmetic only.
- `LetLeashHang`'s `ApplyDangling` failure branch can never run. Harmless.
- `UnleashSpeakerExecute` has 32 line slots, which matches `CachedLeashed[32]`. OK.
- `SkyrimNet_Leashed_Native.pex` and `SkyrimNet_Leashed_PlayerAlias.pex` changed only because they were recompiled. Their sources are unchanged.

## Decisions (user answers)

- Late event: fix it by re-sending the native record without narrating.
- Let hang ends any struggle.
- TargetMenu `give to` / `tie to` on an untracked (DOM) collar adopts it. That is fine, because it is an explicit choice. Let hang still does not adopt it.
- The 08:42 test ran the current build, so it counts as a partial in-game pass.

## Results

- [SkyrimNet_Leashed_Actions.psc](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc):
  - `NarrateUnleash`, holder-less branch: when the actor is recorded with no cached holder, is not `CachedIsTied`, and is not `LeashFramework.IsLeashed`, it calls `NotifyLeash(None, leashed, CachedKind, CachedDistance, CachedBodyPart, false)` to undo the handler's `NotifyUnleash`, traces `NarrateUnleash late event kept dangling: X (reason)`, and returns.
  - `LetLeashHang`:
    - `EndStruggle(leashed, false)` before `ApplyDangling`.
    - `tied = holder == None`.
    - The tied line is now "… unties X's leash and leaves it hanging from X's <body>."
- The three changelogs and KNOWLEDGE.md ("Holder-less unleash") are updated.
- Build: `compile: papyrus` passed (3 files). `Actions.pex` was written at 10:17:24. No DLL change. `make release` was not run.

## Not yet tested

- Everything this review fixed.
- The untracked DOM collar path from 02 (status, unleash, let hang no-op, give/tie adopt).
- Unleash and let hang on a tied leash.
- Unleash on the player's own collar (`UnleashSpeakerExecute`).

## Next

1. Deploy the new `.pex` and `leashed.js`.
2. Hold Coralyn → TargetMenu → `let hang`. Expect exactly one narration line. Wait more than 5 s; there should be no second "comes loose" line. A `NarrateUnleash late event kept dangling` trace in Papyrus.0.log is fine.
3. Have a held NPC struggle, then `let hang`. Expect the struggle to stop.
4. Tie an NPC to a point → `let hang`. Expect "… unties Coralyn's leash and leaves it hanging from Coralyn's neck."
5. Coralyn with DOM's collar: expect the status `leash: dangling`. `let hang` should change nothing and trace `(untracked)`. `unleash` should remove the collar with one line. Note whether DOM puts the collar back on. Optionally, `give to` should adopt the collar.
6. Regression: unleash a held NPC, a tied NPC, and the player's own collar.
7. Capture logs. If the tests pass, commit. Release only when asked.
8. Decide whether to fix native `LeashStatus` returning `world` for a dangling record (DLL).
