Scriptname SkyrimNet_Leash_Native Hidden

;/
Native helpers implemented by SkyrimNet_Leash.dll.

LeashFramework exposes its leash table only through Papyrus, so the SKSE side cannot ask
it who holds a given leash. These functions let the Papyrus side push the authoritative
holder reported by LeashFramework.GetLeashHolder into the plugin, which serves it to
SkyrimNet decorators and character bios.
/;

;/
Records holder as the actor holding leashed's leash, plus the kind, distance, and body-part
tokens used in character bios.

Pass None for holder to record a holderless world-position leash. That is stored as a
known holderless leash rather than as an unknown one, so the plugin does not fall back to
guessing a nearby holder.

kind: chain, rope, magic, or holder_shield. leashDistance: tight, short, middle, or long.
bodyPart: neck or waist. Empty strings are allowed when the values are not yet known.
/;
Function NotifyLeash(Actor holder, Actor leashed, String kind, String leashDistance, String bodyPart) Global Native

;/
Drops the recorded holder for leashed.
/;
Function NotifyUnleash(Actor leashed) Global Native

;/
Folds value to lower case and collapses every run of spaces, tabs, hyphens, and
underscores into a single underscore, with leading and trailing separators removed.

Used to normalize the free-text style and leash type tokens supplied by the LLM.
/;
String Function NormalizeToken(String value) Global Native
