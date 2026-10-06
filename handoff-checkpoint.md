# Handoff Checkpoint

## Metadata
* **Date:** October 5, 2026 (after implementing the post-10:52 plan)
* **Branch:** `sexlab`. All work is **uncommitted** and waits on a playtest.
* **Topic:** Leashing NPCs that wear Devious Devices, and beast-race holders/leashed actors.

## Done (not playtested)
1. **Build env fixed.** `C:\Skyrim\dev\mods\SKSE64\Scripts\Source` had vanilla copies of 62 SKSE scripts. They were restored from `SKSE64\Scripts old\Source`, and the 39 replaced vanilla files were backed up to the session scratchpad. `compile: pyro` succeeds with `skyrimse.ppj`.
2. **Holder-less unleash follows what's visible** (`NarrateUnleash` in `SkyrimNet_Leashed_Actions.psc`):
   * If `HasLeashBones(leashed)` is true: `ApplyDangling`, plus "…comes loose and now dangles from…".
   * Otherwise: unequip, `ForgetPair`, plus "…will not stay on …, and X is no longer leashed."
3. **Hint text:** slot `45` only. The MessageBox also points at the optional FOMOD patch.
4. **Optional DD ini patch:** `Optional/DDNG_LeashCollars/SKSE/Plugins/DeviousDevices.ini` (DD NG 0.4.3 copy, `aiHiderOverrideSlots = 60, 45, 58`; 58 = waist rope, un-hides DD corsets/harnesses).
5. **Optional beast-race ESP:** `SkyrimNet_Leashed_BeastRaces.esp` (ESL, masters Skyrim.esm and Leash.esm), with source in `Spriggit/SkyrimNet_Leashed_BeastRaces/`.
   * It overrides all 7 Leash.esm ARMAs and adds Argonian/Khajiit and their vampire races.
   * A Spriggit round-trip confirmed the header, masters and races.
6. **FOMOD:** an optional `SelectAny` step with both patches. `deserialize_esp.ps1` builds every `Spriggit/*` folder. `pack_release.ps1` stages and requires both optional files, and a test pack contains them.
7. **Docs:** KNOWLEDGE.md (holder-less unleash, beast races, DD slot 45 and the ini patch, build env), CHANGELOG.md, CHANGELOG-user.md and AGENTS.md.

## Next: playtest
Before testing, install the ini patch (MO2: SkyrimNet Leashed below DD NG) and the beast-race ESP, then restart the game.
* (a) Bob (Argonian) → Skadi (DD-bound): the neck rope stays visible past 10 s, with no `Unable to bind`.
* (b) Bob with `holder_shield`: the hand chain shows `bones=True` and is visible. Check how it fits on the Argonian body.
* (c) An unbound NPC: no extra 3 s wait.
* (d) A human NPC holder → a DD-bound NPC: the hand chain is visible.
* (e) A tied leash that the Framework disconnects: the mesh stays on, the "dangles" line fires, and the actor shows as dangling. On a DD-bound actor without the ini patch, the leash is removed and the "will not stay on" line fires.
* Then commit, with the summary in the first 72 characters.

## Logs
* Papyrus: `C:\Users\bhuff\OneDrive\Documents\my games\Skyrim Special Edition\Logs\Script\Papyrus.0.log`. Grep `[SkyrimNet_Leashed]` and `[Zad-NG]: OnEffect`.
* `…\SKSE\SkyrimNet_Leashed.log`: the TraceLeashBones slot dump.
* `…\SKSE\LeashFramework.log`: `Bound N leash bones` / `Unable to bind`.
