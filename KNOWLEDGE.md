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
| Leash Framework | [checkpoints/leashframework-1.1.3.md](checkpoints/leashframework-1.1.3.md) (0.3.0 adapted; playtest 2026-09-10 looked fine) |
| SkyrimNet | [checkpoints/skyrimnet-beta25-rc7.md](checkpoints/skyrimnet-beta25-rc7.md) (0.4.0; PublicAPI v10; content plugin `goodprovider.leashed`) |

CMake **Configure** may run `git submodule update --init --recursive` from `SKSE_Source` and reset `Skyrim-Leash-Framework` to the **parent gitlink**. After a pin, the index must record the new SHA (`git add Skyrim-Leash-Framework`) or Configure will walk it back. Do not compile or ship Framework `.pex` / DLL from the submodule.

Beta 25 does not read `prompts/`, `config/triggers/`, or `config/actions/`. Canonical LLM content is `SKSE/Plugins/SkyrimNet/external/goodprovider.leashed/` (`manifest.json` `id` must equal the folder name). The same actions and prompts are also copied to `config/actions/` and `prompts/` so pre-0.25 SkyrimNet still loads them (`tools/sync_legacy_skyrimnet_content.py`; do not edit those copies by hand). Prompt paths inside the plugin are unchanged (`prompts/leash_actions/…`, `0409_leashframework.prompt`). Action YAML filename (before `.yaml`) must equal the in-file `name` (case-insensitive); keep `name` casing. Settings schema stays at `config/plugins/SkyrimNet_Leashed/manifest.yaml` — that is not a content-plugin folder. Do not ship into `library/`. Upstream: SkyrimNet `docs/modding/MIGRATING_TO_BETA25.md`.

## Papyrus quirks

- Full unclip restores the pre-apply count of the **one** Leash.esm mesh we equipped. A copy they already had stays. `EquipItem` spares of that form are removed. Do not `RemoveItem` every leash type or the whole stack. Vanilla prisoner cuffs stay `RemoveItem` 1.
