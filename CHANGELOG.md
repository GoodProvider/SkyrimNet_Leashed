# Changelog

## [0.4.2](https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/0.4.2) — since [0.4.1](https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/0.4.1)

### Papyrus

- Native `OpenPanelFor(Actor leashed, String layout, String verb)` on `SkyrimNet_Leashed_Native`. `TM_OpenLeash` / `TM_OpenUnleash` call it with `"vertical"` and `"leash"` / `"unleash"`.
- Leash fallback chain. `ApplyToHolder` / `ApplyToTiePoint` now return the used leash id (`""` on failure) and wrap `TryApplyToHolder` / `TryApplyToTiePoint`. Catalog ids `neck_rope` / `neck_chain` / `neck_magic` / `waist_rope` / `wrists_chain` / `holder_shield` (`LeashIdFor`, `LeashIdKind`, `LeashIdBody`); `LeashFallbacks(id)` orders the others most similar first (body part, material, leashed-worn, holder-worn). Mesh or Framework failure tries each in order; first-person mesh owners are skipped; tie points skip `holder_shield`. `NarrateMeshFailed` only after all fail. `LeashedToHolder` narrates the fallback kind/body.
- DD bound NPCs: `IsDeviousBoundNPC` (`zad_DeviousHeavyBondage`, non-player). DD NG's DeviceHider (`CheckNPCArmor`, `sBoundNakedNPCs`) renders no non-device armor on them, so the Leash.esm mesh vanishes after Framework binds. `LeashMeshSurvivesHider` re-checks bones 3s after apply and backs out; `ApplyToHolder` then skips candidates worn by that NPC and goes to `holder_shield`; tie points skip fallbacks. `NarrateMeshFailed` DD variant (`<holder> tries and fails to leash <leashed> with a <kind> leash.`; the back-out `MarkSuppressed`s so OnUnleash stays quiet) plus a one-time MessageBox pointing at `aiHiderOverrideSlots`. When the only DD-safe candidate is `holder_shield` on a first-person player, `ApplyToHolder` forces third person first; DD failures narrate even in first person.
- VR: native `IsVR()`. `ApplyToHolder` no longer calls `Game.ForceThirdPerson()` on VR; the leash fails (`NarrateMeshFailed`) instead, and `WaitForLeashMesh` shows a VR-specific notification.
- Hider hint (MessageBox + traces) now says slot `45` only (58 is DD's corset slot, 39 shields) and points at the optional FOMOD ini patch.
- `NarrateUnleash` holder-less branch follows what the player sees. If `HasLeashBones(leashed)` is true, `ApplyDangling` keeps the mesh on and the line is "The <kind> leash comes loose and now dangles from <X>'s <bodyPart>." Otherwise it unequips, `ForgetPair`s, and the line is "The <kind> leash will not stay on <X>'s <bodyPart>, and <X> is no longer leashed." This replaces "fails to hold and falls off".
- Stuck ragdoll fix. Native `IsKnockedDown(Actor)` / `KnockState(Actor)`. `PlayStruggleAnim` / `StopStruggleAnim` send no animation events to a ragdolled or knocked-down actor (that stopped LF forced recovery from ever getting the actor up). `OnLeashFrameworkRagdollPulled` starts a watchdog (`WatchDowned` / `TickDowned` on the 1s update, now `RefreshUpdates`): stuck 8s inside leash range → `BleedOutStop`; 14s → `ResetStuckActor` (PAH Diary of Mine SpecialReset port: Disable/Enable, SetAlpha, `IdleForceDefaultState`, `QueueNiNodeUpdate`).
- `NormalizeDistance` accepts `tight_length` / `short_length` / `middle_length` / `medium_length` / `long_length`. `SpokenDistance` appends ` length` in `NarrateApply` and `NarrateLeash`.
- DD hider checks are per leash. `DDHidesArmor` / `DDHidesLeash` skip a candidate only when its slot is not in `aiHiderOverrideSlots`, so with the optional ini patch, neck and waist leashes are tried on DD-bound NPCs instead of going straight to `holder_shield` (holders) or failing (tie points). The DD MessageBox and wording only show when the hider hides the requested leash. Retying a `holder_shield` leash to a tie point uses `neck_chain` (and its fallbacks) instead of equipping the holder's hand chain on the leashed actor. `WaitForLeashMesh` traces an unequipped leash armor separately.
- Leash Framework refusal (`TryApplyToHolder` sets `FrameworkRefused`) stops the `ApplyToHolder` fallback loop and skips `NarrateMeshFailed`; it is not a mesh failure.
- Stuck-knockdown watchdog: `ResetStuckActor` never `Disable`/`Enable`s the player, and `TickDowned` does not count time while `Paralysis > 0`.
- `String LeashStatus(Actor leashed) Global` for the SexLab Target Menu panel: `""` not leashed, `"world"` tied to a point, `"<holderFormId>|<holderName>"` held. When `LeashFramework.IsLeashed` is false it falls back to native `SkyrimNet_Leashed_Native.LeashStatus` (`LeashState::IsLeashed` = leashed faction or recorded pair, holder from `LeashState::GetLeashHolder`), the same check the hotkey panel uses, so `unleash` is offered for every leashed target.

### SKSE / WebUI

- Build against MinLL/CommonLibVR 4.39.5 (MIT, SE 1.7.99 / 1.7.104) instead of CommonLibVR e60c123. The `commonlibsse-ng-fork` overlay port is now a copy of SkyrimNet_SexLab's. No source changes were needed; `IsInRagdollState` and the knock-state order (6 get-up, 7 down) are unchanged. Players need the matching Address Library.
- `TraceLeashBones` also logs each occupied third-person biped slot (item, addon, 3D node) to diagnose hidden collars.
- `WebUI::OpenFor` sends payload `layout`, `presetLeashed`, and `presetVerb`. `Show` runs on the next SKSE task so a caller that hides another overlay can Unfocus first, and consumes the pending open request before its early returns so a blocked open cannot leak its preset into a later one.
- SexLab Target Menu overlay: one **leash** option, `SKSE/Plugins/SkyrimNet_SexLab/webui/TargetMenu/Actor/options/0700_leashed_panel.json` (`panel: leash`, `panelScript: panels/leashed.js`, `requiresPlugin: SkyrimNet_Leashed.esp`, executes `LeashedToHolder`). `PrismaUI/views/SkyrimNet_SexLab/panels/leashed.js` renders inside SexLab's view with its cascade pulldowns: subject / leashed / status / action (`unleash`, `tie to` ❯ point, `give to` ❯ holder) plus distance / type / body for a new leash or a tie-to. Type/body pairs without a Leash.esm mesh are disabled (wrists = chain, waist = rope). Status comes from `papyrusQuery` → `LeashStatus`, re-queried on each open for the Target Menu's actor (not the previous pick) and whenever the leashed actor changes. While that query is pending, Start and new-leash type/body rows stay off so a collared target cannot be re-leashed before **unleash** appears.
- Decorators `speaker_holds_leash` (flag) and `get_speaker_held_actors` (payload): actors whose leash the speaker holds in hand (not tied or dangling).
- Native `DDHiderOverridesSlotMask(Int slotMask)`: reads `Data/SKSE/Plugins/DeviousDevices.ini` `[DeviceHider] aiHiderOverrideSlots` once and logs the slots.

### Install / FOMOD

- New optional install step (`SelectAny`):
  - **Devious Devices NG: show leash collars and ropes on bound NPCs** installs `Optional/DDNG_LeashCollars/SKSE/Plugins/DeviousDevices.ini`, the DD NG 0.4.3 ini with `aiHiderOverrideSlots = 60, 45, 58` (58 = waist rope; the FOMOD warns that it also stops DD hiding corsets and harnesses under armor). It replaces the DD ini and must win the MO2 conflict.
  - **Beast-race leash meshes** installs `SkyrimNet_Leashed_BeastRaces.esp` (ESL, masters Skyrim.esm and Leash.esm). It overrides all 7 Leash.esm ArmorAddons and adds ArgonianRace, ArgonianRaceVampire, KhajiitRace and KhajiitRaceVampire. Source: `Spriggit/SkyrimNet_Leashed_BeastRaces/`.
- `deserialize_esp.ps1` builds every `Spriggit/*` folder (ModKey from `spriggit-meta.json`). `pack_release.ps1` stages and requires the beast-race ESP and copies the DD ini source `SKSE/Plugins/DeviousDevices.ini` into `Optional/DDNG_LeashCollars/`.
- `LICENSE`: MIT.

### Escape / HUD

- PrismaUI leash panel (`PrismaUI/views/SkyrimNet_Leashed/index.html`) is now a cascade matching SexLab's Target Menu instead of the horizontal sentence bar (the 0.4.1 `fitBar` sizing is gone). [Start] + style on top, then subject / leashed / status / action / distance / type / body rows. Each field opens columns to the right (hover drill, ❯ branches, ‹ back, Escape peels a column, outside click closes). Action is a verb → object tree (`leash` ❯ holder, `leash to` ❯ tie point; when leashed: `unleash`, `tie to` ❯, `give to` ❯); a leaf starts at once. Distance / type / body only show for a new leash. The `layout` payload field is ignored (always vertical).

### Docs

- Dual-ship `0409_leashframework.prompt`: distance tokens render as `{{ pair.distance }} length`.
- Player README is SFW: dropped `images/SkyrimNet_Leashed_small.png`; LoversLab Leash Framework and Devious Devices links labeled NSFW.
- `README.md` leash panel and SexLab Target Menu sections describe the cascade panel and `0700_leashed_panel.json`; Address Library must match the game version.

## [0.4.1](https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/0.4.1) — since [0.4.0](https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/0.4.0)

### Escape / HUD

- PrismaUI leash bar (`PrismaUI/views/SkyrimNet_Leashed/index.html`): `#bar` stays `width: max-content` with `max-width: calc(100% - 10em)` (5em each side). Pulldown columns `flex: 1 1 auto; min-width: 0`; `.col.word` and `.col.action` (Start) stay `flex: 0 0 auto`.
- Closed pulldown values and column labels ellipsize. `fitBar()` equal-shares leftover width when the natural row exceeds the cap; `resize` re-runs it. Open menus stay `width: max-content` with `max-width: calc(100vw - 10em)` so they do not widen the bar.

### Docs

- Player front door: `README.md` leash panel notes the 5em side gap and shrinking names.

## [0.4.0](https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/0.4.0) — since [0.3.0](https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/0.3.0)

### Actions

- Dual-ship LLM actions and prompts: canonical Beta 25 plugin `goodprovider.leashed` ([`SKSE/Plugins/SkyrimNet/external/goodprovider.leashed/`](SKSE/Plugins/SkyrimNet/external/goodprovider.leashed/)) plus pre-0.25 copies at `config/actions/` and `prompts/`.
- Action YAML `name`, `customCategory`, and filename prefix `leash_` → `leashed_` (e.g. `leashed_leashSubject`, `leashed_change`, `leashed_escape`, `leashed_none_target_unleash`). Category parents drop the trailing `_` in the filename so it equals `name`. Prompt paths `leash_actions/` and `0409_leashframework.prompt` are unchanged.
- Start-leash is two categories: `leashed_leashSubject` (speaker collars someone) and `leashed_leashLeashed` (someone collars the speaker). There is no `leashed_leash.yaml`.
- `leashed_leashSubject`, `leashed_leashLeashed`, and the five `leashed_leash_*` children have no `eligibilityRules` (Papyrus still refuses bad targets).
- Refused-speaker/target prompts: use only when the actor can stop the collar, not merely object.
- Per-action enabled/cooldown settings keyed by the old names reset once.

### Papyrus

- Unclip restores the pre-apply `GetItemCount` of the worn Leash.esm mesh (`CachedArmorCounts`). A copy they already had stays; extras `EquipItem` spawned of that form are removed. Panel cache-miss still unequips the detected worn form. Take/give do not strip the collar.
- Native `HasLeashBones` walks third-person 3D the same way as `TraceLeashBones`. After `EquipTypeArmor`, `WaitForLeashMesh` `QueueNiNodeUpdate` then polls `HasLeashBones` (~2s). Ready is bones, not `IsEquipped`.
- If still none: skip `ApplyLeashToHand` / `ApplyLeashAtPosition`, unequip, `ForgetPair`. Player mesh owner in first person (`GetCameraState() == 0`): `Debug.Notification("You must be in third person view for the leash to work")`. Else `NarrateMeshFailed` (Argonian scales / Khajiit fur / generic “will not stay on.”).

### SKSE / WebUI

- Settings schema stays at [`config/plugins/SkyrimNet_Leashed/manifest.yaml`](SKSE/Plugins/SkyrimNet/config/plugins/SkyrimNet_Leashed/manifest.yaml) (`plugin.name` SkyrimNet_Leashed, `leash.*` keys). C++ still calls `PublicGetPluginConfigValue("SkyrimNet_Leashed", …)`. PublicAPI pin remains v10.
- Registers Papyrus `HasLeashBones` on `SkyrimNet_Leashed_Native`.
- CMake Release post-build `copy_directory` of `SKSE/Plugins/SkyrimNet` (settings + external plugin). DLL FileVersion **0.4.0**.

### Install / FOMOD

- Ships both layouts. SkyrimNet 0.25+ reads plugin `goodprovider.leashed` (External badge). Older SkyrimNet reads `config/actions/` and `prompts/`. Do not use **Plugins > Import Old Content** for this mod's files.

### Docs

- Player front door: `README.md` (both SkyrimNet layouts, `leashed_*` action names, leash-bone apply). SkyrimNet pin: `checkpoints/skyrimnet-beta25-rc7.md`. Settings vs content plugin split and `IsEquipped` vs bones: `KNOWLEDGE.md`.

## [0.3.0](https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/0.3.0) — since [0.2.0](https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/0.2.0)

### Papyrus

- Compile header [`Headers/LeashFramework.psc`](Headers/LeashFramework.psc) matches Leash Framework **1.1.3** comments plus `SetRagdollOverride` / `SetTeleportOverride` (declared, not called). Submodule `Skyrim-Leash-Framework` stays `bf3b33c`.
- `OnLeashFrameworkPulled`: actor-held follow-start (`holder` set and `numArg < GetMaxLeashLength`) does not `EndStruggle` or taut-narrate. Yank is world-tie, `numArg >= GetMaxLeashLength`, ragdoll, unclip, or `leash_none_struggle_stop`. Dead senders are ignored.
- Native `DistanceMin` (settle / minLength). Apply uses per-token settle plus catch-up (`DistanceMax`).
- Wrists stays chain-only. Distance picks `Leash_hand_chain` `0xD69` (tight/short), `Leash_hand_chain_long` `0x1` (middle), `Leash_hand_chain_xlong` `0x3` (long); missing forms fall back to `0xD69`.
- `PlayStruggleAnim` sends `IdleNervous` when the actor is not locomoting, then the DD FNIS event from `DetectBodyPart` (not the Papyrus pair cache): neck `DDCollarStruggle01`, wrists `DDRegCuffsFrontStruggle01`, waist `DDChastityBeltStruggle01`. `StopStruggleAnim` still `IdleForceDefaultState` and rebinds wrists from `DetectBodyPart`.
- `UnequipTypeArmor` unequips the worn copy then `RemoveItem` 1 (the copy added for wear). It does not drain remaining player-owned stacks. `EquipTypeArmor` strips extras `EquipItem` can spawn.
- Mod event `SkyrimNet_Leashed_OpenPanel` calls native `OpenPanel`.

### SKSE / WebUI

- Natives `DistanceMin` and `OpenPanel`. `OpenPanel` → `WebUI::Open()` / `Show()` and does not require `leash.controls.hotkeyEnabled`.
- Manifest settle keys `leash.settle.*` (minLength) and catch-up `leash.distance.*` (maxLength). Defaults: tight 50/120, short 90/220, middle 150/380, long 240/580. C++ clamps settle ≤ catch-up. Actor-held follow gap is Framework 40% from settle to catch-up.
- SkyrimNet `PublicAPI.h` pin v10 plus `PublicAPIMemoryQuery.h`; this plugin does not call the memory-query API.
- Compile headers that must stay in git: `SKSE_Source/src/Papyrus/Bridge.h`, `SKSE_Source/lib/PrismaUI/PrismaUI_API.h`, `SKSE_Source/include/SkyrimNet/PublicAPIMemoryQuery.h`. CRT stays `/MD` (`x64-windows-static-md`).

### Escape / HUD

- Actor-held follow-start `OnActorPulled` (Leash Framework 1.1.3, before maxLength) does not end struggle. Walking skips `IdleNervous` so follow-walk is not cancelled every `RegisterForUpdate(1.0)`. World-tie, catch-up yank, ragdoll, unclip, or `leash_none_struggle_stop` still end it.
- PrismaUI **unleash** stays full power, including the player unclipping themselves. Other plugins (e.g. SkyrimNet_SexLab) can fire `SkyrimNet_Leashed_OpenPanel`.

### Install / FOMOD

- FOMOD still refuses unless `Leash.esm` is active. Tested with Leash Framework **1.1.3**.

### Docs

- Player front door: `README.md` (settle / catch-up table, wrist meshes, SexLab panel button). Quirks / pins: `KNOWLEDGE.md`. LF 1.1.3 drift: `checkpoints/leashframework-1.1.3.md`. SkyrimNet beta25-rc6 pin: `checkpoints/skyrimnet-beta25-rc6.md`. Dependency updates: skill `dependency_drift`.

## [0.2.0](https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/0.2.0) — since [0.1.0](https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/0.1.0)

### Actions

- Renamed `questEditorId` / `scriptName` in all 14 executable action YAMLs to `SkyrimNet_Leashed` / `SkyrimNet_Leashed_Actions`. Action names, `leash.*` config keys, and decorator IDs are unchanged.
- `leash_escape_.yaml` and `leash_escape_struggle.yaml` share gates: `is_struggle_enabled`, `speaker_is_leashed`, and not `speaker_is_struggling`. Category is not stricter than the child.
- Root `leash_none_struggle_stop` stays gated only on `speaker_is_struggling` (no `customCategory`). Root Unleash remains `leash_none_target_unleash`.

### Papyrus

- Scripts renamed `SkyrimNet_Leashed_Actions` / `_PlayerAlias` / `_Native`.
- `leash_escape_struggle` is a looping struggle state (`IdleNervous` while standing, then optional DD FNIS events by cached body part: neck `DDCollarStruggle01`, wrists `DDRegCuffsFrontStruggle01`, waist `DDChastityBeltStruggle01`). No ESM check; unregistered events stay on `IdleNervous`. Walking does not end it; yank, unclip, or `leash_none_struggle_stop` does.
- `StruggleExecute` no-ops when `StruggleEnabled` is off; `OnUpdate` silently `EndStruggle`s anyone already looping.

### SKSE / WebUI

- Plugin identity renamed from `SkyrimNet_Leash` to `SkyrimNet_Leashed`: `SkyrimNet_Leashed.dll` / `.log`, `PrismaUI/views/SkyrimNet_Leashed/`, manifest `plugin.name`. ESP EditorID `SkyrimNet_Leashed`, FormID `000800` unchanged.
- SkyrimNet plugin menu **Enable struggle** (`leash.escape.enabled`, default on) → decorator `is_struggle_enabled`.

### Escape / HUD

- While struggling, a short-lived `leash` event (`{name} continues to struggle with {her/his/their} leash.`) refreshes every second. DirectNarration still-beats wait for the longer of `leash.escape.narrationInterval` (default 5) and `leash.escape.cooldown` (default 20): `Despite {name}'s attempts, the leash holds.`
- Dropped ZaZ body-part clips and the 20s attempt-count window.

### Install / FOMOD

- Packaging renamed: FOMOD `SkyrimNet Leashed`, `versions/SkyrimNet_Leashed ${VERSION}.7z`, CMake project `SkyrimNet_Leashed`, MO2 deploy folder `SkyrimNet_Leashed`. Existing `SkyrimNet_Leash` installs must be removed rather than updated in place, or the old ESP and DLL keep loading alongside the new ones.

## [0.1.0](https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/0.1.0)

First public SkyrimNet ↔ Leash Framework bridge.

### Actions

- Categories `leash_leash`, `leash_change`, and `leash_escape` each carry a dynamic `intent` param (no execute function on the parent).
- Root action `leash_none_target_unleash` (`UnleashTargetExecute`): speaker unclips a nearby collared actor, never themselves. No `customCategory`. Eligibility: `collared_nearby`.
- Removed the `leash_unleash` category (`leash_unleash_speaker`, `leash_unleash_target`). Self-unclip is not an LLM action.
- `leash_leash`: `leash_leash_target` / `leash_leash_speaker` (`LeashedToHolder`), `leash_leash_tie_target` (`LeashedToTiePoint`), `leash_leash_refused_speaker` / `leash_leash_refused_target` (`LeashedRefused`). Parent gates `is_leash_available` plus `unleashed_nearby` or not `speaker_is_leashed`.
- `leash_change`: `leash_change_take_*` (`TakeLeash`), `leash_change_give_*` (`GiveLeash`), `leash_change_tie_*` (`LeashedToTiePoint`). Parent gates `is_leash_available` plus `collared_nearby` or `speaker_is_leashed`.
- `leash_escape_struggle` (`StruggleExecute`): collared speaker only (`speaker_is_leashed` and not already struggling). The leash does not come off.
- `leash_none_struggle_stop` (`StopStruggleExecute`): collared speaker already in the struggle idle (`speaker_is_struggling`). Root action; no `customCategory`.
- Tokens: style `forcefully|normally|gently`; distance `tight|short|middle|long`; type `chain|rope|magic`; body part `neck|wrists|waist`; tie point `floor|left|back|front|right|wall`. Holder and give-receiver may be `None` (dangling, not world-tied).
- Wrists is chain only. `leash_leash_target` / `leash_leash_speaker` / `leash_leash_tie_target` `leashType` copy says so; `body_part` copy says wrists uses the chain hand mesh. Leash.esm only ships `Leash_hand_chain` (`0xD69`).
- DirectNarration prompts under `SKSE/Plugins/SkyrimNet/prompts/leash_actions/`. Bio overlay `0409_leashframework.prompt` lists visible pairs via `leashframework_visible_pairs`.

### Papyrus

- `SkyrimNet_Leash_Actions`: apply, take, give, tie, refuse, struggle, stop struggling, and unleash execute; Framework event hooks; pair cache; type-armor equip; narration importance 1 / 2 / 3.
- `KindForBodyPart` forces `chain` when the body part is `wrists`. `ArmorForKind` uses `Leash.esm` `0xD69` for wrists (and for legacy `holder_shield`).
- `ApplyWristBind` / `ClearWristBind`: prisoner cuffs `Skyrim.esm` `0x10E039`, bound-standing idle `0x109837` / cut idle `0x109B6A`, fallbacks `OffsetBoundStandingPlayerInstant` / `BoundStandingCut`. `OnLeashFrameworkRagdollPulled` registers `GetUpEnd` to re-apply the wrist bind.
- `UnleashTargetExecute` rejects speaker-as-leashed. `UnleashSpeakerExecute` remains for the PrismaUI panel (including the player unclipping themselves).
- `StruggleExecute`: enter a per-actor struggle state and loop `IdleNervous` while standing. First line is DirectNarration. Later beats every `leash.escape.narrationInterval` seconds are optional DirectNarration. Walking does not end the state. A leash yank, unclip, or `StopStruggleExecute` does.
- `SkyrimNet_Leash_PlayerAlias` (`OnPlayerLoadGame`): re-registers Framework events and `RepushCachedPairs` into the SKSE cache.
- `SkyrimNet_Leash_Native`: `NotifyLeash`, `NotifyUnleash`, `NotifyStruggle`, `NormalizeToken`, `DistanceMax`, `DistanceFromLength`, `StruggleNarrationInterval`, `StruggleCooldown`, `TraceLeashBones`.

### SKSE / WebUI

- Flag decorators: `is_leash_available`, `is_unleash_available`, `speaker_on_leash`, `leashed_nearby`, `unleashed_nearby`, `speaker_is_leashed`, `collared_nearby`, `speaker_is_struggling`. `is_unleash_available` and `speaker_on_leash` stay registered; in-tree YAML no longer consumes them.
- JSON payloads: `get_nearby_unleashed_actors`, `get_nearby_leashed_actors`, `get_speaker_leash_partners`, `leashframework_visible_pairs`, `get_nearby_collared_actors`, `get_nearby_actors`.
- PrismaUI leash panel (`PrismaUI/views/SkyrimNet_Leash/`): hotkey `leash.controls.hotkey` default VK 220 (`\`). Panel **unleash** dispatches `UnleashSpeakerExecute` when the subject is the leashed actor, else `UnleashTargetExecute`.
- Panel `lockWristType()`: body **wrists** forces type **chain** and the type pulldown lists only `chain`.
- SkyrimNet plugin menu `SKSE/Plugins/SkyrimNet/config/plugins/SkyrimNet_Leash/manifest.yaml` (`plugin.version` 0.1.0): hotkey, distance tokens, type, body part (`leash.ui.bodyPart` notes wrists → `Leash_hand_chain`, type forced to chain), tie point, struggle narration interval (`leash.escape.narrationInterval`, default 5s), escape cooldown (`leash.escape.cooldown`, default 20s).
- DLL CRT is `/MD` (`x64-windows-static-md`). In-game must be the Release build.

### Escape / HUD

- Escape is try-only. Category copy may say they get the leash off so the LLM selects it; `StruggleExecute` never disconnects. While struggling they can pick `leash_none_struggle_stop`.
- Do not gate `leash_escape` on `is_leash_available` (combat). Struggle and the Escape category need `speaker_is_leashed` and not `speaker_is_struggling`; stop needs `speaker_is_struggling`.
- PrismaUI **unleash** is full power, including the player unclipping their own collar. The hotkey does not route through `StruggleExecute`.

### Install / FOMOD

- FOMOD refuses to install unless `Leash.esm` is active (no `LeashFramework.dll` fileDependency). Installs `SkyrimNet_Leash.esp`, `SKSE/`, `Scripts/`, `PrismaUI/`.
- Requires SkyrimNet, SKSE, Address Library, and [Leash Framework](https://www.nexusmods.com/skyrimspecialedition/mods/187303) (`Leash.esm` + `LeashFramework.dll`). PrismaUI is only needed for the panel hotkey; YAML actions work without it.
- ESP source is `Spriggit/SkyrimNet_Leash/` (small plugin, master `Leash.esm`, quest `SkyrimNet_Leash` with player alias). `make release` stamps `FOMOD/info.xml` from Makefile `VERSION=0.1.0` and packs `versions/SkyrimNet_Leash 0.1.0.7z` (no PDBs).

### Docs

- Player front door: `README.md`. Technical changelog: this file. Developer summary: `CHANGELOG-developer.md`. Player changelog: `CHANGELOG-player.md`. Agent map: `llms.txt` / `AGENTS.md`. Packer: `Makefile` + `.github/workflows/package.yml`.
