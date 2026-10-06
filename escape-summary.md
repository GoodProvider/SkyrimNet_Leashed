# Unforgiving Devices escape — agent notes

Inspiration dump for SkyrimNet_Leash's escape system. **Do not port UD.** Steal tensions, not the QTE loop.

Human-readable twin: [escape-human.md](escape-human.md). Vendor tree: `UnforgivingDevices/` (IHateMyKite). Official docs: https://ihatemykite.github.io/Documentation/index.html#

## Key UD files

| File | Role |
| --- | --- |
| `UnforgivingDevices/Scripts/Source/UD_CustomDevice_RenderScript.psc` | Device HP, minigames, accessibility, NPC evaluate |
| `UnforgivingDevices/Scripts/Source/UD_NPCInteligence.psc` | Motivation, cooldown, AI gates |
| `UnforgivingDevices/Scripts/Source/UDCustomDeviceMain.psc` | Difficulty mods, crit QTE, helper CD |
| `UnforgivingDevices/Scripts/Source/UD_WidgetControl.psc` | Dual meters |
| `UnforgivingDevices/Scripts/Source/UD_SkillManager_Script.psc` | `AGIL` / `STRN` / `MAGK` / `CUTT` / `MAIN` |
| `UnforgivingDevices/Scripts/Source/UD_UserInputScript.psc` | Struggle hotkey → device menu |
| `UnforgivingDevices/Scripts/Source/UD_Modifier_Loose.psc` | Struggle with tied hands |
| `UnforgivingDevices/Scripts/Source/UD_Modifier_Sentient.psc` | Device fights back on crit fail |
| `UnforgivingDevices/Scripts/Source/UD_ModOutcome_RestoreDurability.psc` | Mend |

## Mental model

UD: restraint has **HP**. Session drains stats and durability. Durability 0 → escape. Stats floor → exhausted, progress kept.

Leash already inverted this: `weakness` 0→100, break at `> 100`. Keep that polarity.

## Two meters

| UD | Field | Leash analog |
| --- | --- | --- |
| Durability | `current_device_health` / `UD_Health` (default 100 + level scale) | `weakness` (inverted) |
| Condition | `UD_condition` 0 Excellent / 1 Good / 2 Normal / 3 Bad / 4 destroy | missing — fray that Strengthen should not fully erase |

Condition also buffs later struggle (`getModResistPhysical` / `getModResistMagicka` add ~10% per level). Widget colors: green → yellow-green → orange → red. Leash HUD bands today: yellow ≤30, orange ≤60, red >60, remainder blue (`PrismaUI/views/SkyrimNet_Leash/hud.html`).

Struggle types treat condition differently (`struggleMinigame`):

- type 0 Normal: `_condition_mult_add = -0.9` (barely frays)
- type 1 Desperate: `-0.5`
- type 2 Magic: `+1.5` (frays fast)

## Methods (`struggleMinigame` / lock / cut)

Device is escapable if `UD_durability_damage_base > 0` **or** lockpickable locks (`isEscapable()`). Accessibility is a separate gate (`canBeStruggled`).

| `aiType` | Method | Cost | Skill |
| --- | --- | --- | --- |
| 0 | Normal | Stamina | `AGIL` |
| 1 | Desperate | Stamina + Health; DPS rises as durability drops | `STRN` |
| 2 | Magic | Stamina + Magicka | `MAGK` |
| 3 | Slow | Tiny DPS, **no crits, no exhaustion** | physical resist only |
| 5 | Useless | Flavor, no damage | — |
| — | Cutting | Stamina + Health; needs `UD_CutChance > 0` | `CUTT` |
| — | Lockpick / key / repair jam | Stamina; locks can jam or time-lock | `MAIN` on repair |

NPC `EvaluateNPCAI()`: 50% locks-first vs struggle-first, then walk remaining methods. Random struggle subtype `RandomInt(0,2)`.

**Kind mapping:** rope → normal/desperate/cut; chain → desperate + physical resist; magic → type 2 (Leash already contests Magicka).

Leash today: one action `leash_speaker_escape` → `AttemptEscapeExecute`. UD lesson: **one restraint, several methods with different costs.** Slow struggle is the best fit for walking-on-a-leash.

## Minigame loop — do not port

`minigame()`: animation, AV drain per tick, durability ` (base + add) * dt * damageMult `. `UD_DamageMult` starts as `getAccesibility()`. AV below `_minMinigameStat*` or NPC combat → stop. Success: unlock, skill XP, `UpdateMotivation(+50)`. Fail: exhaustion, NPC `UpdateMotivation(-5)` if elapsed ≥ 2s.

Crits (`StruggleCritCheck`, ~15%/s, duration 0.5–1.2s): press matching stamina/magicka key. Hit: `UD_StruggleCritMul` (default 3.75×) + 1.25× AV restore. Miss: sentient may activate; lockpick/key can jam. NPCs/AutoCrit are simulated rolls.

**Do not** add QTE, vanilla lockpick, orgasm-while-struggling, gold-per-attempt, or iWant Widgets. PrismaUI is the HUD.

If a crit analog is wanted: extra weakness spike on a good attempt, or LLM intent desperate vs careful — not a timed key.

## Accessibility (`getAccesibility`)

- Hands free → 1.0
- Mittens → 0.5
- Hands tied, no helper, not Loose → **0.0** (no roll)
- Loose modifier → still allowed at looseness %

