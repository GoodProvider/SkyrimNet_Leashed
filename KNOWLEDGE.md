# Knowledge

Standing facts for agents. Consult before game, script, or ESP changes. Append quirks after sessions.

## leashed_leash parent must not use cached nearby flags (2026-09-15)

Game Data Explorer showed **no eligible actors** for any start-leash child. The old `leashed_leash` category parent required `is_leash_available` plus `unleashed_nearby` OR not `speaker_is_leashed`. Ungating the parent alone was not enough: GDE’s eligible-actor list for a category is the **union of children that pass**.

**Cause:** those flags are `StateCache::Flagged`. Cache miss (actor not in the 750ms nearby snapshot) returns **false** → `"unavailable"`. Target/tie/refused-target children require `unleashed_nearby == available`, so they fail; the parent list stays empty. GDE evaluates every UUID-mapped actor; most are not in player + high/middleHigh process lists. Do not live-recompute inside `Flagged` (PublicAPI callbacks must be thread-safe).

**Fix:** `leashed_leashSubject.yaml`, `leashed_leashLeashed.yaml`, and all five `leashed_leash_*` children have no `eligibilityRules` (same as SexLab start actions). Papyrus still refuses bad targets at execute time. `leashed_change` / `leashed_escape` / `leashed_none_*` still use their own flags. Refresh Actions in GDE after YAML change.

## start-leash is two categories, not one (2026-09-15)

Direct narration `*bob leashes nina*` (Nina speaking) had a single `leashed_leash` parent eligible. Copy tried to cover both “you collar someone” and “someone collars you”; Gemma still treated it as speaker-as-holder and skipped `ACTION:`.

**Fix:** split into `leashed_leashSubject` (speaker collars someone) and `leashed_leashLeashed` (speaker is collared, including player/narration). Renaming the parent resets per-action enabled/cooldown keyed by `leashed_leash`. This does not override `0800` (“event already happened”) or `0750` (“only if you agreed”). Refresh Actions in GDE after YAML change.

## Dependency updates

When reviewing current code against a **dependency update**, follow the `dependency_drift` skill (`.cursor/skills/dependency_drift/SKILL.md`) and read the latest `checkpoints/<dependency-name>-<version>.md`.

| Dependency | Latest checkpoint |
| --- | --- |
| Leash Framework | [checkpoints/leashframework-1.1.4.md](checkpoints/leashframework-1.1.4.md) (no drift from 1.1.3; holder always given slack) |
| SkyrimNet | [checkpoints/skyrimnet-beta25-rc7.md](checkpoints/skyrimnet-beta25-rc7.md) (0.4.0; PublicAPI v10; content plugin `goodprovider.leashed`) |

CMake **Configure** may run `git submodule update --init --recursive` from `SKSE_Source` and reset `Skyrim-Leash-Framework` to the **parent gitlink**. After a pin, the index must record the new SHA (`git add Skyrim-Leash-Framework`) or Configure will walk it back. Do not compile or ship Framework `.pex` / DLL from the submodule.

Beta 25 does not read `prompts/`, `config/triggers/`, or `config/actions/`. Canonical LLM content is `SKSE/Plugins/SkyrimNet/external/goodprovider.leashed/` (`manifest.json` `id` must equal the folder name). The same actions and prompts are also copied to `config/actions/` and `prompts/` so pre-0.25 SkyrimNet still loads them (`tools/sync_legacy_skyrimnet_content.py`; do not edit those copies by hand). Prompt paths inside the plugin are unchanged (`prompts/leash_actions/…`, `0409_leashframework.prompt`). Action YAML filename (before `.yaml`) must equal the in-file `name` (case-insensitive); keep `name` casing. Settings schema stays at `config/plugins/SkyrimNet_Leashed/manifest.yaml` — that is not a content-plugin folder. Do not ship into `library/`. Upstream: SkyrimNet `docs/modding/MIGRATING_TO_BETA25.md`.

## IsEquipped is not Framework-bind ready (2026-09-15)

