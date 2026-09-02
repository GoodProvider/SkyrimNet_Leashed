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

| Action | Effect |
| --- | --- |
| leash_leash_target | Speaker holds the leash; target is leashed. Style: `forceful\|normal\|gently`. Type: `body_rope\|neck_rope\|neck_chain\|magic_rope\|holder_shield`. |
| leash_leash_speaker | Target holds the leash; speaker is leashed. Same style and type tokens. |
| leash_unleash_target | Disconnect the leash the speaker shares with a named actor. Offered only when the speaker is on a leash with someone. |
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
