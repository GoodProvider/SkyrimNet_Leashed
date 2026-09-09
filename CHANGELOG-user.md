## Unreleased

- Optional Devious Devices standing struggle clips (neck / wrists / waist). Install the DD *files*, leave every DD plugin disabled, skip SexLab, and rebuild FNIS/Nemesis/Pandora. Without those animation files, struggle stays the vanilla nervous idle.

https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/0.1.0

- First public SkyrimNet bridge for Leash Framework: NPCs can leash, take, give, tie, refuse, and unclip someone else through SkyrimNet actions.
- Three action groups (Leash, Change leash, Escape) plus root Unleash (`leash_none_target_unleash`) and root stop (`leash_none_struggle_stop`). NPCs cannot pick an action that takes their own leash off.
- Unleash a nearby collared person with `leash_none_target_unleash`. It never unclips the speaker.
- Escape is struggle-only: looping nervous idle and narration, leash stays on. They keep struggling if they walk. They stop when they pick `leash_none_struggle_stop`, get yanked, or are unclipped. While struggling, nearby context gets a short-lived “continues to struggle” event every second. Spoken still-beats wait for Escape cooldown (default 20s, settable): “Despite {name}'s attempts, the leash holds.”
- SexLab, Devious Devices, and Unforgiving Devices are not required.
- `\` opens a PrismaUI leash bar (enable/remap in the SkyrimNet plugin menu). Unleash on that bar is full power, including taking off your own collar.
- YAML actions work without PrismaUI. The panel needs PrismaUI installed.
- Character bios list leash pairs the speaker can see (held, dangling, or tied).
- FOMOD will not install unless `Leash.esm` is active. Load `SkyrimNet_Leash.esp` after `Leash.esm`. You still need `LeashFramework.dll` in-game.
- Wrists always uses the chain hand mesh (panel type locks to chain). That also equips prisoner cuffs and a bound-standing pose; getting up after a ragdoll pull puts the pose back on.
- Children can be leashed. Distance, type, body part, and tie point defaults live in the SkyrimNet plugin menu.
