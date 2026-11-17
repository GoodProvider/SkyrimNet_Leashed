# 01 — unleash removes the collar; new "let hang" verb

Repo / branch / baseline: SkyrimNet_Leashed `main` @ `f87e94c` (0.5.1). All changes are uncommitted. Plan: `C:\Users\bhuff\.claude\plans\right-now-there-is-memoized-pudding.md`.

Logs: none. The work came from reading code, so no `logs/` folder was created.

## Goal

After a panel **unleash**, the collar stayed on the actor and dangled. The user asked for two things:
- **unleash** takes the leash off completely.
- A new **let hang** verb gives the old dangling result: the collar stays on and nothing holds or ties the other end.

## Results

- [SkyrimNet_Leashed_Actions.psc:2510](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L2510) `NarrateUnleash`: the holder-less branch now needs `WearsLeashArmor` ([:490](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L490), new) as well as `HasLeashBones` before it calls `ApplyDangling`. With no record and no worn armor, it returns without narrating and writes the trace `NarrateUnleash late event ignored`.
- `UnleashTargetExecute` ([:2369](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L2369)) and `UnleashSpeakerExecute` ([:2425](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L2425)) now unequip, `ForgetPair` and `NotifyUnleash` before they narrate. The speaker version collects its lines first and narrates after `UnleashAll`.
- `LetLeashHang(subject, leashed)` ([:1970](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L1970)) is new:
  - A held or tied leash goes through `ApplyDangling` and becomes dangling.
  - Narration has three cases: "lets go of … and leaves it hanging", "takes … leash from H and leaves it hanging", and "unties … leaves it hanging from their <body>".
  - If the leash already dangles or the actor is not leashed, it does nothing and writes a trace.
- [WebUI.cpp:472](../../SKSE_Source/src/WebUI/WebUI.cpp#L472): the `let hang` action now calls `LetLeashHang`.
- Both panels show `let hang` after `unleash` for a leashed actor:
  - Magelight `views/panel/index.html`: `verbValues` and `actionCascade`.
  - `PrismaUI/.../panels/leashed.js`: the tree, `leashFireStart` and `leashSeedAction`.
- README panel table, three changelog `Unreleased` sections and the KNOWLEDGE.md "Holder-less unleash" note are updated.
- Builds:
  - `compile: papyrus` passed. It rebuilt all 3 `.pex` files, because `-nocache` rebuilds Native and PlayerAlias too.
  - The Release DLL built and was copied to `SKSE/Plugins/SkyrimNet_Leashed.dll`.
  - `make release` was not run.

## Decisions

- **Root cause is a hypothesis from reading code.** Explicit unleash calls `MarkSuppressed`, but suppression is only a 2.0 s real-time window (`NarrateSuppressWindow`). If the Framework's `OnUnleash` is handled after that window, three things line up:
  - The pair has already been forgotten, so there is no holder.
  - `HasLeashBones` still reads true, because the 3D has not refreshed since the unequip.
  - So `ApplyDangling` puts the collar back on.

  The fix checks the synchronous `IsEquipped` instead of relying on the time window.
- **No new LLM action** (the user chose this). LLMs can already drop a leash through give/holder None.
- **let hang does not end a struggle**, because the actor is still collared. This matches `GiveLeash` with no receiver.
- The `let hang` verb is always offered for a leashed actor, even when the leash already dangles. Papyrus does nothing in that case, so the panel needs no new `dangling` field.

## Not yet tested

Nothing has been tested in game.

## Next

1. In game, hold an NPC and use **unleash**. Expected: the collar is gone, there is no dangle line, and the panel offers `leash` / `leash to`. Repeat with an NPC tied to a point, with a wrist leash, and with the player unleashing themselves.
2. Use **let hang** on a held NPC, then on a tied NPC. Expected: the collar stays on, it dangles, there is one narration line, and the panel still shows the actor as leashed. Use it again on the dangling actor. Expected: nothing happens.
3. Repeat steps 1–2 from the SexLab Target Menu panel.
4. Check `Papyrus.0.log` for `NarrateUnleash late event ignored`. If it appears, the 2 s window hypothesis is confirmed. Capture the logs for the next session.
5. If the tests pass: commit. Release only when asked.
