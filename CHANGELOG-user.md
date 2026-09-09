https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/0.1.0

- First public SkyrimNet bridge for Leash Framework: NPCs can leash, take, give, tie, refuse, and unclip someone else through SkyrimNet actions.
- Three action groups (Leash, Change leash, Escape) plus one root Unleash. NPCs cannot pick an action that takes their own leash off.
- Unleash a nearby collared person with `leash_target_unleash`. It never unclips the speaker.
- Escape is struggle-only: animation and narration, leash stays on. Repeated tries in a 20-second window just count attempts.
- Optional ZaZ standing clips for struggle; without ZaZ it uses vanilla idles. SexLab, Devious Devices, and Unforgiving Devices are not required.
- `\` opens a PrismaUI leash bar (enable/remap in the SkyrimNet plugin menu). Unleash on that bar is full power, including taking off your own collar.
- YAML actions work without PrismaUI. The panel needs PrismaUI installed.
- Character bios list leash pairs the speaker can see (held, dangling, or tied).
- FOMOD will not install unless `Leash.esm` is active. Load `SkyrimNet_Leash.esp` after `Leash.esm`. You still need `LeashFramework.dll` in-game.
- Wrists always uses the chain hand mesh (panel type locks to chain). That also equips prisoner cuffs and a bound-standing pose; getting up after a ragdoll pull puts the pose back on.
- Children can be leashed. Distance, type, body part, and tie point defaults live in the SkyrimNet plugin menu.
