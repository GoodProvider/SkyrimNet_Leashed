# SkyrimNet_Leash

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/goodprovider)

SkyrimNet (LLM) bridge for [Leash Framework](https://www.nexusmods.com/skyrimspecialedition/mods/187303). NPCs can leash and unleash through SkyrimNet actions, and character bios list leash pairs the speaker can see.

## Play / install

Requirements:

- [SkyrimNet](https://github.com/MinLL/SkyrimNet-GamePlugin)
- [Leash Framework (Nexus 187303)](https://www.nexusmods.com/skyrimspecialedition/mods/187303) — `Leash.esm` and `SKSE\Plugins\LeashFramework.dll`
- SKSE, Address Library
- [PrismaUI](https://www.nexusmods.com/skyrimspecialedition/mods/114324) — only needed for the in-game leash panel hotkey. YAML actions work without it.

The FOMOD installer **refuses to install** unless `Leash.esm` is active and `SKSE\Plugins\LeashFramework.dll` is present.

Load `SkyrimNet_Leash.esp` after `Leash.esm`. Enable the mod in MO2 or Vortex.

Leash apply still needs leash bones on the target (Leash.esm armor / SMP node names). See the Leash Framework Nexus page.

### Leash panel hotkey

With PrismaUI installed, `\` opens a horizontal leash bar. Verb options depend on whether the selected leashed actor is already collared: **leash** / **leash to** when unleashed, **unleash** / **tie to** / **give to** when leashed. Extra columns (distance, type, holder, location) appear only for the chosen verb. Press `\` again or Escape to close. The game pauses while the panel is focused.

Enable, remap, and default **distance** / **type** / **tie point** live in the SkyrimNet plugin menu (`SkyrimNet_Leash`). SkyrimNet_SexLab’s Start Sex hotkey also defaults to `\` but is off unless you turn it on — do not bind both to the same key.

### Actions

Three categories. Style: `forcefully|normally|gently`. Distance: `tight|short|middle|long`. Type: `chain|rope|magic`. Body part: `neck|waist`. Tie point: `floor|left|back|front|right|wall`. Holder and give-receiver may be `None` to leave the leash hanging from the collared actor (not tied to a world point). Take still picks that leash up.

**leash_leash** (start a leash)

| Action | Effect |
| --- | --- |
| leash_leash_target | Speaker leashes the target. Holder is the speaker, another nearby actor, or None (dangling). |
| leash_leash_speaker | A nearby actor leashes the speaker. Holder is that actor or None. Speaker must not already be leashed. |
| leash_leash_tie_target | Speaker ties an unleashed target to a world point. |
| leash_leash_refused_speaker | Speaker refuses to be leashed by a nearby actor. |
| leash_leash_refused_target | A nearby actor refuses to be leashed by the speaker. |

**leash_change** (move an existing leash)

| Action | Effect |
| --- | --- |
| leash_change_take_target | Speaker takes a collared actor's leash. |
| leash_change_take_speaker | A nearby actor takes the speaker's leash. Speaker must be leashed. |
| leash_change_give_target | Speaker gives a collared actor's leash to another nearby actor, or to None (drops it). |
| leash_change_give_speaker | A nearby actor gives the speaker's leash to another nearby actor, or to None. |
| leash_change_tie_target | Speaker re-ties a collared actor's leash to a world point. |
| leash_change_tie_speaker | A nearby actor re-ties the speaker's leash to a world point. |

**leash_unleash**

| Action | Effect |
| --- | --- |
| leash_unleash_target | Disconnect the leash the speaker shares with a named actor. |
| leash_unleash_speaker | Drop every leash the speaker is part of. |

Children may be leashed.

Higher importance uses SkyrimNet DirectNarration so nearby NPCs react immediately; lower importance records a `leash` event for context without interrupting speech. If the player is not the subject, leashed actor, or holder and has no line of sight on the leashed actor, the line is always an event.

| Importance | Lines | Narration |
| --- | --- | --- |
| 1 | Apply, take, give, tie, refuse, or unleash while the player is subject, leashed, holder, or give-receiver | Always DirectNarration |
| 2 | Those same actions without the player, or a ragdoll stumble | DirectNarration if SkyrimNet’s speech queue is empty, otherwise a `leash` event |
| 3 | Taut (non-ragdoll) pull | `leash` event if the pull cooldown is free, otherwise dropped |

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
- `SKSE/Plugins/SkyrimNet/config/plugins/SkyrimNet_Leash/manifest.yaml`
- `PrismaUI/views/SkyrimNet_Leash/index.html`
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
