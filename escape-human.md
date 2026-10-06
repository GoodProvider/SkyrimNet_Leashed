# How Unforgiving Devices handles escape

A plain-language look at [Unforgiving Devices](https://github.com/IHateMyKite/UnforgivingDevices) so we can steal *ideas* for SkyrimNet_Leash — not the whole minigame.

Technical twin for agents: [escape-summary.md](escape-summary.md).

## The one-sentence version

In Unforgiving Devices, a restraint is not a lock you either pick or don't. It has a health bar. You wear it down over several tries, you get tired doing it, and someone else can tighten it back up.

Leash already works that way, just inverted: the leash starts at 0% weakness and breaks at 100%.

## What a struggle feels like

You open the device, pick *how* you want to fight it, and a short session starts. You (and maybe a helper) spend stamina, health, or magicka. A bar on screen moves. If the bar empties, you are free. If you run out of juice first, the session stops, you are exhausted, but the progress you made is still there for next time.

That is the important difference from a single dice roll: **trying costs something, and leftover progress matters.**

## Two numbers, not one

Unforgiving Devices tracks:

1. **How tight it is right now** (durability). This is the bar you are fighting this session.
2. **How beaten-up it is overall** (condition: excellent → good → normal → bad → destroyed). This is the long game. A frayed device is easier to fight later, and it mends more slowly.

Leash currently only has weakness. Tighten (`Strengthen`) slams that back to zero. A second "fray" number would mean the holder can snug the leash without magically undoing every yank.

The color language already matches our HUD: green/yellow when things are fine, orange when it is serious, red when it could go at any moment.

## Several ways to fight the same thing

You do not always panic-yank.

| Approach | Feel | Cost |
| --- | --- | --- |
| Normal struggle | Steady work | Stamina, agility |
| Desperate | Harder the more it has already given; brute force | Stamina **and** health, strength |
| Magic | Spell the bindings apart | Magicka |
| Slow / careful | Tiny progress, you don't exhaust yourself | Almost nothing |
| Cutting | If the material can be cut | Stamina and health |
| Locks | Pick, use a key, or repair a jammed lock | Stamina, and you can jam it worse |

For leashes that maps naturally:

- **Rope** — normal yank, desperate yank, or cut.
- **Chain** — desperate, slow, expensive.
- **Magic** — magicka contest (we already do this).

The sleeper idea is **slow struggle**: working the leash while you walk, instead of a panic button every two seconds.

We only have one escape action today. The interesting upgrade is not more YAML for its own sake — it is letting the speaker choose *how hard* they try.

## Other people

Unforgiving Devices treats help as a first-class thing. A second person makes progress much faster, pays their own stamina, and then needs a rest before they help again.

We already have the opposite: someone else can **tighten** a weakened leash, or fully unleash a third party.

The missing piece is **help-to-escape** — an ally working the knot with you, adding weakness, without instantly setting them free. That fits "hold still, I'll get this off you" better than a full unleash.

## NPCs do not try every thirty seconds

NPC wearers have **motivation**. Default: they try about every 30 minutes of game time. After a success they get hungrier to keep going. After a failed, exhausting try they back off a little. They will not start if they are in combat, sneaking, following you, or almost out of stamina. They also pick *which* device to fight first (heavy bondage before a gag; a "comfortable" device they may ignore).

Our escape key has a 2-second real-time debounce so the player cannot spam it. That is fine for the hotkey. For LLM NPCs, a slower "I have been working this for a while" clock would feel less like they try every line of dialogue.

## Trying should tire you

When a Unforgiving Devices session ends without freedom, you are exhausted: slower, tired face, stacked fatigue. Careful slow struggle skips that on purpose.

We play `IdleNervous` and wait two seconds. A closer match would be: the attempt actually spends stamina (or magicka on a magic leash), and you cannot try again until that recovers. The leash type already picks which pool to use.

## The holder is not a reset button

Devices can **mend** on their own. A badly frayed one mends slower. Harder difficulty makes escape slower *and* mend faster.

`Strengthen` today wipes weakness to zero. A Unforgiving-Devices-like holder would snug it some, not all — faster on rope, slower on a chewed-up chain, maybe not at all on magic without real attention.

## What we already have (keep)

- Escape can be turned off entirely.
- Rope / chain / magic each have an "escapable" toggle and a 1–100 strength.
- Weakness 0–100; over 100 you pop free.
- Player hotkey (Right Shift) and an LLM "I try to escape" action.
- Someone else can tighten a weak leash, or unleash a person who is not themselves.
- Bios mention how loose it looks and a rough chance on the next try.
- A non-pausing weakness bar while the player is leashed.

That is already the Unforgiving Devices *shape*. The rest is pacing and texture.

## Ideas worth considering (not a promise)

In rough order of payoff:

1. A short "keep struggling until you run out of breath" beat, instead of one tap of weakness.
2. Desperate vs careful vs magic as different costs, not different menus.
3. Fray that survives a tighten.
4. An ally helping you loosen, not only fully unleashing you.
5. NPCs who wait, then try, instead of trying every reply.
6. Exhaustion as the price of a yank.
7. Tied / dangling / holder-right-there changing how easy it is, not only yes/no.
8. A magic leash that *pulls back* when you fail.

## What we should not copy

The timed "hit the flashing stamina bar" crit game. The Skyrim lockpick menu. Gold to even *try*. Sex/orgasm as part of struggling. A separate widget mod. A whole lock-and-key system unless we later decide a rope has a knot that must come undone first.

## Design brief

Escape is a leash getting weaker and more frayed. Each try is a short, tiring struggle that matches the leash type. The person holding it snugs it back; they do not rewind time. Friends can help. People on a leash work at it over the afternoon, not every sentence.
