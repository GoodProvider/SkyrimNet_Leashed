# SkyrimNet_Leash

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/goodprovider)

SkyrimNet (LLM) bridge for [Leash Framework](https://www.nexusmods.com/skyrimspecialedition/mods/187303). NPCs can leash and unleash through SkyrimNet actions, and character bios list leash pairs the speaker can see.

## Play / install

Requirements:

- [SkyrimNet](https://github.com/MinLL/SkyrimNet-GamePlugin)
- [Leash Framework (Nexus 187303)](https://www.nexusmods.com/skyrimspecialedition/mods/187303) — `Leash.esm` and `SKSE\Plugins\LeashFramework.dll`
- SKSE, Address Library

The FOMOD installer **refuses to install** unless `Leash.esm` is active and `SKSE\Plugins\LeashFramework.dll` is present.

Load `SkyrimNet_Leash.esp` after `Leash.esm`. Enable the mod in MO2 or Vortex.

Leash apply still needs leash bones on the target (Leash.esm armor / SMP node names). See the Leash Framework Nexus page.

### Actions

Three categories. Style: `forcefully|normally|gently`. Distance: `tight|short|middle|long`. Type: `chain|rope|magic`. Body part: `neck|waist`. Tie point: `floor|left|back|front|right|wall`.

**leash_leash** (start a leash)

| Action | Effect |
| --- | --- |
| leash_leash_target | Speaker leashes the target and holds the leash. |
| leash_leash_speaker | A nearby actor leashes the speaker and holds the leash. Speaker must not already be leashed. |
| leash_leash_tie_target | Speaker ties an unleashed target to a world point. |
| leash_leash_refused_speaker | Speaker refuses to be leashed by a nearby actor. |
| leash_leash_refused_target | A nearby actor refuses to be leashed by the speaker. |

**leash_change** (move an existing leash)

| Action | Effect |
| --- | --- |
| leash_change_take_target | Speaker takes a collared actor's leash. |
| leash_change_take_speaker | A nearby actor takes the speaker's leash. Speaker must be leashed. |
| leash_change_give_target | Speaker gives a collared actor's leash to another nearby actor. |
| leash_change_give_speaker | A nearby actor gives the speaker's leash to another nearby actor. |
| leash_change_tie_target | Speaker re-ties a collared actor's leash to a world point. |
| leash_change_tie_speaker | A nearby actor re-ties the speaker's leash to a world point. |

**leash_unleash**

| Action | Effect |
| --- | --- |
| leash_unleash_target | Disconnect the leash the speaker shares with a named actor. |
| leash_unleash_speaker | Drop every leash the speaker is part of. |

Children may be leashed.

## Build from clone

Prerequisites: Git, Visual Studio 2022 (Desktop C++), CMake 3.21+, [vcpkg](https://github.com/microsoft/vcpkg) with `VCPKG_ROOT` set. Use an “x64 Native Tools” / Developer PowerShell prompt so `cl.exe` is on PATH.

```text
git clone --recurse-submodules https://github.com/GoodProvider/SkyrimNet_Leash.git
cd SkyrimNet_Leash
```

If you already cloned without submodules:

```text
git submodule update --init --recursive
```

Later updates:

```text
git pull
git submodule update --init --recursive
git submodule update --remote --merge
```

### Papyrus

VS Code / Cursor task **`compile: pyro`** (uses `skyrimse.ppj` and `Scripts/Source`).

### SKSE plugin

VS Code / Cursor tasks **`CMake: Configure (Debug)`** then **`CMake: Build SKSE (Debug)`**, or CLI:

```text
cd SKSE_Source
cmake --preset debug
cmake --build --preset build-debug
```

Release: `--preset release` / `build-release`.

Those tasks copy `SkyrimNet_Leash.dll` to `SKSE/Plugins/`. If `SKYRIM_MODS_FOLDER` is set to your MO2 mods folder, CMake also deploys into `SKYRIM_MODS_FOLDER/SkyrimNet Leash/SKSE/Plugins/`.

Confirm the mod folder contains:

- `SKSE/Plugins/SkyrimNet_Leash.dll`
- `SKSE/Plugins/SkyrimNet/config/actions/`
- `SKSE/Plugins/SkyrimNet/prompts/submodules/character_bio/0409_leashframework.prompt`
- `SkyrimNet_Leash.esp`
- compiled `Scripts/SkyrimNet_Leash_Actions.pex`, `Scripts/SkyrimNet_Leash_PlayerAlias.pex`, and `Scripts/SkyrimNet_Leash_Native.pex`

### ESP from Spriggit

`Spriggit/SkyrimNet_Leash/` is the ESP source. Deserialize with Spriggit when you need a binary `.esp`:

```text
dotnet tool run spriggit deserialize -i Spriggit/SkyrimNet_Leash -o SkyrimNet_Leash.esp
```

## Submodules

| Path | Upstream |
| --- | --- |
| `Skyrim-Leash-Framework` | https://github.com/asdasdduck/Skyrim-Leash-Framework (`master`) |
| `SkyrimNet-GamePlugin` | https://github.com/MinLL/SkyrimNet-GamePlugin (`main`) |
