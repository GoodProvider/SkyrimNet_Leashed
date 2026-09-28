# Checkpoint: escape minigame handoff

**Status:** one-shot tap (`leash_speaker_escape` / weakness HUD) was **removed from this release**. Minigame designed, **not implemented**. Current game has flavor category `leash_escape` with child `leash_escape_struggle` (animation + DirectNarration; the leash never comes off). Implement this checkpoint from scratch; do not restore `AttemptEscapeExecute`.

**Approved plan:** Cursor plan `escape_minigame_de51e7a7` (also summarized below). Implement that; do not re-litigate lockpick UI.

**Repo:** `c:\Skyrim\dev\mods\SkyrimNet_Leashed`  
**Prior chat:** Unforgiving Devices inspiration → kind-specific methods → category YAML + shared minigame → player dialogue + vanilla lockpick + ragdoll abort.

Read first: [AGENTS.md](../AGENTS.md), [llms.txt](../llms.txt), this file, then [escape-summary.md](../escape-summary.md) (UD notes). Human twin: [escape-human.md](../escape-human.md).

---

## What ships today (do not break)

One-shot escape already works:

| Piece | Where |
| --- | --- |
| YAML | [`SKSE/Plugins/SkyrimNet/config/actions/leash_speaker_escape.yaml`](../SKSE/Plugins/SkyrimNet/config/actions/leash_speaker_escape.yaml) — **delete when category lands** |
| Execute | `AttemptEscapeExecute` in [`Scripts/Source/SkyrimNet_Leash_Actions.psc`](../Scripts/Source/SkyrimNet_Leash_Actions.psc) (~1067) |
| Formula | `gain = strength * RandomInt(1, strength) / typeStrength`; break at `weakness > 100` |
| Strength | `max(3, baseAV/10)`; magic = Magicka, else Stamina |
| Decorators | `speaker_can_escape`, `weakened_leash_nearby` — [`Registration.cpp`](../SKSE_Source/src/SkyrimNet/Registration.cpp) |
| Cache | [`LeashState.h`](../SKSE_Source/src/Leash/LeashState.h) `weakness` only |
| HUD | [`PrismaUI/views/SkyrimNet_Leash/hud.html`](../PrismaUI/views/SkyrimNet_Leash/hud.html) |
| Hotkeys | Panel `leash.controls.hotkey` default **220** (`\`); struggle `leash.escape.hotkey` default **161** (Right Shift) — [`WebUI.cpp`](../SKSE_Source/src/WebUI/WebUI.cpp) `OnEscapeStruggle` already **only if player is leashed**, skips panel key and keyboard Escape |
| Config | [`manifest.yaml`](../SKSE/Plugins/SkyrimNet/config/plugins/SkyrimNet_Leash/manifest.yaml) `leash.escape.*` |
| Bio | [`0409_leashframework.prompt`](../SKSE/Plugins/SkyrimNet/prompts/submodules/character_bio/0409_leashframework.prompt) |
| Narration | `Narrate(..., "optional")` = DirectNarration if player can see **and** speech queue empty; success uses required `DirectNarration` |
| Strengthen | `leash_change_strengthen` still **hard-resets weakness to 0** |
| Unleash target | `leash_target_unleash` stays a **root** action |

Cooldown: `EscapeCooldown = 2.0` real seconds. Anim: `IdleNervous` only.

Category precedent: [`leash_leash_.yaml`](../SKSE/Plugins/SkyrimNet/config/actions/leash_leash_.yaml), [`leash_change_.yaml`](../SKSE/Plugins/SkyrimNet/config/actions/leash_change_.yaml) — `intent`, no `scriptName`.

ESP source of truth: [`Spriggit/SkyrimNet_Leash/`](../Spriggit/SkyrimNet_Leash/) (quest only today). Serialize: `dotnet tool run spriggit deserialize` is in README; **serialize after Spriggit edits**.

Compile: Papyrus task **`compile: pyro`**. SKSE **Release** for in-game (Debug `/MDd` breaks SkyrimNet decorator ABI).

---

## Decisions already made (do not reopen)

1. **Do not port UD’s QTE**, orgasm-struggle, gold-per-attempt, iWant Widgets, or SexLab anim packs. Steal tensions.
2. Keep weakness polarity **0→100, break >100**.
3. **Player lockpick = vanilla Lockpicking menu.** NPCs never open it (silent AV roll). PrismaUI lockpick was offered and rejected.
4. Escape is a **category** `leash_escape` with `intent`. This **overrides** current AGENTS.md “do not invent a parent for Escape.” Update AGENTS.md when implementing.
5. **Give up is a root** (`leash_giveup.yaml`), not a category child (parent is “not in minigame”).
6. **Do not gate the category on `is_leash_available`** (combat). Escape works in combat.
7. Player and NPC share **one** Papyrus session (same tick, drain, anim, duration).
8. Two hotkeys stay separate: struggle (leashed only) vs full-power **leash panel**.
9. Multiple player methods → **Papyrus `Message.Show()`**, not PrismaUI picker.
10. UD/DD struggle event **names** as optional clips; fallback `IdleNervous`. Do **not** vendor UD HKX / FNIS unless asked later.
11. No `leash_escape` help-to-escape action in this pass (optional later).

---

## Kind mechanics (target)

| Kind | Methods | Extra state | Fail / objects |
| --- | --- | --- | --- |
| **Rope** | `struggle`, `cut` (blade in inventory) | `fray` 0–100 | No lock. Cut faster if frayed. |
| **Chain** | `struggle` (yank, `methodMult` 0.15 while locked), `pick`, `key` | `lock`: `locked` / `jammed` / `open` | Padlock key on **holder** at leash-start; vanilla lockpicks `0x0A`. Pick fail → jam. Key works jammed. |
| **Magic** | `unravel` | none extra | Magicka drain; optional fail spike later (sentient). |

Default hotkey method: unravel / pick if picks+locked / key if has key / else struggle.

---

## Implement this (approved plan)

### YAML

Delete `leash_speaker_escape.yaml`.

| File | Role |
| --- | --- |
| `leash_escape_.yaml` | Category. `speaker_can_start_escape`. `intent`. |
| `leash_escape_struggle.yaml` | static `method: struggle` → `StartEscapeExecute` |
| `leash_escape_cut.yaml` | `cut` |
| `leash_escape_pick.yaml` | `pick` |
| `leash_escape_key.yaml` | `key` |
| `leash_escape_unravel.yaml` | `unravel` |
| `leash_giveup.yaml` | Root. `GiveUpEscapeExecute`. `speaker_in_escape_minigame`. |

Children: `customCategory: leash_escape`, speaker `subject`, static `method`. Prompts under `SKSE/Plugins/SkyrimNet/prompts/leash_actions/`.

`speaker_can_start_escape` = escape on + leashed + kind escapable + **not in session** + **at least one method flag** (empty category is bad).

### State / C++

Extend `RecordedPair`: `fray`, `lock`, `inMinigame`. Papyrus pushes via new native (e.g. `NotifyEscapeState`) or folded notify.

Flags: `speaker_can_start_escape`, `speaker_in_escape_minigame`, `speaker_can_escape_struggle|cut|pick|key|unravel`.

`IsRagdolled(Actor)` native: `RE::Actor::IsInRagdollState()` + knocked-down.

Hotkey: `OnEscapeStruggle` → **`PlayerEscapeHotkeyExecute`**, not `AttemptEscapeExecute`.

### Papyrus session (player and NPC)

Constants: tick **1.0 s**, max **8 ticks**, AV floor **25%**.

```
gain_per_tick = (strength * RandomInt(1, strength) / typeStrength) * methodMult * accessMult / 8.0
```

Loop: ragdoll/dead/unleashed → abort (keep weakness); drain AV; apply gain; `Narrate(..., "optional")` with method + %; `weakness > 100` → existing disconnect + **required** DN; AV floor or 8 ticks → exhaust stop; replay anim if it died.

Give up / ragdoll: `IdleForceDefaultState`, keep weakness/fray/lock, optional DN.

`PlayerEscapeHotkeyExecute`: in session → give up; one method → start; several → Message forms (Spriggit).

### ESP (Spriggit)

- Key `SkyrimNet_Leash_PadlockKey` — add to holder on chain apply; remove on unleash/give
- Dummy locked `Container` + REFR in a hidden cell
- Message forms per kind combo + Cancel (+ Give up / Cancel if in session)

Player `pick`: pause ticks, `SetLockLevel` from chain strength (1–100 → 0/25/50/75/100), Activate dummy → Lockpicking Menu. `RegisterForMenu("Lockpicking Menu")`. Unlocked → open + break (or full-rate struggle). Still locked + backed out → no jam. Pick broke → optional jam.

NPC `pick`: Lockpicking AV vs level; consume one pick; fail → jammed.

### Animations (optional names)

Neck: `UD_Solo_Struggle_Collar01` / `_H`, then `DDCollarStruggle01`. Waist: `UD_Solo_Struggle_Corset01`. Wrists: `UD_Solo_Struggle_Arms01` / `DDArmsStruggle01`. Else `IdleNervous`. Stop: `IdleForceDefaultState`.

### Docs when done

AGENTS.md: Escape is a category; Give up + `leash_target_unleash` stay roots. README: two hotkeys, method dialogue, vanilla lockpick. Update escape-summary/human + llms.txt YAML names.

---

## Explicitly out of scope this pass

- PrismaUI lockpick / method list
- Vendoring UD meshes / FNIS
- Help-to-escape pair anims
- Strengthen as partial mend (still full reset unless you have time)
- Magic “sentient pull” VFX
- Slow/fidget/overpower extra methods
- Changing the leash panel

---

## First moves for the new chat

1. State confidence ≥90% before ESP/script/game edits.
2. YAML + prompts + decorator flags (so LLM surface exists).
3. Papyrus session + hotkey dispatch (player/NPC parity).
4. Spriggit key/container/messages + vanilla lockpick.
5. Ragdoll native + tick abort.
6. `compile: pyro`, CMake **Release**, spriggit serialize.
7. In-game: rope struggle/cut, chain pick (player menu vs NPC silent), give up, ragdoll stop, panel hotkey still works unleashed.

Logs: `SkyrimNet_Leash.log`, `SkyrimNet.log` (paths in AGENTS.md).
