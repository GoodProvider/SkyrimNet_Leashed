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

Pass None for holder to record a holderless leash. tied true is a world-position anchor;
tied false is a dangling (unheld) collar. A non-None holder ignores tied.

kind: chain, rope, magic, or holder_shield. leashDistance: tight, short, middle, or long.
bodyPart: neck, wrists, or waist. Empty strings are allowed when the values are not yet known.
/;
Function NotifyLeash(Actor holder, Actor leashed, String kind, String leashDistance, String bodyPart, Bool tied) Global Native

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

;/
Maximum leash length in game units for tight, short/close, middle, or long.
Reads the SkyrimNet plugin config; unknown tokens use middle.
/;
Float Function DistanceMax(String leashDistance) Global Native

;/
Maps a measured maxLength back to tight, short, middle, or long by nearest
configured length.
/;
String Function DistanceFromLength(Float maxLength) Global Native

;/
Logs every third-person node whose name contains "Leash", plus how many of those
sit under NPC Neck, NPC Spine1, and NPC Spine2. Used to diagnose Framework bind.
/;
Function TraceLeashBones(Actor who) Global Native
