## Unreleased

- Leashing a freshly captured Diary of Mine slave no longer turns into a tug-of-war between the leash and DOM's pose (needs the matching SkyrimNet DOM update). The leash no longer tries to "unstick" an NPC that DOM is posing, tying, or holding cowering.
- NPCs who are already leashed are not leashed again every few lines.

## [0.4.4](https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/0.4.4)

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
