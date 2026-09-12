# Knowledge

Standing facts for agents. Consult before game, script, or ESP changes. Append quirks after sessions.

## Dependency updates

When reviewing current code against a **dependency update**, follow the `dependency_drift` skill (`.cursor/skills/dependency_drift/SKILL.md`) and read the latest `checkpoints/<dependency-name>-<version>.md`.

| Dependency | Latest checkpoint |
| --- | --- |
| Leash Framework | [checkpoints/leashframework-1.1.3.md](checkpoints/leashframework-1.1.3.md) (0.3.0 adapted; playtest 2026-09-10 looked fine) |
| SkyrimNet | [checkpoints/skyrimnet-beta25-rc7.md](checkpoints/skyrimnet-beta25-rc7.md) (0.4.0; PublicAPI v10; content plugin `goodprovider.leashed`) |

CMake **Configure** may run `git submodule update --init --recursive` from `SKSE_Source` and reset `Skyrim-Leash-Framework` to the **parent gitlink**. After a pin, the index must record the new SHA (`git add Skyrim-Leash-Framework`) or Configure will walk it back. Do not compile or ship Framework `.pex` / DLL from the submodule.

Beta 25 does not read `prompts/`, `config/triggers/`, or `config/actions/`. LLM content ships as `SKSE/Plugins/SkyrimNet/external/goodprovider.leashed/` (`manifest.json` `id` must equal the folder name). Prompt paths inside the plugin are unchanged (`prompts/leash_actions/…`, `0409_leashframework.prompt`). Action YAML filename (before `.yaml`) must equal the in-file `name` (case-insensitive); keep `name` casing. Settings schema stays at `config/plugins/SkyrimNet_Leashed/manifest.yaml` — that is not a content-plugin folder. Do not ship into `library/`. Upstream: SkyrimNet `docs/modding/MIGRATING_TO_BETA25.md`.
