# 02 — TargetMenu offers unleash / let hang for an untracked dangling collar

Repo / branch / baseline: SkyrimNet_Leashed `main` @ `f87e94c` (0.5.1). All changes are uncommitted, including those from 01. Plan: `C:\Users\bhuff\.claude\plans\skyrimnet-sexlab-targetmenu-leash-for-greedy-marshmallow.md`.

Logs: [logs/2026-11-17-08-36-56/](logs/2026-11-17-08-36-56/) (`MANIFEST.txt`, SkyrimNet_Leashed, LeashFramework, SkyrimNet_SexLab, Papyrus.0). The snapshot is from the 2026-11-17 08:22 session. The 2026-11-15 15:14–15:19 session that showed the problem was read in place and has since been overwritten.

## Goal

The SexLab TargetMenu leash panel for Coralyn listed only `tie to` / `give to`. The user confirmed she wears a dangling collar. The panel should also list `unleash` and `let hang`.

## Findings

- 2026-11-15 logs (read in place):
  - LeashFramework logged `Loaded 0 persistent leash(es)`.
  - Papyrus had no `[SkyrimNet_Leashed]` traces.
  - Coralyn is a DOM slave (`DOM_SlaveManager: UpdateActorArray Recovered actor index 0 name = Coralyn`).
- The 2026-11-17 snapshot looks the same: [LeashFramework.log](logs/2026-11-17-08-36-56/LeashFramework.log) has 0 persistent leashes, and [Papyrus.0.log](logs/2026-11-17-08-36-56/Papyrus.0.log) has no Leashed traces.
- Cause, from reading code:
  - `leashed.js` offers `unleash` / `let hang` only when `isLeashed` is true.
  - `LeashStatus` checks LeashFramework, then native `LeashState::IsLeashed` (the faction or a recorded pair).
  - A Leash.esm mesh the plugin never recorded matches neither check, so the status is `''`. The most likely source is DOM's magic collar `0x2CE`; a leftover from the old unleash bug is also possible.
  - `UnleashTargetExecute` and `LetLeashHang` skipped such an actor too.

## Results

- [SkyrimNet_Leashed_Actions.psc](../../Scripts/Source/SkyrimNet_Leashed_Actions.psc):
  - New Global `WearsLeashArmorGlobal`, which checks Leash.esm `0x806`/`0x2CE`/`0x804`/`0x800`/`0xD69`/`0x1`/`0x3`. `WearsLeashArmor` now wraps it.
  - `LeashStatus` returns `"dangling"` when the native status is `''` and the actor wears leash armor.
  - `UnleashTargetExecute`: new last resolve branch `WearsLeashArmor(target)` → `untracked`. It unequips through `UnequipPairArmor(None, …)` and narrates the normal unclip line.
  - `UnleashSpeakerExecute`: the same for the player's own untracked mesh. It reuses the `cacheMiss` unequip and narration path.
  - `LetLeashHang`: an untracked collar logs `LetLeashHang skipped: leash already hangs from X (untracked)` and does nothing. It does not adopt the pair.
- [leashed.js](../../PrismaUI/views/SkyrimNet_SexLab/panels/leashed.js): new `statusDangling`. The status row shows `leash: dangling`. The verb tree is unchanged, because `isLeashed` is already true for `dangling`.
- The three changelogs and KNOWLEDGE.md ("Holder-less unleash" section) are updated.
- Build: `compile: papyrus` passed (3 files). No DLL change. `make release` was not run.

## Decisions

- `let hang` does not adopt an untracked collar, so a collar DOM put on stays DOM's.
- `unleash` takes an untracked collar off. If DOM equips it again, DOM owns that collar.
- Out of scope: the Magelight hotkey panel and the LLM decorators still use native `LeashState::IsLeashed`, so they don't see an untracked collar. A fix would add the same worn-armor check in `LeashState.cpp`.

## Not yet tested

Nothing has been tested in game, including everything from 01.

## Next

1. Deploy the new `.pex` and `leashed.js`. Coralyn (dangling collar) → TargetMenu → leash. Expected: a `leash: dangling` row and the verbs `unleash`, `let hang`, `tie to`, `give to`.
2. `let hang` → nothing changes, and `Papyrus.0.log` shows `LetLeashHang skipped … (untracked)`.
3. `unleash` → the collar comes off with one unclip line, and reopening shows only `tie to` / `give to`. Note whether DOM puts the collar back on.
4. Run 01's Next steps 1–4 on a held or tied NPC (regression, plus the `NarrateUnleash late event ignored` check).
5. Capture logs. If the tests pass, commit. Release only when asked.
6. Decide whether the hotkey panel and decorators should also see untracked collars (C++).