Unlocking some locks adds up to +300% struggle DPS (`_getLockMinigameModifier`). Time-locked locks block lock minigames.

Leash analog: tied / dangling / holder-present should be a **multiplier**, not only a decorator boolean. Self-escape vs third-party is already split (`leash_speaker_escape` vs `leash_target_unleash`).

## Helpers (`*MinigameWH`)

Helper adds skill to DPS, pays their own AVs, free hands +0.15–0.4 damage, help XP, per-pair CD (`UD_MinigameHelpCd` default 60 min game time, −10%/helper level). Exhaustion on both.

Leash **Strengthen** is the inverse (reset weakness). UD also has **help-to-escape**: add progress without full disconnect. Stronger analog than only `leash_target_unleash`.

## NPC AI (`UD_NPCInteligence`)

- Motivation 100 → try every 30 min game time. 200 → 15 min. 0 → never.
- Gates: alive, has devices, not combat, no weapon, not sneaking, not in minigame, not following player, stamina ≥ 90%, cooldown elapsed.
- `GetAiPriority()`: heavy bondage 75, belt 35, hood 30, gag 26, generic 25, vibrator 15 (40 while on). Comfortable (`CMF`) lowers priority.

Leash `EscapeCooldown = 2.0` real seconds is anti-spam, not pacing. Prefer **game-time + motivation** for LLM NPCs so `leash_speaker_escape` is not every turn.

## Exhaustion vs cooldown

`addStruggleExhaustion` → magic effect, tired expression, `UD_DeviceExhaustionNum` stacks. Slow/useless skip it.

Leash: 2s debounce + `IdleNervous`. Cheap steal: drain the kind's AV on attempt, gate next try on recover. Magic already uses Magicka; rope/chain use Stamina (`EscapeStrength` in `Scripts/Source/SkyrimNet_Leash_Actions.psc`).

## Mend vs Strengthen

`mendDevice`: `time * strength * (1 - 0.1 * condition) * getMendDifficultyModifier()`. Worse condition → slower mend. Harder MCM escape → faster mend.

`StrengthenLeashExecute` currently sets weakness to **0**. Prefer partial restore, slower if frayed, rope mends faster than chain, magic may not mend without attention.

## Skills / difficulty

`getSkillsPerc` feeds DPS. Skills advance on successful escape. Device level scales `UD_Health`.

```
getStruggleDifficultyModifier:
  1.0
  + (0.6 - 0.15 * DDescapeDifficulty)   // if DD linked
  + 0.25 * (1 - UD_StruggleDifficulty)
```

Leash: `strength = max(3, baseAV/10)` vs `kindStrength` (rope 1, chain 3, magic 10 from manifest). Optional: hidden leashcraft skill; print resist the way UD prints "Very resistant / Almost immune".

## Current Leash surface (keep)

| Piece | Where |
| --- | --- |
| Manifest toggles / kind strength | `SKSE/Plugins/SkyrimNet/config/plugins/SkyrimNet_Leash/manifest.yaml` (`leash.escape.*`) |
| YAML | `leash_speaker_escape.yaml`, `leash_change_strengthen.yaml`, `leash_target_unleash.yaml` (top-level, no category parent, no `intent`) |
| Decorators | `speaker_can_escape`, `weakened_leash_nearby` in `SKSE_Source/src/SkyrimNet/Registration.cpp` |
| Execute | `AttemptEscapeExecute` / `StrengthenLeashExecute` in `Scripts/Source/SkyrimNet_Leash_Actions.psc` |
| Formula | `gain = strength * RandomInt(1, strength) / typeStrength`; break `weakness > 100` |
| Cache / chance / bands | `SKSE_Source/src/SkyrimNet/StateCache.cpp` (`EscapeChance`, `WeaknessBand`) |
| Weakness store | `LeashState::SetWeakness` |
| HUD | `PrismaUI/views/SkyrimNet_Leash/hud.html`; push from `WebUI.cpp` |
| Bio | `SKSE/Plugins/SkyrimNet/prompts/submodules/character_bio/0409_leashframework.prompt` |
| Player hotkey | `leash.escape.hotkey` default 161 (Right Shift) → `AttemptEscapeExecute` |

Category YAML rule: root escape/unleash have **no** parent. Do not invent `leash_escape_.yaml`. Category eligibility must not be stricter than children.

## Steal / skip

**Steal (priority):**

1. Short timed struggle (drain AV, weakness ∝ time survived) instead of a single tap.
2. Multiple methods: desperate / careful / magic — LLM-selectable or from intent.
3. Condition/fray separate from weakness so Strengthen is not a full undo.
4. Helper struggle that *adds* weakness.
5. Motivation + game-time cooldown for NPCs.
6. Exhaustion as attempt cost.
7. Accessibility multiplier (tied / dangling / holder nearby).
8. Sentient magic-leash on fail (pull, magicka burn).

**Skip:** crit QTE, lockpick menu, plug/inflate minigames, orgasm-during-struggle, Demanding gold, iWant Widgets, lock subsystem unless you invent a "knot/clasp" that must loosen before weakness can finish.

## Design brief

> Escape is persistent **weakness + fray**. Each attempt is a short, stat-draining struggle whose method matches leash kind. Holders **mend**, they do not hard-reset. Helpers speed escape. NPCs try on a **motivation clock**, not every dialogue turn.
