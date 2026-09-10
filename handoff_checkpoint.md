# Handoff checkpoint

Date: 2026-09-02. Status: **planning complete, nothing implemented.** Pick this up in a new chat.

**Read first:** `c:\Users\bhuff\.cursor\plans\leash_subject_narration_458a6442.plan.md`

This file is the locked decisions, implementation traps from the last review, and where to start. Do not re-litigate the design unless the user asks.

---

## What this work is

Rework SkyrimNet leash actions so the YAML sentence decides who acts, add DOM-style tie-to-point, take, and give, and make DirectNarration name the acting actor plus the leash type.

Reference implementations:

- SexLab speaker injection: `../skyrimNet_SexLab/Scripts/Source/SkyrimNet_SexLab_Actions.psc` (`type: speaker` as initiator).
- DOM apply: `../PAH Diary Of Mine/Scripts/Source/DOM_Keys.psc` `DOMDoAttachLeash` (bones, left-hand, `ApplyLeashAtPosition` offsets).
- Native API: `Headers/LeashFramework.psc` (do not vendor or recompile).

---

## Decisions locked in

- **Subject-first.** YAML swaps who fills `subject` / `leashed` / `holder`. No per-sentence Papyrus wrappers. Plugins may pass three different actors.
- **Holder required on `LeashedToHolder`.** Skip if `holder == None`. No equip-only state: an actor outside `LeashedFaction` is invisible to every decorator.
- **Not player-centered.** Any actor can hold or be leashed. Every actor holder uses `ApplyLeashToHand` (left). Do not branch on `Game.GetPlayer()`.
- **`tiePoint`, not `location`.** `Location` is a Papyrus type (case-insensitive).
- **Tokens:** type `chain|rope|magic`, body part `neck|waist`, distance `tight|short|middle|long`, style `forcefully|normally|gently`.
- **`holder_shield` is detect-only.** Keep `DetectType` / `ArmorForType` / `ApplyHolderOwnedLeashToBone` so existing shield leashes still unequip. Do not offer it as an LLM type.
- **Do not `DisconnectLeash` before re-apply.** Framework replace already sends `OnUnleash`; an extra disconnect double-narrates.
- **Take / Give reuse `DetectType`**, never hardcoded `rope`/`neck`/`middle`, so a DOM-applied mesh is not swapped.
- **Three categories** (one `customCategory` per action):
  - `leash_leash` — start a leash (holder apply, create-via-tie, refuse)
  - `leash_change` — take, give, re-tie an existing leash
  - `leash_unleash` — remove
- **`LeashedToTiePoint` is duplicated** across `leash_leash` and `leash_change` (same Papyrus function, different YAML `name`, target list, eligibility).
- **Third-party fallback narration** stays on `LeashFramework_OnLeash` / `OnUnleash` / pull. Subject = holder from `GetLeashHolder`, or no subject when world-anchored. Suppress our own applies with a **per-actor timestamp** (copy `LastPullTimes`), not a shared bool.

---

## Papyrus API

```papyrus
Function LeashedToHolder(Actor subject, Actor leashed, Actor holder, String style, String leashDistance, String leashType, String body_part)

Function TakeLeash(Actor subject, Actor leashed)

Function GiveLeash(Actor subject, Actor leashed, Actor receiver)

Function LeashedToTiePoint(Actor subject, Actor leashed, String style, String leashDistance, String leashType, String body_part, String tiePoint)

Function LeashedRefused(Actor subject, Actor leashed)
```

Skip: `leashed == None`; `holder == None` on holder-apply; `receiver == None` on give; holder/receiver equals leashed; give where `receiver == subject` (that is Take).

Apply: `ApplyLeashToHand(holder, leashed, parentBone, "Leash1", min, max, true, false)`.

Tie: `ApplyLeashAtPosition` using DOM offset math in `DOMDoAttachLeash`, then `NotifyLeash(None, leashed)`.

Unleash: keep the two existing executes, rename `akActor` → `subject`.

---

## Action YAML

Parent files: `leash_leash_.yaml`, `leash_change_.yaml`, `leash_unleash_.yaml`. Replace the current children; delete orphans (`leash_leash_target`, `leash_leash_speaker`, and their prompts).

**`leash_leash`** (targets unleashed; create)

- `[Speaker] [style] leashes [Target]'s [body_part] with a [distance] [type] leash` → `LeashedToHolder(Speaker, Target, Speaker, …)` — `get_nearby_unleashed_actors` / `unleashed_nearby`
- `[Target] [style] leashes [Speaker]'s [body_part] with a [distance] [type] leash` → `LeashedToHolder(Target, Speaker, Target, …)` — `get_nearby_actors` / `speaker_is_leashed == unavailable`
- `[Speaker] [style] ties [Target]'s [body_part] to the [tiePoint] with a [distance] [type] leash` → `LeashedToTiePoint` — unleashed list (must send type + body_part; no mesh yet)
- Optional symmetry (not in the plan list, add if wanted): `[Target] ties [Speaker]…` under this category with `speaker_is_leashed == unavailable`
- `[Speaker] refused to be leashed by [Target]` → `LeashedRefused(Target, Speaker)` / speaker not leashed
- `[Target] refused to be leashed by [Speaker]` → `LeashedRefused(Speaker, Target)` / `unleashed_nearby`

