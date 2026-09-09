# Escape and struggle — current mechanics

High-level handoff for another mod that wants to match how SkyrimNet_Leash treats self-escape today. This is behavior, not an implementation spec.

Related design (not shipping): [escape-human.md](escape-human.md) / [escape-summary.md](escape-summary.md) (Unforgiving Devices inspiration), [checkpoints/escape-checkpoint.md](checkpoints/escape-checkpoint.md) (future minigame).

## One sentence

A leashed character can **try** to get their own leash off. Trying never succeeds. Someone else can still take it off them. The player can take their own off through the leash panel.

## What is in vs out

| In | Out |
| --- | --- |
| Flavor struggle: looping idle + narration | Actual disconnect from struggle |
| LLM picks “escape” when they mean yank / wriggle / unclip themselves | A real self-unclip LLM action |
| Persistent struggle state until they stop, are yanked, or unclipped (walking keeps it) | Weakness, fray, stamina drain, minigame |
| Third party unclips a nearby collared person | NPC self-unclip |
| Player panel unleash, including the player’s own collar | Routing the panel or its hotkey through struggle |

There is no leash health, no chance roll, and no “it finally comes off after N tries.” Struggle is theater so the LLM has somewhere to put “I want this off.”

## Who can do what

**Struggle (self).** Only the collared person, on their own leash. Holder, dangling, or world-tied does not matter. Combat does not block it. Children can be leashed and can struggle. Several people can struggle at once.

**Stop struggling.** Root action `leash_none_struggle_stop`. Only someone already struggling. They give up. Walking does not count as giving up. The leash stays on. Opposite of Escape; not a child of that category.

**Unclip someone else.** Root action `leash_none_target_unleash`. A nearby actor who is collared (not merely holding a leash). Never the speaker themselves. This is the real remove-leash path for NPCs.

**Unclip yourself.** Not an LLM action. The player still can, via the in-game leash panel: pick themselves as the collared subject and choose unleash. That fully disconnects. Keyboard Escape only closes the panel; it does not struggle and does not unclip.

NPCs cannot pick escape and come off. That is intentional until a later minigame is the self-free path.

## How the LLM sees it

Escape is a category: “get your own leash off.” Copy is allowed to sound like success (yank, wriggle, work the knot, unclip) so the model **selects** it when the character is fighting their collar. Execute still never disconnects.

Rules of the category:

- Always pick it when the speaker struggles or tries to take their own leash off.
- Not for giving up (that is `leash_none_struggle_stop`).
- Not for unclipping someone else.
- Not for handing a leash to anyone.
- Eligible when the speaker is leashed and not already struggling. Do not hide it in combat; struggle does not need a “can start a new leash” gate.

Children: start struggle only (hidden while already struggling). Intent on the category is how they try and why (flavor for selection, unused by execute).

## What happens on a struggle

1. If the speaker is not leashed, nothing.
2. If they are already struggling, nothing.
3. Enter struggle state: loop `IdleNervous` while standing. Walking does not end it; they keep struggling. Yank, unclip, or `leash_none_struggle_stop` does.
4. The leash stays on. No Framework disconnect, no armor unequip.

Animation is always vanilla `IdleNervous`. ZaZ, SexLab, Devious Devices, and Unforgiving Devices are not used.

## Repeat beats while struggling

Tracked **per collared actor**, real time. Still DirectNarration waits for the longer of `leash.escape.narrationInterval` (default 5 seconds) and `leash.escape.cooldown` (default 20 seconds).

**Start:** play the idle and narrate a failed attempt — they struggle against the [rope/chain/magic] leash at their [neck/wrists/waist], but it holds.

**Every second while still struggling:** short-lived `leash` event — `{name} continues to struggle with {her/his/their} leash.` Same event id is refreshed so it does not stack.

**Every wait seconds while still struggling:** optional DirectNarration — `Despite {name}'s attempts, the leash holds.` If the speech queue is busy, that beat is a `leash` event instead.

**Stop struggling:** optional DirectNarration — `{name} stops struggling to remove their leash.` If the speech queue is busy, that beat is a `leash` event instead.

A leash yank or unclip also ends the idle. Those paths already have their own lines, so they do not add a second “stops struggling” sentence.

## Narration contract

Match the rest of leash speech: high importance interrupts so nearby people react; low importance is context without stealing the speech queue. If the player is not involved and cannot see the collared actor, it is always a background event.

| Beat | How it lands |
| --- | --- |
| Start struggle while the player is the strugglers, the collared person, or the holder (or can see them) | Immediate narration |
| Start struggle without the player in the scene | Immediate narration only if the speech queue is empty; otherwise a leash event |
| Stop struggling, or later beats on the interval | Optional DirectNarration if the queue is empty; otherwise a leash event |

Do not invent success text. Struggle lines always fail.

## What actually removes a leash

| Path | Who | Result |
| --- | --- | --- |
| Struggle | The collared speaker | Never removes it |
| Stop struggling | The collared speaker already struggling | Leaves the idle; leash stays on |
| Unclip other | Speaker targeting a nearby collared actor | Disconnect that pair |
| Player panel unleash | Player; subject may be themselves | Full disconnect, including self |

Unclip-other refuses if the speaker is the collared person. Panel unleash does not refuse that case.

## What a port should preserve

- Struggle is selectable and visible; it is not an escape.
- Category copy may promise freedom; the outcome must not.
- Self-remove is player-panel only, not LLM.
- Third party unclip stays a separate, real action.
- Combat must not hide struggle.
- Struggle is a state, not a one-shot clip. Repeat LLM picks must not spam; the interval is the extra narration.
- Stop struggling (`leash_none_struggle_stop`) is a root action, opposite of Escape. Walking does not end the state.

## What a port should not add (this surface)

No weakness bar, no kind-strength roll, no strengthen/mend, no helper-to-escape, no give-up minigame, no player struggle hotkey, no routing panel unleash through struggle. Those belong to the later minigame, not current escape.
