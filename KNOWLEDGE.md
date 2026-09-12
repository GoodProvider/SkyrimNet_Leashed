# Knowledge

Standing facts for agents. Consult before game, script, or ESP changes. Append quirks after sessions.

## Dependency updates

When reviewing current code against a **dependency update**, follow the `dependency_drift` skill (`.cursor/skills/dependency_drift/SKILL.md`) and read the latest `checkpoints/<dependency-name>-<version>.md`.

| Dependency | Latest checkpoint |
| --- | --- |
| Leash Framework | [checkpoints/leashframework-1.1.3.md](checkpoints/leashframework-1.1.3.md) (0.3.0 adapted; playtest 2026-09-10 looked fine) |
| SkyrimNet | [checkpoints/skyrimnet-beta25-rc6.md](checkpoints/skyrimnet-beta25-rc6.md) (0.3.0 header pin; PublicAPI v10 additive, no caller changes) |

CMake **Configure** may run `git submodule update --init --recursive` from `SKSE_Source` and reset `Skyrim-Leash-Framework` to the **parent gitlink**. After a pin, the index must record the new SHA (`git add Skyrim-Leash-Framework`) or Configure will walk it back. Do not compile or ship Framework `.pex` / DLL from the submodule.
