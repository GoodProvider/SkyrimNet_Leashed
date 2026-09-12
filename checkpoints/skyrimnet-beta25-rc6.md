# Checkpoint: SkyrimNet beta25-rc6

**Status:** SkyrimNet_Leashed **0.3.0** reviewed against installed SkyrimNet **beta25-rc6** (product **0.25.0**, PublicAPI **v10**). Header pin only. No caller, YAML, prompt, or Papyrus changes. Next SkyrimNet bump: skill `dependency_drift`, then a new `checkpoints/skyrimnet-<version>.md`.

**Repo:** `c:\Skyrim\dev\mods\SkyrimNet_Leashed`  
**Compared:** this repo 0.3.0 vs `../SkyrimNet` (`skyrimnet-bundle-beta25-rc6.zip`, MO2 `version=d2026.9.10.0`). Runtime log `SkyrimNet 0.25.0` / `PublicGetVersion() = 10`. GitHub tag `MinLL/SkyrimNet-GamePlugin` **`vbeta25-rc6`**.

Read first: [KNOWLEDGE.md](../KNOWLEDGE.md), [AGENTS.md](../AGENTS.md), `.cursor/skills/dependency_drift/SKILL.md`.

---

## Header pin

[`SKSE_Source/include/SkyrimNet/PublicAPI.h`](../SKSE_Source/include/SkyrimNet/PublicAPI.h) copied from `../SkyrimNet/CppAPI/PublicAPI.h` (**currently 10**). Companion [`SKSE_Source/include/SkyrimNet/PublicAPIMemoryQuery.h`](../SKSE_Source/include/SkyrimNet/PublicAPIMemoryQuery.h) is required: `PublicAPI.h` includes it.

`FindFunctions()` lives in the header. [`Api.cpp`](../SKSE_Source/src/SkyrimNet/Api.cpp) does not wrap v10. **Do not call** `PublicQueryMemoriesForActor` / `QueryMemoriesForActor`.

ABI: same MSVC + dynamic **`/MD`**. In-game must be **Release**. Debug `/MDd` still breaks decorator `std::string` (SEH). SE ≠ VR.

## Additive API (v9 → v10)

- `PublicQueryMemoriesForActor(formId, queryJSON)` plus typed `MemoryQuery` / `QueryMemoriesForActor` wrapper.
- Existing v2–v9 exports (decorators, UUID, plugin config, C++ actions, events, world knowledge) are unchanged.

## Surfaces we call (unchanged)

**C++ (resolved):** `PublicRegisterDecorator` (v5+ required), `PublicFormIDToUUID`, `PublicGetPluginConfigValue`.

**Papyrus** in [`SkyrimNet_Leashed_Actions.psc`](../Scripts/Source/SkyrimNet_Leashed_Actions.psc): `DirectNarration`, `RegisterEvent`, `RegisterShortLivedEvent`, `GetSpeechQueueSize`. Signatures in `../SkyrimNet/Source/Scripts/SkyrimNetApi.psc` match.

**Unused (declared in PublicAPI, not wrapped):** `PublicRegisterCPPAction`, `PublicRegisterCPPSubCategory`, memory/diary/dialogue queries, `PublicRegisterEventCallback`, busy-state, world-knowledge CRUD, `PublicSendCustomPromptToLLM`, **`PublicQueryMemoriesForActor`**. Actions stay YAML → Papyrus.

## Caller map

| Surface | Where |
| --- | --- |
| `PublicRegisterDecorator` | [`Registration.cpp`](../SKSE_Source/src/SkyrimNet/Registration.cpp) flags + JSON payloads |
| `PublicFormIDToUUID` | [`StateCache.cpp`](../SKSE_Source/src/SkyrimNet/StateCache.cpp) actor IDs in JSON |
| `PublicGetPluginConfigValue` | [`Config.cpp`](../SKSE_Source/src/WebUI/Config.cpp) `leash.*` keys |
| `SkyrimNetApi.DirectNarration` | `Narrate` (importance 1, or 2 / optional when speech queue empty) |
| `SkyrimNetApi.RegisterEvent` | `Narrate` when player cannot see, or queue busy (`"leash"`) |
| `SkyrimNetApi.RegisterShortLivedEvent` | taut pull TTL; struggle heartbeat `leash_struggle_<formId>` 1500 ms |
| `SkyrimNetApi.GetSpeechQueueSize` | gates DirectNarration vs `RegisterEvent` |
| Action YAML | [`SKSE/Plugins/SkyrimNet/config/actions/`](../SKSE/Plugins/SkyrimNet/config/actions/) |
| Prompts | [`prompts/leash_actions/`](../SKSE/Plugins/SkyrimNet/prompts/leash_actions/), bio [`0409_leashframework.prompt`](../SKSE/Plugins/SkyrimNet/prompts/submodules/character_bio/0409_leashframework.prompt) |
| Manifest | [`manifest.yaml`](../SKSE/Plugins/SkyrimNet/config/plugins/SkyrimNet_Leashed/manifest.yaml) (`plugin.name` SkyrimNet_Leashed) |

### Decorators registered

**Flags** (`available` / `unavailable`): `is_leash_available`, `is_unleash_available` (kept public), `speaker_on_leash` (kept public), `leashed_nearby` (kept public), `unleashed_nearby`, `speaker_is_leashed`, `collared_nearby`, `speaker_is_struggling`, `is_struggle_enabled`.

**JSON payloads:** `get_nearby_unleashed_actors`, `get_nearby_leashed_actors` (kept public), `get_speaker_leash_partners` (kept public), `leashframework_visible_pairs`, `get_nearby_collared_actors`, `get_nearby_actors`.

### YAML / prompt conventions still valid

- Category parents (`leash_leash_.yaml`, `leash_change_.yaml`, `leash_escape_.yaml`): `customCategory` + `intent`; no `scriptName`. Root `leash_none_*` omit `customCategory`. These remain **our** convention (still absent from SkyrimNet `WORKFLOW_ACTIONS.md`).
- Eligibility: `decoratorName`, `arguments: [currentActor]`, `expectedValue: available|unavailable`.
- Bio overlay `0409_leashframework.prompt` sits between stock `0400_appearance` and `0410_equipment` (mod-integrations 0300–0499). No name collision.

## Out of scope / open

- Calling `PublicQueryMemoriesForActor` / `QueryMemoriesForActor`
- VR parity (this bridge is SE-only)
- Makefile `VERSION` bump / changelog (review-only pin)
- Git tag / GitHub Release / `make release`
