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
7. **Docs:** KNOWLEDGE.md (holder-less unleash, beast races, DD slot 45 and the ini patch, build env), CHANGELOG.md, CHANGELOG-developer.md, CHANGELOG-player.md and AGENTS.md.

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

---

# Handoff: CommonLibSSE-NG v11.0.0 migration (planned, nothing edited yet)

## Metadata
* **Date:** October 5, 2026
* **Branch:** `sexlab`. No files changed for this task yet.
* **Goal:** Build Leashed's SKSE DLL against CommonLibSSE-NG v11.0.0 (Skyrim 1.7.x / Address Library format 5), as `../SkyrimNet_SexLab` did in its commit `4ebea86`.
* **Decision (user):** keep the vcpkg workflow and bump the overlay port. Do not switch to a submodule.

## Findings
* **SexLab:** submodule `SKSE_Source/lib/CommonLibSSE-NG` (`https://github.com/alandtse/CommonLibSSE-NG.git`, branch `ng`), pinned to tag `v11.0.0` = `94faaed0c60eddd8347767f2d4d29a97c93bde8c`. It uses `add_subdirectory`.
* **Leashed:** `SKSE_Source/vcpkg.json` depends on `commonlibsse-ng-fork`, an overlay port in `SKSE_Source/vcpkg-ports/commonlibsse-ng-fork/` (see `vcpkg-configuration.json`). `portfile.cmake` pins `alandtse/CommonLibVR` at `e60c1238d558eb12f2d6f230605abbd568f7a76d`, which is an ancestor of v11.0.0 (406 commits behind). The port also copies openvr into `extern/openvr` and passes `-DSKSE_SUPPORT_XBYAK=on`.
* **SkyrimNet API:** Leashed's `include/SkyrimNet/PublicAPI.h` already matches rc4 (API v12). The untracked `include/SkyrimNet/PublicAPIDiaryQuery.h` is identical to SexLab's stand-in. No API work is needed.
* `VCPKG_ROOT` is `c:\dev\vcpkg`. Presets: `cmake --preset release`, then `cmake --build --preset build-release`, from `SKSE_Source/`.

## Steps
1. In `portfile.cmake`, set `REF` to `94faaed0c60eddd8347767f2d4d29a97c93bde8c`. Use repo `alandtse/CommonLibSSE-NG` if the old URL's redirect fails. Replace `SHA512` (set it to `0`, build, and copy the real hash from the error). Check v11 still has `extern/openvr` and `SKSE_SUPPORT_XBYAK`; adjust if not.
2. In the port's `vcpkg.json`, set `version-date` to 2026-10-04 and bump `port-version`. Sync dependencies with v11's own `vcpkg.json`.
3. Confirm v11's `cmake/CommonLibSSE.cmake` and install layout still match the portfile fixups (`lib/cmake`, `share/CommonLibSSE`).
4. Breaking-change scan: in SexLab's submodule checkout, run `git log --format=%B e60c123..v11.0.0 | grep -A8 'BREAKING CHANGE'`. Grep Leashed `SKSE_Source/src` for the named symbols. SexLab found none applicable (package, HitData, BSShaderAccumulator, BSGraphics::State, GetActiveEffectList, VR layouts).
5. Build, then fix drift in `src/Leash`, `src/Papyrus`, `src/WebUI`. Recheck `IsInRagdollState` and the knock-state enum order (KNOWLEDGE.md line 71: 6 = get-up, 7 = down).
6. Docs: CHANGELOG.md and KNOWLEDGE.md entries. Note that the plugin needs no runtime list, but players need the matching Address Library. CommonLib v5+ is GPL-3.0-or-later with the Skyrim Modding Exception.

## Constraints
* If Skyrim is running, do not write the DLL, `.pex`, ESP or PrismaUI HTML. Ask the user to quit first.
* SE is not VR. Do not claim VR support.
* Commit summary in the first 72 characters.

## Verify
* The release build succeeds and the DLL lands in `SKSE/Plugins/` and the MO2 mod folder.
* The plugin loads in the SKSE log on the user's runtime.
* In game, leash, knock-down, struggle and the PrismaUI overlay still work.
