https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/0.4.0

- LLM actions and prompts ship in both the Beta 25 plugin folder and the older SkyrimNet folders, so this version still works on pre-0.25 SkyrimNet.
- On SkyrimNet 0.25+, plugin `goodprovider.leashed` should show under Installed Plugins with an **External** badge.
- Action names changed from `leash_*` to `leashed_*` (Leash someone, Get leashed, Change, Escape, Unleash, stop struggling). If you customized enabled/cooldown for the old names, set them again once.
- Start-leash is two groups: collaring someone else, and being collared (including the player).
- Do not use **Plugins > Import Old Content** for this mod. An imported copy hides later updates of ours.
- Hotkey, distances, and struggle toggles stay in **Settings → Plugins → SkyrimNet_Leashed**. Those keys did not change.
- Taking a leash off keeps a collar copy they already owned and only removes extras the mod added to wear it.
- A leash is only applied once the collar has leash bones. First-person view on the player skips apply and asks you to switch to third person.
- If the collar mesh never attaches (Argonian/Khajiit or a missing mesh), the pair is skipped and narrated instead of an invisible leash.