Direct narration `!nina leashes Bob` ran `leashed_leash_target` and Framework **accepted** the pair (`OnLeash` applied, `ApplyLeashToHand` true). No visible rope: `Unable to bind … found 0 child bone(s) containing 'Leash1' under 'NPC Spine2 [Spn2]'`. `TraceLeashBones` on the player found Neck/Spine1/Spine2 but **zero** nodes named `Leash`.

**Cause:** `WaitForLeashMesh` treated `IsEquipped` as ready. Papyrus can mark `Leash_neck` (0x804) worn before the collar NIF attaches `Leash1_*` children — beast/male ArmorAddon miss, first-person third-person armor 3D not loaded, or missing mesh. Framework returns true when the pair is accepted, not when the spline bound.

**Fix:** native `HasLeashBones` walks third-person 3D the same way as `TraceLeashBones`. After `EquipTypeArmor`, `QueueNiNodeUpdate` then poll `HasLeashBones` (~2s). If still none, do **not** call `ApplyLeashToHand` / `ApplyLeashAtPosition`; unequip, `ForgetPair`. If the mesh owner is the player and `Game.GetCameraState()` is 0 (first person), `Debug.Notification("You must be in third person view for the leash to work")`. Otherwise `NarrateMeshFailed`: same line as `Debug.Notification` and DirectNarration (Argonian scales / Khajiit fur / generic “will not stay on.”). Do not invent an Argonian collar mesh. Playtest: Nina (Nord) visual OK; Bob (Argonian) `bones=False` in third person.

**Fallback (2026-10-05):** a failed attempt no longer narrates by itself. `ApplyToHolder` / `ApplyToTiePoint` walk `LeashFallbacks(LeashIdFor(kind, bodyPart))` (most similar first, `holder_shield` last since the holder wears that mesh) and keep the first that binds. `NarrateMeshFailed` plays once, only when every candidate fails. Candidates whose mesh owner is the first-person player are skipped. Worst case is ~6 × the 2s bone poll.

## Devious Devices bound NPCs: leash invisible (2026-10-05)

Skadi (NPC, DD armbinder + blindfold) leashed by the player: Framework `Bound 13 leash bones`, narration fired, then 2s later `Unable to bind … found 0 child bone(s)`; rope invisible. The visible rope **is** the Leash.esm armor NIF on the leashed actor (Framework only drives its `Leash1_*` bones), so no armor 3D = invisible leash. Biped dump: `IsEquipped=TRUE` but no slot 45 entry.

**Cause:** DD NG `src/Hider.cpp` hooks `InitWornArmor`. Non-device armor on a non-player goes through `CheckNPCArmor`; with the default hider setting `sBoundNakedNPCs` (`zadDevicesUnderneathScript.Setting = 1`) it returns `!IsBound(actor)`, so a **bound NPC renders no normal armor at all**. It briefly showed only while DD restarted the heavy-bondage effect. Not a slot conflict (Leash neck 45, waist 58, hand chain 39; armbinder 46, blindfold 55), not timing, not Leash Framework.