**`leash_change`** (existing leash)

- Take / Give / re-tie as in the plan. Unique YAML names, e.g. `leash_change_tie_target` vs `leash_leash_tie_target`.
- Re-tie still **maps every Papyrus slot**. SkyrimNet is positional: do not omit `style` / `leashType` / `body_part`. Use empty `static` values; Papyrus `DetectType` when blank.
- **Parent eligibility is OR:** `collared_nearby` OR `speaker_is_leashed`. A parent that only checks nearby collared hides “Target takes Speaker’s leash” when nobody else is wearing one.

**`leash_unleash`** — existing children, subject-first rename.

Give: prompt must say never pick the leashed actor as `receiver`; Papyrus skips `receiver == leashed` and `receiver == subject`.

---

## DirectNarration

Originator = `subject`. Listener = `leashed` if different, else holder. Interpolate **style as a word** (`forcefully` / `normally` / `gently`). Do not reuse `StyleAdverb` (it returns `""` for normal and would produce a double space).

- Apply: `[Speaker] [style] leashes [Target]'s [body_part] with a [distance] [type] leash.`
- Take: `[Speaker] takes [Target]'s leash.`
- Give: `[Speaker] gives [Target]'s leash to [receiver].`
- Tie: `[Speaker] ties [Target]'s leash to the [tiePoint].`
- Refused: `[Leashed] refused to be leashed by [Subject].` Originator = refusing actor.
- Unleash: `[Speaker] unclips the [type] leash from [Target]'s [body_part].`
- Third-party: same apply sentence with holder as subject, or subjectless if world-anchored.

---

## Tokens / mesh

- Distance → `maxLength`: tight 80, short 150, middle 220, long 300. `minLength` 50.
- `neck` → `NPC Neck [Neck]`; chain `0x806`, rope `0x804`, magic `0x32CE` (confirm in xEdit; DOM uses `0x2CE`).
- `waist` → `NPC Spine1 [Spn1]`; rope `0x800`; chain/magic reuse body rope mesh.
- Bone match `Leash1`.
- Cache needs parallel arrays for type, body part, and distance (not only `CachedTypes`).

Rework, not rename: `NormalizeType`, `TypePhrase`, `ArmorForType`, `DetectType`, `ParentBone`, `NarratePull`.

---

## SKSE (required)

Today’s `leashed_nearby` / `get_nearby_leashed_actors` include **holders**. Add:

| Decorator | Meaning |
| --- | --- |
| `speaker_is_leashed` | `LeashState::IsLeashed(speaker)` only |
| `collared_nearby` | a nearby actor is `IsLeashed` (not holder-only) |
| `get_nearby_collared_actors` | that collared-only list |
| `get_nearby_actors` | nearby living actors excluding speaker (taker / receiver) |

Files: `SKSE_Source/src/SkyrimNet/StateCache.{h,cpp}`, `Registration.cpp`.

---

## Implementation traps (from review)

1. YAML cannot drop Papyrus parameters. Empty `static` + `DetectType` on blank.
2. `leash_change_.yaml` parent: `collared_nearby` **OR** `speaker_is_leashed`.
3. Give receiver list includes the leashed actor unless prompt + Papyrus both exclude them.
4. Unique YAML `name` for the two tie children.
5. Style as a spoken word, not `StyleAdverb`.
6. Confirm magic form ID in xEdit before shipping.

---

## Repo state

Last commit: `01cdb03 fixed bug`. Untracked (pre-existing, not this work):

- `SKSE_Source/src/Leash/LeashState.{h,cpp}`, `SKSE_Source/src/SkyrimNet/{Api,StateCache}.{h,cpp}`, `SKSE_Source/src/Papyrus/`
- `Scripts/Source/SkyrimNet_Leash_Native.psc`, `Scripts/Source/SkyrimNet_Leash_PlayerAlias.psc`
- `SKSE/Plugins/SkyrimNet/config/`, `SKSE/Plugins/SkyrimNet/prompts/leash_actions/`

None of the planned changes have been started.

---

## Compile / logs

- Papyrus: task **`compile: pyro`** only.
- SKSE: **`CMake: Configure (Release)`** then **`CMake: Build SKSE (Release)`** (cwd `SKSE_Source`). In-game must use Release (`/MDd` breaks SkyrimNet decorator ABI).
- Logs: `C:\Users\bhuff\OneDrive\Documents\my games\Skyrim Special Edition\SKSE\SkyrimNet_Leashed.log` and `LeashFramework.log`.

---

## First steps tomorrow

1. Confirm magic leash form ID in xEdit (`Leash.esm` `0x32CE` vs `0x2CE`).
2. Rework token helpers in `Scripts/Source/SkyrimNet_Leash_Actions.psc`.
3. Five entry points + execute narration + per-actor event suppression.
4. SKSE decorators, then YAML/prompts against them.
5. README action table; `compile: pyro` + Release SKSE.
