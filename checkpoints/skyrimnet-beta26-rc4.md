# Checkpoint: SkyrimNet beta26-rc4

**Status:** SkyrimNet_Leashed **0.4.2** has its header pinned to the SkyrimNet **beta26-rc4** devkit (PublicAPI **v12**). **No PublicAPI or Papyrus signature break** for the surfaces we call. Next SkyrimNet bump: run skill `dependency_drift`, then write a new `checkpoints/skyrimnet-<version>.md`.

**Repo:** `c:\Skyrim\dev\mods\SkyrimNet_Leashed`  
**Compared:** `../skyrimnet devkit/CppAPI` (`skyrimnet-devkit-beta26-rc4.zip`, MO2 `version=d2026.10.4`) vs `../SkyrimNet 25 rc7/CppAPI`. Prior pin: [skyrimnet-beta25-rc7.md](skyrimnet-beta25-rc7.md).

Read first: [KNOWLEDGE.md](../KNOWLEDGE.md), [AGENTS.md](../AGENTS.md), `.cursor/skills/dependency_drift/SKILL.md`.

---

## Header pin

[`PublicAPI.h`](../SKSE_Source/include/SkyrimNet/PublicAPI.h) and [`PublicAPIMemoryQuery.h`](../SKSE_Source/include/SkyrimNet/PublicAPIMemoryQuery.h) are copied verbatim from the devkit `CppAPI/`. They report **currently 12**. `PublicAPIMemoryQuery.h` has comment-only changes against rc7.

**Missing upstream header:** rc4 `PublicAPI.h` does `#include "PublicAPIDiaryQuery.h"`, but neither the rc4 main zip nor the devkit zip ships that file. [`PublicAPIDiaryQuery.h`](../SKSE_Source/include/SkyrimNet/PublicAPIDiaryQuery.h) is a local stand-in, the same file as `../SkyrimNet_SexLab/SKSE_Source/include/PublicAPIDiaryQuery.h`. It defines just enough (`DiaryOrder`, `DiaryQuery`, `DiaryQueryToJSON`) for `QueryDiaryEntries()` to compile. **Replace it with the real header once upstream ships it.** We never query diaries.

ABI: same MSVC + dynamic **`/MD`**. In-game **Release** only. SE ≠ VR.

## API delta v10 → v12 (all unused here)

| Version | Added |
| --- | --- |
| v11 | `PublicGetTimelineState`, `PublicQueryDiaryEntries` (+ `QueryDiaryEntries()` wrapper), `PublicAddDiaryEntry`, `PublicUpdateDiaryEntry`, `PublicDeleteDiaryEntry`, `PublicUpdateMemory`, `PublicDeleteMemory`, `PublicSearchActors`, `PublicRegisterEvent` (optional audience) |
| v12 | `PublicSendCustomDecisionToLLM` (decision-model templates under `prompts/decisions/`) |

`FindFunctions()` loads the new pointers only when `PublicGetVersion() >= 11` / `>= 12`. That means the DLL still loads against v10 SkyrimNet, so `min_skyrimnet_version` stays `0.25.0`.

Doc-only return-shape changes: `PublicGetRecentEvents` now documents `data` / `id` / `actors` fields, and dialogue history documents `speaker` as `player`/`npc` with the line in `data`. We call neither.

## Surfaces we call (unchanged)

| Surface | Where |
| --- | --- |
| `PublicGetVersion` | [`Api.cpp`](../SKSE_Source/src/SkyrimNet/Api.cpp) (log only) |
| `PublicRegisterDecorator` | [`Api.cpp`](../SKSE_Source/src/SkyrimNet/Api.cpp) via [`Registration.cpp`](../SKSE_Source/src/SkyrimNet/Registration.cpp) |
| `PublicFormIDToUUID` | [`Api.cpp`](../SKSE_Source/src/SkyrimNet/Api.cpp) via [`StateCache.cpp`](../SKSE_Source/src/SkyrimNet/StateCache.cpp) |
| `PublicGetPluginConfigValue` | [`Api.cpp`](../SKSE_Source/src/SkyrimNet/Api.cpp) via [`Config.cpp`](../SKSE_Source/src/WebUI/Config.cpp) `"SkyrimNet_Leashed"` + `leash.*` |
| Papyrus narration/events | [`SkyrimNet_Leashed_Actions.psc`](../Scripts/Source/SkyrimNet_Leashed_Actions.psc) |

Decorators, YAML and prompt conventions: unchanged from [skyrimnet-beta25-rc7.md](skyrimnet-beta25-rc7.md).

## Out of scope / open

- Delete the stand-in `PublicAPIDiaryQuery.h` once upstream ships it
- CommonLibSSE-NG bump (SkyrimNet_SexLab moved to v11.0.0 alongside its rc4 align)
- Adopting any v11/v12 functions
- VR parity (this bridge is SE-only)
