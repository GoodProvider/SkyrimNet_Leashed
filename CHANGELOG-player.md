## [0.4.5](https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/0.4.5)

- The `\` leash panel now needs Magelight UI instead of PrismaUI. Without it the panel is off; everything else still works.
- NPCs stop struggling as soon as the leash pulls them, including when they start following you. They only struggle while you stand still.
- NPCs no longer start struggling while knocked down or being dragged.
- Should fix a long freeze when tabbing back in and a small stutter every 3/4 second in busy areas.
- Leashes that no longer exist are cleared when you load a save, so NPCs stop mentioning them.
- After loading a save from an earlier session, leash pull lines and injuries no longer stop for a while.
- Changing a neck or waist leash to wrists no longer leaves the old leash on.
- NPCs far away, including leashed NPCs left in another area, no longer show up as nearby.
- Loading a save no longer narrates a downed NPC as being dragged.
- Typing `\` in the console no longer opens the leash panel.

## [0.4.4](https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/0.4.4)

- Leashing a fresh Diary of Mine captive no longer fights DOM's pose (needs SkyrimNet DOM update).
- The leash no longer tries to "unstick" an NPC that DOM is posing, tying, or holding cowering.
- NPCs who are already leashed are not leashed again every few lines.

## [0.4.3](https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/0.4.3)

### Injury Engine (optional plugin support) 

- Optional Injury Engine: being yanked off your feet scrapes you, dragging adds abrasions. They heal, never kill.
- With Injury Engine, the yank and its scrapes are narrated as one line (setting "Falling hurts").
- NPCs notice when someone is dragged along by a leash (setting "Dragging event interval").

### NPC Struggling 

- Struggling against a leash stops on its own after 7 seconds (setting "Maximum Struggle period"). It also stops right away when OStim, SexLab, furniture, or a mount takes over the NPC, so their animation is not interrupted.

## [0.4.2](https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/0.4.2)

- The leash panel (`\` hotkey) is now a vertical menu like SexLab's Target Menu.
- SexLab's Target Menu has a leash option: leash, tie, give, or unleash from there.
- If a leash won't fit (Argonian, Khajiit), the mod tries other leashes, closest match first.
- Devious Devices-bound NPCs keep a visible leash; an optional patch shows collars and ropes.
- Optional beast-race plugin shows leash meshes on Argonians and Khajiit.
- Leashed NPCs knocked down no longer stay stuck on the ground; they get up or are reset.
- When a holder-less leash comes undone, the narration matches what you see.
- On VR the mod never forces third person; a leash that needs it fails and is narrated.
- Spoken lines say "middle length" and so on, so distances are not read as leash types.
- Built on CommonLibVR 4.39.5, so match your Address Library to your game version. Now MIT licensed.
