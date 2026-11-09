# 02 — Wrist struggle event fixed; TargetMenu wrists no longer greyed out

Repo / branch / baseline: SkyrimNet_Leashed `main` @ `dc4baed`. All changes below are uncommitted, on top of the user's earlier uncommitted `CHANGELOG-player.md` edits. Supersedes [checkpoint-01.md](checkpoint-01.md).

Logs: none. The work needed no logs and `logs/` does not exist, so nothing was moved or copied.

## Goal

Point wrist struggles at a DD event that exists, and clear the test blocker: `wrists` was greyed out in the SexLab TargetMenu leash panel.

## Results

Built and compiled. Not tested in game.

- [SkyrimNet_Leashed_Actions.psc:1992](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L1992): `StruggleAnimEvent` returns `DDRegCuffsFrontStruggle02` for wrists.
- [leashed.js](../../PrismaUI/views/SkyrimNet_SexLab/panels/leashed.js): the body row now uses `ssListCascade`, with no disabled entries. Picking a part with no mesh for the current type sets the type to that part's first valid kind (wrists `chain`, waist `rope`). `leashListCascade` had no other callers and was removed. The type row is unchanged: picking a type invalid for the body part still resets the body to neck.
- Docs: [README.md:26](../../README.md#L26), [README.md:43](../../README.md#L43), [escape-current-summary.md:56](../../escape-current-summary.md#L56), `CHANGELOG.md` and `CHANGELOG-player.md` (new `Unreleased` sections at the top; the user's edits and historical lines are untouched). [KNOWLEDGE.md:147](../../KNOWLEDGE.md#L147) makes no grey-out claim, so it was left alone.
- `compile: papyrus` arguments: exit 0, 3 files. `SkyrimNet_Leashed_Actions.pex` written. `Native.pex` and `PlayerAlias.pex` also show as modified (same sizes, no source change) because the task recompiles everything.
- No SKSE build was needed.

## Decisions

- **`DDRegCuffsFrontStruggle02`**, for the reasons in checkpoint-01. If it looks wrong in game, 03 is a one-word swap.
- **Auto-switch the type rather than grey out the body part** (user's choice). This matches the Magelight panel's `lockWristType` ([index.html:263](../../Magelight/SkyrimNet_Leashed/views/panel/index.html#L263)).

## Findings

**Why wrists was greyed out**

- `leashUi.leashType` defaults to `rope`, and `LEASH_BODY_KINDS.wrists` is `['chain']`, so the body row disabled wrists until the type was `chain`. The grey-out was UI only: Papyrus already forces chain for wrists (`KindForBodyPart`, [Actions.psc:421](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L421)).
- The body row is still hidden on an already-leashed target unless the action is `tie to` ([leashed.js:344](../../PrismaUI/views/SkyrimNet_SexLab/panels/leashed.js#L344)). This is intended, but the test needs an unleashed NPC.

**How a struggle can be triggered**

- No hotkey or panel starts one. `StruggleExecute` is reached only from the LLM action `leashed_escape_struggle`.
- **Correction to checkpoint-01:** the action is `leashed_escape_struggle`, not `leash_escape_struggle`.
- The Right Shift `OnEscapeStruggle` hotkey described in `checkpoints/escape-checkpoint.md` no longer exists in `SKSE_Source`. The only hotkey is the `\` leash panel.

**Which mod folder is live**

- `C:\Skyrim\dev\mods\SkyrimNet Leashed` (with a space) is a separate, older install that still has the greying code.
- MO2 profile `SkyrimNet SexLab` enables this repo (`SkyrimNet_Leashed`). Profile `SkyrimNet_Leash` enables the old copy instead, so the fix will not appear there. All other profiles have this repo disabled.
- Profile `SkyrimNet SexLab` has both `Devious Devices for SE-AE-VR` and `Devious Devices NG` enabled, with `Pandora Output`.

## Not yet tested

- Everything in game.
- `leashed.js` has had no syntax check, because Node is not installed here. `ssListCascade`'s signature was checked against SexLab's [index.html:7061](../../../SkyrimNet_SexLab/PrismaUI/views/SkyrimNet_SexLab/index.html#L7061). If the panel fails to open, look here first.
- The `sae` console command below has not been tried on this install.
- Whether the current `Pandora Output` registers 02 (a text search of it was inconclusive).

## Next

Use MO2 profile `SkyrimNet SexLab`.

1. Clip check, no leash needed: open the console, click an NPC, run `sae DDRegCuffsFrontStruggle02`, then `sae IdleForceDefaultState` to stop. `sae DDRegCuffsFrontStruggle01` should do nothing. If 02 also does nothing, rebuild Pandora and retry.
2. Panel: open the TargetMenu on an unleashed NPC with type `rope`. `wrists` is clickable and picking it shows type `chain`; picking `waist` shows type `rope`. Start the wrist leash and confirm a wrist chain appears.
3. Struggle: get that NPC to pick `leashed_escape_struggle` through SkyrimNet dialogue (for example, tell them to try to pull free). `Papyrus.0.log` shows `PlayStruggleAnim <actor> DDRegCuffsFrontStruggle02`. The NPC plays the cuffs-front struggle, without visible stutter when `OnUpdate` re-sends it.
4. End of struggle: the NPC returns to the bound idle (`StopStruggleAnim` / `ApplyWristBind`).
5. No commit or release has been made; both wait for the user.
6. Reply to the reporter (left to the user).
