# Checkpoint: SkyrimNet beta25-rc7

**Status:** SkyrimNet_Leashed **0.4.0** ships against installed SkyrimNet **beta25-rc7** (product **0.25.0**, PublicAPI **v10**). Content plugin `goodprovider.leashed` is in place. **No PublicAPI / Papyrus signature break** for surfaces we call. Next SkyrimNet bump: skill `dependency_drift`, then a new `checkpoints/skyrimnet-<version>.md`.

**Repo:** `c:\Skyrim\dev\mods\SkyrimNet_Leashed`  
**Compared:** this repo 0.4.0 vs `../SkyrimNet` (`SkyrimNet-beta25-rc7.zip`, MO2 `version=d2026.9.12.0`). Prior pin: [skyrimnet-beta25-rc6.md](skyrimnet-beta25-rc6.md).

Read first: [KNOWLEDGE.md](../KNOWLEDGE.md), [AGENTS.md](../AGENTS.md), `.cursor/skills/dependency_drift/SKILL.md`.

---

## Header pin

[`SKSE_Source/include/SkyrimNet/PublicAPI.h`](../SKSE_Source/include/SkyrimNet/PublicAPI.h) vs `../SkyrimNet/CppAPI/PublicAPI.h`: **currently 10** both. All **34** `Public*` exports match. Companion [`PublicAPIMemoryQuery.h`](../SKSE_Source/include/SkyrimNet/PublicAPIMemoryQuery.h) required. Do not call `PublicQueryMemoriesForActor` / `QueryMemoriesForActor`.

ABI: same MSVC + dynamic **`/MD`**. In-game **Release** only. SE ≠ VR.

## Plugin systems (two layers)

### A. Settings schema

[`config/plugins/SkyrimNet_Leashed/manifest.yaml`](../SKSE/Plugins/SkyrimNet/config/plugins/SkyrimNet_Leashed/manifest.yaml) with `plugin.name: SkyrimNet_Leashed`. C++ [`Config.cpp`](../SKSE_Source/src/WebUI/Config.cpp) calls `PublicGetPluginConfigValue("SkyrimNet_Leashed", "leash.*", …)`.

### B. Content library (Beta 25)

Ships at [`SKSE/Plugins/SkyrimNet/external/goodprovider.leashed/`](../SKSE/Plugins/SkyrimNet/external/goodprovider.leashed/) (`manifest.json` `id` equals folder name, `version` 0.4.0, `min_skyrimnet_version` 0.25.0). Actions and prompts live inside that folder. Filename stem equals in-file `name`. Prefix `leash_` → `leashed_`. Do not ship into `library/`. Upstream: `../SkyrimNet/docs/modding/MIGRATING_TO_BETA25.md`.

## Surfaces we call (unchanged)

| Surface | Where |
| --- | --- |
| `PublicRegisterDecorator` | [`Registration.cpp`](../SKSE_Source/src/SkyrimNet/Registration.cpp) |
| `PublicFormIDToUUID` | [`StateCache.cpp`](../SKSE_Source/src/SkyrimNet/StateCache.cpp) |
| `PublicGetPluginConfigValue` | [`Config.cpp`](../SKSE_Source/src/WebUI/Config.cpp) `"SkyrimNet_Leashed"` + `leash.*` |
| Papyrus narration/events | [`SkyrimNet_Leashed_Actions.psc`](../Scripts/Source/SkyrimNet_Leashed_Actions.psc) |

### Decorators registered

**Flags:** `is_leash_available`, `is_unleash_available`, `speaker_on_leash`, `leashed_nearby`, `unleashed_nearby`, `speaker_is_leashed`, `collared_nearby`, `speaker_is_struggling`, `is_struggle_enabled`.

**JSON payloads:** `get_nearby_unleashed_actors`, `get_nearby_leashed_actors`, `get_speaker_leash_partners`, `leashframework_visible_pairs`, `get_nearby_collared_actors`, `get_nearby_actors`.

### YAML / prompt conventions

- Category parents (`leashed_leash.yaml`, `leashed_change.yaml`, `leashed_escape.yaml`): `customCategory` + `intent`; no `scriptName`. Root `leashed_none_*` omit `customCategory`.
- Eligibility: `decoratorName`, `arguments: [currentActor]`, `expectedValue: available|unavailable`.
- Bio overlay `prompts/submodules/character_bio/0409_leashframework.prompt` unchanged relative path inside the plugin.

## Out of scope / open

- Calling `PublicQueryMemoriesForActor` / `QueryMemoriesForActor`
- Header re-copy (comment-only upstream delta)
- Plugin Hub publish
- VR parity (this bridge is SE-only)
- Git tag / GitHub Release / `make release`