- Exempt: the player; armor whose slot is in `DeviousDevices.ini [DeviceHider] aiHiderOverrideSlots` (checked before `CheckNPCArmor`); DD MCM NPC-naked mode off.
- User fix: `aiHiderOverrideSlots = 60, 45, 58` (45 = Leash.esm collars and neck ropes, 58 = waist rope `Leash_bodyAA`). The override matches the armor slot mask and skips both `CheckForceStrip` and `ProcessHider` for every actor, player included, so 58 also un-hides DD corsets and harnesses under body armor. Accepted trade-off (2026-10-05) so waist ropes show; the FOMOD warns about it. Without 58, a waist rope on a bound NPC falls back to a neck rope. Do not add 39 (shields). DD reads only `Data\SKSE\Plugins\DeviousDevices.ini`, once at startup, with no override files: restart the game.
- Optional FOMOD patch `Optional/DDNG_LeashCollars/`: a copy of the DD NG 0.4.3 ini with only that line changed. It replaces the user's DD ini, must win the MO2 conflict (SkyrimNet Leashed below DD NG), and drifts with DD updates. Re-copy it when DD NG's ini changes.
- Mod: `IsDeviousBoundNPC` (non-player wearing `zad_DeviousHeavyBondage`, keyword by EditorID). After a successful Framework apply on such a mesh owner, `LeashMeshSurvivesHider` waits 3s and re-checks `HasLeashBones`; gone -> disconnect, unequip, fall back. `ApplyToHolder` then skips every other candidate worn by that NPC and goes to `holder_shield` (holder-worn). Tie points skip fallbacks entirely. `NarrateMeshFailed` says "<holder> tries and fails to leash <leashed> with a <kind> leash." (back-out is `MarkSuppressed`, no OnUnleash line) and shows a one-time MessageBox with the ini fix.
- First person (2026-10-05 10:35 playtest): the `holder_shield` fallback was silently skipped because the holder was the first-person player, and `NarrateMeshFailed` was gated off. Now, when the DD hider ate the requested mesh and the next candidate is `holder_shield` on the first-person player, `ApplyToHolder` calls `Game.ForceThirdPerson()` (+0.5s) and tries it. A DD failure always narrates, even in first person.
- `TraceLeashBones` logs each occupied third-person biped slot (item, addon, 3D) — a missing slot entry with `IsEquipped=TRUE` means a hider skipped `InitWornArmor`.
- Build env (fixed 2026-10-05): `C:\Skyrim\dev\mods\SKSE64\Scripts\Source` had vanilla copies of 62 SKSE scripts (Form.psc without `RegisterForModEvent`, Actor, Game, Armor…; ArmorAddon, Input, FormType… missing). Restored from `SKSE64\Scripts old\Source`. If `compile: pyro` fails on `UnregisterForModEvent` again, check `Form.psc` is 10749 bytes, not 5763.

## Holder-less unleash: narration follows what is visible (2026-10-05)

A non-suppressed Framework disconnect with no holder (tied or dangling leash) reaches `NarrateUnleash` with `holder == None`. The leashed actor owns every holder-less mesh. If `HasLeashBones(leashed)` is true, the leash still shows: `ApplyDangling` keeps it on and records it as dangling, and the line is "The <kind> leash comes loose and now dangles from <X>'s <bodyPart>." Otherwise it is unequipped and forgotten, and the line is "The <kind> leash will not stay on <X>'s <bodyPart>, and <X> is no longer leashed." Explicit unleash, `ApplyDangling` and the DD back-out all `MarkSuppressed`, so they never reach this branch.

## Beast races: Leash.esm ARMAs exclude Argonian/Khajiit (2026-10-05)

All 7 Leash.esm ArmorAddons (`Leash_neckAA`, `Leash_neck_chainAA`, `Leash_neck_runicAA`, `Leash_bodyAA`, `Leash_hand_chainAA`, `_longAA`, `_xlongAA`) have race Default (0x19) plus human/elf races, but not `ArgonianRace` 013740, `ArgonianRaceVampire` 08883A, `KhajiitRace` 013745 or `KhajiitRaceVampire` 088845. A beast-race mesh owner gets `equipped=TRUE bones=False`, and its biped has no entry for the slot. This breaks the collar on a beast leashed actor and the `holder_shield` hand chain on a beast holder (10:52 playtest: Bob, Argonian player).

- Optional FOMOD plugin `SkyrimNet_Leashed_BeastRaces.esp` (ESL; masters Skyrim.esm, Leash.esm; source `Spriggit/SkyrimNet_Leashed_BeastRaces/`) overrides all 7 ARMAs and adds the 4 races. It uses the human meshes; fit on beast skeletons is not verified.
- The ARMA JSON is copied from `Spriggit.CLI convert-from-plugin` of Leash.esm. If Leash.esm changes its ARMAs, regenerate it (this patch overrides every field).

## Ragdoll pull: never send animation events to a knocked-down actor (2026-10-05)

11:10 playtest: after a load door, LF teleported Skadi before her 3D was ready, then started forced recovery (ragdoll pull). Struggle had just started, so `PlayStruggleAnim` sent `IdleNervous` + `DDChastityBeltStruggle01`, and `OnLeashFrameworkRagdollPulled` → `EndStruggle` → `StopStruggleAnim` sent `IdleForceDefaultState`, all while she was knocked down. LF never logged `Completed forced recovery` (or `timed out`), and she never got up. The two earlier ragdoll pulls with no struggle recovered normally.

