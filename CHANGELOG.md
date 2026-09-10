# Changelog

## Unreleased

- Renamed the plugin identity from `SkyrimNet_Leash` to `SkyrimNet_Leashed`: `SkyrimNet_Leashed.esp` (quest EditorID `SkyrimNet_Leashed`, FormID `000800` unchanged), `SkyrimNet_Leashed_Actions` / `_PlayerAlias` / `_Native`, `SkyrimNet_Leashed.dll` (and `SkyrimNet_Leashed.log`), `PrismaUI/views/SkyrimNet_Leashed/`, `SKSE/Plugins/SkyrimNet/config/plugins/SkyrimNet_Leashed/manifest.yaml` (`plugin.name`), and `questEditorId` / `scriptName` in all 14 executable action YAMLs. Action names, `leash.*` config keys, and decorator IDs are unchanged.
- Packaging renamed with it: FOMOD `SkyrimNet Leashed`, `versions/SkyrimNet_Leashed ${VERSION}.7z`, CMake project `SkyrimNet_Leashed`, MO2 deploy folder `SkyrimNet_Leashed`. Existing installs must be removed rather than updated in place, or the old ESP and DLL keep loading alongside the new ones.
- `leash_escape_struggle` is a looping struggle state (`IdleNervous` while standing, then optional DD FNIS events by body part: neck `DDCollarStruggle01`, wrists `DDRegCuffsFrontStruggle01`, waist `DDChastityBeltStruggle01`). No ESM check; unregistered events stay on `IdleNervous`. Walking does not end it; yank, unclip, or `leash_none_struggle_stop` does. While struggling, a short-lived `leash` event (`{name} continues to struggle with {her/his/their} leash.`) refreshes every second. DirectNarration still-beats wait for the longer of `leash.escape.narrationInterval` (default 5) and `leash.escape.cooldown` (default 20): `Despite {name}'s attempts, the leash holds.`
- Root `leash_none_struggle_stop` (`StopStruggleExecute`), gated on `speaker_is_struggling`. Opposite of Escape; no `customCategory`. Optional DirectNarration: `{name} stops struggling to remove their leash.`
- Root Unleash is `leash_none_target_unleash`. Escape category gates match struggle (`speaker_is_leashed` and not `speaker_is_struggling`).
- Dropped ZaZ body-part clips and the 20s attempt-count window.

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

- Player front door: `README.md`. Technical changelog: this file. Player summary: `CHANGELOG-user.md`. Agent map: `llms.txt` / `AGENTS.md`. Packer: `Makefile` + `.github/workflows/package.yml`.
