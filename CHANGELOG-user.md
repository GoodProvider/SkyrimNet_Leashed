https://github.com/GoodProvider/SkyrimNet_Leash/releases/tag/0.3.0

- Built for Leash Framework **1.1.3**. Walking while someone holds the leash no longer counts as a yank, so Escape struggle keeps going until a real snap, ragdoll, unclip, or they pick stop.
- Distance tokens now have a **settle** (how close when standing still) and a **catch-up** (when the leash yanks). Defaults are looser than 0.2.0 so “middle” and “long” actually feel different while walking. Reset the plugin-menu length numbers if you want the new catch-up defaults; settle is a new set of fields.
- Wrists is still chain-only. Middle uses the long hand-chain mesh, long uses extra-long; tight and short keep the original hand chain.
- Struggle loops the vanilla nervous idle while standing, then optional Devious Devices clips (neck / wrists / waist) if those animation files are installed and FNIS/Nemesis/Pandora was rebuilt. Walking does not end struggle and skips the nervous idle so they can keep following. Without the DD files, they stay on the nervous idle.
- Unleash takes off the worn leash mesh copy and leaves extra copies you kept in inventory.
- SkyrimNet_SexLab’s Start Sex SkyMessage can open this same leash bar even if the Leashed hotkey is off.