- `SkyrimNet_Leashed_Native.IsKnockedDown(who)` (3D loaded and `IsInRagdollState()` or knock state != normal, the same test as LF's `ForcedRecoveryController`) gates `PlayStruggleAnim` and `StopStruggleAnim`. `KnockState(who)` is for traces (CommonLib order: 6 get-up, 7 down).
- Watchdog: `OnLeashFrameworkRagdollPulled` → `WatchDowned`. `TickDowned` runs on the shared 1s `OnUpdate` (`RefreshUpdates` covers struggle and watchdog). Stuck time only counts while 3D is loaded, still knocked down, and the holder is within `GetMaxLeashLength` (LF is not dragging). 8s → `SendAnimationEvent "BleedOutStop"` (bleedout to standing, as PAHE `pahcore.BleedOutStop`). 14s → `ResetStuckActor`, a port of PAH Diary of Mine "Fix unresponsive or invisible actor" (`DOM_Keys.SpecialReset`: Disable/Enable, SetAlpha 1, `IdleForceDefaultState`, `QueueNiNodeUpdate`) plus the wrist bind. Watches drop after 60s or on `ForgetPair`.

## VR: never force third person (2026-10-05)

`SkyrimNet_Leashed_Native.IsVR()` (`REL::Module::IsVR()`) gates the `Game.ForceThirdPerson()` in `ApplyToHolder`. On VR the `holder_shield` fallback for a DD-hidden mesh and a first-person player skips the camera switch, traces it, and falls through to the normal failure path (`NarrateMeshFailed`). `WaitForLeashMesh` shows a VR-specific notification. SE behaviour is unchanged.

## CommonLibSSE-NG v11 via vcpkg overlay port (2026-10-05)

`SKSE_Source/vcpkg-ports/commonlibsse-ng-fork` pins `alandtse/CommonLibSSE-NG` v11.0.0 (`94faaed`). The GitHub tarball leaves `extern/openvr` empty (submodule), so the portfile still fetches ValveSoftware/openvr into it; without it `BSVRInterface.h` fails on `openvr.h`. v11 has no `LICENSE` (use `COPYING.txt` + `EXCEPTIONS.md`) and needs `nlohmann-json`, `simpleini`, `toml11`. A clean configure builds CommonLib from source (~6 min). Leashed's source compiled unchanged. Runtime needs the Address Library matching the player's Skyrim version.

## Papyrus quirks

- Full unclip restores the pre-apply count of the **one** Leash.esm mesh we equipped. A copy they already had stays. `EquipItem` spares of that form are removed. Do not `RemoveItem` every leash type or the whole stack. Vanilla prisoner cuffs stay `RemoveItem` 1.

## SexLab TargetMenu overlay (2026-09-16)

SkyrimNet_SexLab concatenates `Data/SKSE/Plugins/SkyrimNet_SexLab/webui/TargetMenu/Actor/options/*.json` on save load. This repo ships `0700_leashed_panel.json` at that path (MO2 overlay): `type: papyrus`, `panel: leash`, `source: leash`, `scriptName: SkyrimNet_Leashed_Actions`. Since 2026-09-28 the TargetMenu **leash** row cascades in-menu like the other options, using SexLab's own `renderLeashPanelBody` (style / subject / leashed / status / action ❯). It no longer closes the menu to open this overlay. That panel's `leashFireStart` calls `LeashedToHolder` / `LeashedToTiePoint` / `GiveLeash` / `TakeLeash` / `UnleashTargetExecute` / `UnleashSpeakerExecute` directly, with fixed distance/type/body (`middle` / `rope` / `neck`). **Keep those signatures stable.** The old leaf files `0700_leashed.json` / `0700_unleashed.json` (→ `TM_OpenLeash` / `TM_OpenUnleash`) were removed. Those functions remain for the overlay. `OpenPanelFor` defers Show to the next SKSE task because SexLab runs Papyrus then Hide; do not `RegisterForSingleUpdate` for this (struggle already owns `OnUpdate`).
