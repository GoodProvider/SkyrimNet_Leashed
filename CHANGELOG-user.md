https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/0.4.2

- If a leash will not go on (for example a collar on an Argonian or Khajiit), the mod now tries the other leashes, closest match first: another neck leash, then waist or wrists, then a holder-worn chain. You only get the “will not stay on” line if none of them work.
- Devious Devices hides normal armor on bound NPCs (armbinder and similar), which made their leash invisible. The mod now notices this and uses the holder's hand chain instead. To get the real collar on bound NPCs, pick the optional "Devious Devices NG: show leash collars on bound NPCs" patch in the installer, or add `45` to `aiHiderOverrideSlots` in `SKSE/Plugins/DeviousDevices.ini` yourself (for example `aiHiderOverrideSlots = 60, 45`), then restart the game.
- New optional "Beast-race leash meshes" plugin: lets leash collars, ropes and the holder's hand chain show on Argonians and Khajiit. It uses the human meshes, so the fit may clip.
- When a leash with no holder comes undone, you now hear what you see: if it is still visible it stays on and dangles; if it is not visible it is removed, and the narration says it would not stay on.
- The SkyrimNet SexLab target menu leash panel now lets you pick the leash type (chain, rope, magic) and where it attaches (neck, wrists, waist). Attachment points with no leash of that type are greyed out.
- The leash panel can open in a vertical stack aimed at a chosen actor, not only as the horizontal hotkey bar.
- Spoken leash lines now say “middle length” (and the other distance words) so they are not read as a leash type.
- Character bios use the same “length” wording for tied, dangling, and held leashes.
- Distance phrases like “middle length” still map to the same settle and catch-up distances as before.
- The README no longer uses the old hero image. LoversLab and Devious Devices links are marked NSFW.
- A collared NPC still cannot unclip themselves. The player can still do that from the panel hotkey.
