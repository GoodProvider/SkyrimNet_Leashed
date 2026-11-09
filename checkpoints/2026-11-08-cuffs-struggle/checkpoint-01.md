# 01 — Wrist struggle calls a DD event that does not exist

Repo / branch / baseline: SkyrimNet_Leashed `main` @ `dc4baed`. The working tree has the user's uncommitted `CHANGELOG-player.md` edits. This session changed no code.

Logs: none. The work needed no logs, and this is a new folder, so there was nothing to move.

## Goal

A user reported: "The DD integration suggests `DDRegCuffsFrontStruggle01` is used for wrists, but I only see `...Struggle02` and `...Struggle03` (checked DD classic and NG). Seems like Leashed should point to one of these instead?"

Assess the report and record the result. The fix itself is in [Next](#next) and has not been applied.

## Decisions

- **The report is correct.** Leashed should point at an event that exists.
- **Recommended replacement: `DDRegCuffsFrontStruggle02`.** It is the lowest-numbered clip that exists. It is also the smaller file (316,448 vs 525,392 bytes), which suggests a shorter clip but does not prove it. `OnUpdate` re-sends the event on each pass ([SkyrimNet_Leashed_Actions.psc:2230](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L2230)), so a shorter clip is less likely to be cut off.
- Neither clip has been viewed in game. If 02 looks wrong, 03 is a one-word swap.

## Findings

Mod paths are under `C:\Skyrim\dev\mods\`. `<anims>` is `meshes\actors\character\animations`.

**The event does not exist**

- [SkyrimNet_Leashed_Actions.psc:1992](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L1992) (`StruggleAnimEvent`) returns `DDRegCuffsFrontStruggle01` for `wrists`.
- Classic: `Devious Devices for SE-AE-VR\<anims>\DD\FNIS_DD_List.txt:172-173` registers only 02 and 03.
- NG: `Devious Devices NG\<anims>\DD\FNIS_DD_List.txt:178-179` registers only 02 and 03.
- No `.hkx` for `...Struggle01` exists in either mod, and no `FNIS*List*.txt` under `C:\Skyrim\dev\mods` registers it.
- The only other reference is Unforgiving Devices' `UDStruggle_DB_DD.json:24` (repo copy: [UDStruggle_DB_DD.json](../../UnforgivingDevices/skse/plugins/StorageUtilData/UD/Animations/UDStruggle_DB_DD.json#L24)). This is the probable source of the name, but it does not show the clip exists.
- Classic DD's `.psc` files do not mention `CuffsFront`, so DD offers no canonical choice between 02 and 03. The NG mod folder has no `Scripts\Source` to check.

**Effect today (from reading the code, not observed in game)**

- [PlayStruggleAnim](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L2074-L2077) sends `IdleNervous` when the actor is not moving, then the DD event. An unregistered event is ignored, so wrist struggles should only ever have shown `IdleNervous`. This is a missing animation, not a crash.

**Other struggle events are fine**

- `DDChastityBeltStruggle01` is registered in classic (line 175) and NG (line 181).
- `DDCollarStruggle01` is registered in NG only (line 199), which [README.md:28](../../README.md#L28) already documents.

**Where the wrist clips live**

- Classic ships `<anims>\DD\DDRegCuffsFrontStruggle02.hkx` and `03`.
- NG's list registers both, but NG's own `DD\` folder has neither file. NG only has copies under `<anims>\OpenAnimationReplacer\DD to OAR\100002\DD\_variants_ZazHornyA\`.
- So the wrist clip still needs the classic files, which matches [README.md:28](../../README.md#L28) ("classic Assets still covers wrists and waist").

## Not yet tested

- Everything in game: no build was made. The live `Papyrus.0.log` has no DD struggle trace to compare against.
- Whether 02 restarts visibly when `OnUpdate` re-sends it.
- Whether the Pandora or FNIS output on this install registers 02. A text search of `Pandora Output` found nothing, but that search is inconclusive on binary behavior files.

## Next

1. Change [SkyrimNet_Leashed_Actions.psc:1992](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc#L1992) to `DDRegCuffsFrontStruggle02`.
2. Run `compile: papyrus` with Skyrim closed (no `.pex` writes while it runs).
3. Update the current-state docs: [README.md:26](../../README.md#L26) and [escape-current-summary.md:56](../../escape-current-summary.md#L56).
4. Add a new `CHANGELOG.md` entry and leave historical lines 170 and 205 alone. Add a player-facing line to `CHANGELOG-player.md` without disturbing the user's uncommitted edits there.
5. Stop at build and compile. No release unless asked.
6. Test in game with the classic DD files enabled and FNIS/Pandora rebuilt:
   - Clip a wrist leash on an NPC and trigger `leash_escape_struggle`.
   - `Papyrus.0.log` shows `PlayStruggleAnim <actor> DDRegCuffsFrontStruggle02`.
   - The NPC plays the cuffs-front struggle, not just `IdleNervous`, without visible stutter from the re-send.
   - Ending the struggle returns to the bound idle (`StopStruggleAnim` / `ApplyWristBind`).
7. Reply to the reporter (left to the user).
