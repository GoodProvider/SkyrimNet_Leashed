# Changelog

## [0.1.0](https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/0.1.0)

First public SkyrimNet ↔ Leash Framework bridge.

### Actions

- Categories `leash_leash`, `leash_change`, and `leash_escape` each carry a dynamic `intent` param (no execute function on the parent).
- Root action `leash_target_unleash` (`UnleashTargetExecute`): speaker unclips a nearby collared actor, never themselves. No `customCategory`. Eligibility: `collared_nearby`.
- Removed the `leash_unleash` category (`leash_unleash_speaker`, `leash_unleash_target`). Self-unclip is not an LLM action.
- `leash_leash`: `leash_leash_target` / `leash_leash_speaker` (`LeashedToHolder`), `leash_leash_tie_target` (`LeashedToTiePoint`), `leash_leash_refused_speaker` / `leash_leash_refused_target` (`LeashedRefused`). Parent gates `is_leash_available` plus `unleashed_nearby` or not `speaker_is_leashed`.
- `leash_change`: `leash_change_take_*` (`TakeLeash`), `leash_change_give_*` (`GiveLeash`), `leash_change_tie_*` (`LeashedToTiePoint`). Parent gates `is_leash_available` plus `collared_nearby` or `speaker_is_leashed`.
- `leash_escape_struggle` (`StruggleExecute`): collared speaker only (`speaker_is_leashed`). The leash does not come off.
- Tokens: style `forcefully|normally|gently`; distance `tight|short|middle|long`; type `chain|rope|magic`; body part `neck|wrists|waist`; tie point `floor|left|back|front|right|wall`. Holder and give-receiver may be `None` (dangling, not world-tied).
- Wrists is chain only. `leash_leash_target` / `leash_leash_speaker` / `leash_leash_tie_target` `leashType` copy says so; `body_part` copy says wrists uses the chain hand mesh. Leash.esm only ships `Leash_hand_chain` (`0xD69`).
- DirectNarration prompts under `SKSE/Plugins/SkyrimNet/prompts/leash_actions/`. Bio overlay `0409_leashframework.prompt` lists visible pairs via `leashframework_visible_pairs`.

### Papyrus

- `SkyrimNet_Leash_Actions`: apply, take, give, tie, refuse, struggle, and unleash execute; Framework event hooks; pair cache; type-armor equip; narration importance 1 / 2 / 3.
- `KindForBodyPart` forces `chain` when the body part is `wrists`. `ArmorForKind` uses `Leash.esm` `0xD69` for wrists (and for legacy `holder_shield`).
- `ApplyWristBind` / `ClearWristBind`: prisoner cuffs `Skyrim.esm` `0x10E039`, bound-standing idle `0x109837` / cut idle `0x109B6A`, fallbacks `OffsetBoundStandingPlayerInstant` / `BoundStandingCut`. `OnLeashFrameworkRagdollPulled` registers `GetUpEnd` to re-apply the wrist bind.
- `UnleashTargetExecute` rejects speaker-as-leashed. `UnleashSpeakerExecute` remains for the PrismaUI panel (including the player unclipping themselves).
- `StruggleExecute`: first try in a 20s window is DirectNarration plus a standing idle; later tries in that window update one short-lived `leash_struggle_{formID}` event with the attempt count. Optional ZaZ standing clips `ZazAPC225` / `ZazAPC001` / `ZazAPC003`; otherwise `IdleNervous` / `IdleWarmHands` / `IdleInjured`.
- `SkyrimNet_Leash_PlayerAlias` (`OnPlayerLoadGame`): re-registers Framework events and `RepushCachedPairs` into the SKSE cache.
- `SkyrimNet_Leash_Native`: `NotifyLeash`, `NotifyUnleash`, `NormalizeToken`, `DistanceMax`, `DistanceFromLength`, `TraceLeashBones`.

### SKSE / WebUI

- Flag decorators: `is_leash_available`, `is_unleash_available`, `speaker_on_leash`, `leashed_nearby`, `unleashed_nearby`, `speaker_is_leashed`, `collared_nearby`. `is_unleash_available` and `speaker_on_leash` stay registered; in-tree YAML no longer consumes them.
- JSON payloads: `get_nearby_unleashed_actors`, `get_nearby_leashed_actors`, `get_speaker_leash_partners`, `leashframework_visible_pairs`, `get_nearby_collared_actors`, `get_nearby_actors`.
- PrismaUI leash panel (`PrismaUI/views/SkyrimNet_Leash/`): hotkey `leash.controls.hotkey` default VK 220 (`\`). Panel **unleash** dispatches `UnleashSpeakerExecute` when the subject is the leashed actor, else `UnleashTargetExecute`.
- Panel `lockWristType()`: body **wrists** forces type **chain** and the type pulldown lists only `chain`.
- SkyrimNet plugin menu `SKSE/Plugins/SkyrimNet/config/plugins/SkyrimNet_Leash/manifest.yaml` (`plugin.version` 0.1.0): hotkey, distance tokens, type, body part (`leash.ui.bodyPart` notes wrists → `Leash_hand_chain`, type forced to chain), tie point.
- DLL CRT is `/MD` (`x64-windows-static-md`). In-game must be the Release build.

### Escape / HUD

- Escape is try-only. Category copy may say they get the leash off so the LLM selects it; `StruggleExecute` never disconnects.
- Do not gate `leash_escape` on `is_leash_available` (combat). The child only needs `speaker_is_leashed`.
- PrismaUI **unleash** is full power, including the player unclipping their own collar. The hotkey does not route through `StruggleExecute`.

### Install / FOMOD

- FOMOD refuses to install unless `Leash.esm` is active (no `LeashFramework.dll` fileDependency). Installs `SkyrimNet_Leash.esp`, `SKSE/`, `Scripts/`, `PrismaUI/`.
- Requires SkyrimNet, SKSE, Address Library, and [Leash Framework](https://www.nexusmods.com/skyrimspecialedition/mods/187303) (`Leash.esm` + `LeashFramework.dll`). PrismaUI is only needed for the panel hotkey; YAML actions work without it.
- ESP source is `Spriggit/SkyrimNet_Leash/` (small plugin, master `Leash.esm`, quest `SkyrimNet_Leash` with player alias). `make release` stamps `FOMOD/info.xml` from Makefile `VERSION=0.1.0` and packs `versions/SkyrimNet_Leash 0.1.0.7z` (no PDBs).

### Docs

- Player front door: `README.md`. Technical changelog: this file. Player summary: `CHANGELOG-user.md`. Agent map: `llms.txt` / `AGENTS.md`. Packer: `Makefile` + `.github/workflows/package.yml`.
