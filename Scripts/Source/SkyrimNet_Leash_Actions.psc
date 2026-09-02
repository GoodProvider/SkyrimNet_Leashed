Scriptname SkyrimNet_Leash_Actions extends Quest

Float Property PullCooldown = 8.0 Auto Hidden

Actor[] CachedLeashed
Actor[] CachedHolders
String[] CachedTypes
String[] CachedStyles
Float[] LastPullTimes

Event OnInit()
    EnsureCache()
    RegisterLeashEvents()
EndEvent

Function RegisterLeashEvents()
    UnregisterForModEvent("LeashFramework_OnLeash")
    UnregisterForModEvent("LeashFramework_OnUnleash")
    UnregisterForModEvent("LeashFramework_OnActorPulled")
    UnregisterForModEvent("LeashFramework_OnActorRagdollPulled")

    RegisterForModEvent("LeashFramework_OnLeash", "OnLeashFrameworkLeash")
    RegisterForModEvent("LeashFramework_OnUnleash", "OnLeashFrameworkUnleash")
    RegisterForModEvent("LeashFramework_OnActorPulled", "OnLeashFrameworkPulled")
    RegisterForModEvent("LeashFramework_OnActorRagdollPulled", "OnLeashFrameworkRagdollPulled")
EndFunction

String Function ParentBone()
    return "NPC Spine2 [Spn2]"
EndFunction

String Function LeashBoneMatch()
    return "Leash1_1"
EndFunction

; Papyrus requires a literal array size, so this is the one place the cache length lives.
Int Function CacheSize()
    return 32
EndFunction

Function EnsureCache()
    if CachedLeashed.Length != CacheSize()
        CachedLeashed = new Actor[32]
        CachedHolders = new Actor[32]
        CachedTypes = new String[32]
        CachedStyles = new String[32]
        LastPullTimes = new Float[32]
    endif
EndFunction

String Function NormalizeType(String leashType)
    String t = SkyrimNet_Leash_Native.NormalizeToken(leashType)
    if t == "body_rope" || t == "neck_rope" || t == "neck_chain" || t == "magic_rope" || t == "holder_shield"
        return t
    endif
    return "body_rope"
EndFunction

String Function TypePhrase(String leashType)
    String t = NormalizeType(leashType)
    if t == "neck_rope"
        return "neck rope"
    elseif t == "neck_chain"
        return "neck chain"
    elseif t == "magic_rope"
        return "magic rope"
    elseif t == "holder_shield"
        return "shield leash"
    endif
    return "body rope"
EndFunction

String Function StyleAdverb(String style)
    String s = SkyrimNet_Leash_Native.NormalizeToken(style)
    if s == "forceful" || s == "forcefully"
        return "forcefully "
    elseif s == "gently" || s == "gentle"
        return "gently "
    endif
    return ""
EndFunction

String Function ActorLabel(Actor who)
    if who == None
        return "someone"
    endif
    String n = who.GetDisplayName()
    if n == ""
        n = who.GetName()
    endif
    if n == ""
        return "someone"
    endif
    return n
EndFunction

Armor Function ArmorForType(String leashType)
    String t = NormalizeType(leashType)
    Int formId = 0x800
    if t == "neck_rope"
        formId = 0x804
    elseif t == "neck_chain"
        formId = 0x806
    elseif t == "magic_rope"
        formId = 0x32CE
    elseif t == "holder_shield"
        formId = 0xD69
    endif
    return Game.GetFormFromFile(formId, "Leash.esm") as Armor
EndFunction

Bool Function IsHolderOwnedType(String leashType)
    return NormalizeType(leashType) == "holder_shield"
EndFunction

Actor Function MeshOwnerFor(Actor holder, Actor leashed, String leashType)
    if IsHolderOwnedType(leashType)
        return holder
    endif
    return leashed
EndFunction

Function EquipTypeArmor(Actor meshOwner, Armor leashArmor)
    if meshOwner == None || leashArmor == None
        return
    endif
    if meshOwner.GetItemCount(leashArmor) < 1
        meshOwner.AddItem(leashArmor, 1, true)
    endif
    if !meshOwner.IsEquipped(leashArmor)
        meshOwner.EquipItem(leashArmor, false, true)
    endif
EndFunction

Function UnequipTypeArmor(Actor meshOwner, Armor leashArmor)
    if meshOwner == None || leashArmor == None
        return
    endif
    if meshOwner.IsEquipped(leashArmor)
        meshOwner.UnequipItem(leashArmor, false, true)
    endif
EndFunction

Int Function FindLeashedIndex(Actor leashed)
    EnsureCache()
    Int i = 0
    while i < CachedLeashed.Length
        if CachedLeashed[i] == leashed
            return i
        endif
        i += 1
    endwhile
    return -1
EndFunction

Function RememberPair(Actor holder, Actor leashed, String leashType, String style)
    if leashed == None
        return
    endif
    EnsureCache()
    Int i = FindLeashedIndex(leashed)
    if i < 0
        i = FindLeashedIndex(None)
    endif
    if i < 0
        return
    endif
    CachedLeashed[i] = leashed
    CachedHolders[i] = holder
    CachedTypes[i] = NormalizeType(leashType)
    CachedStyles[i] = style
EndFunction

Function ForgetPair(Actor leashed)
    Int i = FindLeashedIndex(leashed)
    if i < 0
        return
    endif
    CachedLeashed[i] = None
    CachedHolders[i] = None
    CachedTypes[i] = ""
    CachedStyles[i] = ""
    LastPullTimes[i] = 0.0
EndFunction

String Function CachedType(Actor leashed)
    Int i = FindLeashedIndex(leashed)
    if i >= 0 && CachedTypes[i] != ""
        return CachedTypes[i]
    endif
    return ""
EndFunction

Actor Function CachedHolder(Actor leashed)
    Int i = FindLeashedIndex(leashed)
    if i >= 0
        return CachedHolders[i]
    endif
    return None
EndFunction

String Function CachedStyle(Actor leashed)
    Int i = FindLeashedIndex(leashed)
    if i >= 0
        return CachedStyles[i]
    endif
    return "normal"
EndFunction

String Function DetectType(Actor holder, Actor leashed)
    Armor shieldArmor = ArmorForType("holder_shield")
    if holder && shieldArmor && holder.IsEquipped(shieldArmor)
        return "holder_shield"
    endif
    if leashed
        Armor chainArmor = ArmorForType("neck_chain")
        if chainArmor && leashed.IsEquipped(chainArmor)
            return "neck_chain"
        endif
        Armor neckArmor = ArmorForType("neck_rope")
        if neckArmor && leashed.IsEquipped(neckArmor)
            return "neck_rope"
        endif
        Armor magicArmor = ArmorForType("magic_rope")
        if magicArmor && leashed.IsEquipped(magicArmor)
            return "magic_rope"
        endif
        Armor bodyArmor = ArmorForType("body_rope")
        if bodyArmor && leashed.IsEquipped(bodyArmor)
            return "body_rope"
        endif
    endif
    String cached = CachedType(leashed)
    if cached != ""
        return cached
    endif
    return "body_rope"
EndFunction

Function UnequipPairArmor(Actor holder, Actor leashed, String leashType)
    String t = NormalizeType(leashType)
    UnequipTypeArmor(MeshOwnerFor(holder, leashed, t), ArmorForType(t))
EndFunction

Function UnequipTrackedFor(Actor who)
    if who == None
        return
    endif
    EnsureCache()
    Int i = 0
    while i < CachedLeashed.Length
        Actor leashed = CachedLeashed[i]
        Actor holder = CachedHolders[i]
        if leashed && (leashed == who || holder == who)
            UnequipPairArmor(holder, leashed, CachedTypes[i])
            CachedLeashed[i] = None
            CachedHolders[i] = None
            CachedTypes[i] = ""
            CachedStyles[i] = ""
            LastPullTimes[i] = 0.0
        endif
        i += 1
    endwhile
EndFunction

Bool Function ApplyTypedLeash(Actor holder, Actor leashed, String style, String leashType)
    if holder == None || leashed == None
        return false
    endif
    if holder == leashed
        Debug.Trace("[SkyrimNet_Leash] ApplyLeash refused: " + ActorLabel(holder) + " cannot leash themselves")
        return false
    endif
    String t = NormalizeType(leashType)
    Armor leashArmor = ArmorForType(t)
    Actor meshOwner = MeshOwnerFor(holder, leashed, t)
    EquipTypeArmor(meshOwner, leashArmor)
    RememberPair(holder, leashed, t, style)
    Debug.Trace("[SkyrimNet_Leash] ApplyLeash holder=" + holder.GetDisplayName() + " leashed=" + leashed.GetDisplayName() + " style=" + style + " type=" + t)
    Bool ok = false
    if IsHolderOwnedType(t)
        ok = LeashFramework.ApplyHolderOwnedLeashToBone(holder, leashed, ParentBone(), ParentBone(), LeashBoneMatch(), 200.0, 300.0, true, 0.0, 0.0, 0.0, 1)
    else
        ok = LeashFramework.ApplyLeash(holder, leashed, ParentBone(), LeashBoneMatch(), 200.0, 300.0, true)
    endif
    if !ok
        Debug.Trace("[SkyrimNet_Leash] ApplyLeash returned false")
        UnequipTypeArmor(meshOwner, leashArmor)
        ForgetPair(leashed)
    endif
    return ok
EndFunction

; SkyrimNet has no way to consume a return value from an action function, so a refused
; leash is reported to the log instead of being swallowed.
Function LeashExecute(Actor akActor, Actor target, String style, String leashType)
    if !ApplyTypedLeash(akActor, target, style, leashType)
        Debug.Trace("[SkyrimNet_Leash] LeashExecute failed for " + ActorLabel(akActor) + " -> " + ActorLabel(target))
    endif
EndFunction

Function TargetLeashesSpeakerExecute(Actor akActor, Actor target, String style, String leashType)
    if !ApplyTypedLeash(target, akActor, style, leashType)
        Debug.Trace("[SkyrimNet_Leash] TargetLeashesSpeakerExecute failed for " + ActorLabel(target) + " -> " + ActorLabel(akActor))
    endif
EndFunction

Function UnleashTargetExecute(Actor akActor, Actor target)
    if akActor == None || target == None
        return
    endif
    Actor holder = akActor
    Actor leashed = target
    if LeashFramework.IsLeashed(akActor) && LeashFramework.GetLeashHolder(akActor) == target
        holder = target
        leashed = akActor
    endif
    String t = DetectType(holder, leashed)
    if LeashFramework.DisconnectLeash(akActor, target)
        UnequipPairArmor(holder, leashed, t)
        return
    endif
    if LeashFramework.DisconnectLeash(target, akActor)
        UnequipPairArmor(holder, leashed, t)
    endif
EndFunction

Function UnleashSpeakerExecute(Actor akActor)
    if akActor == None
        return
    endif
    LeashFramework.UnleashAll(akActor)
    UnequipTrackedFor(akActor)
EndFunction

Function Narrate(String content, Actor originator, Actor target)
    if content == ""
        return
    endif
    SkyrimNetApi.DirectNarration(content, originator, target)
EndFunction

Function NarrateLeash(Actor leashed, String reason)
    if leashed == None || reason == "loaded"
        return
    endif
    Actor holder = LeashFramework.GetLeashHolder(leashed)
    if holder == None
        holder = CachedHolder(leashed)
    endif
    String t = DetectType(holder, leashed)
    String phrase = TypePhrase(t)
    String adverb = StyleAdverb(CachedStyle(leashed))
    String leashedName = ActorLabel(leashed)
    Actor originator = holder
    Actor targetActor = leashed
    if originator == None
        originator = leashed
        targetActor = None
    endif
    String content = ""
    if holder == None
        content = leashedName + " is clipped to a " + phrase + "."
    elseif reason == "replaced"
        content = ActorLabel(holder) + " switches " + leashedName + " to a " + phrase + "."
    else
        content = ActorLabel(holder) + " " + adverb + "clips a " + phrase + " onto " + leashedName + "."
    endif
    Narrate(content, originator, targetActor)
EndFunction

Function NarrateUnleash(Actor leashed, String reason)
    if leashed == None || reason == "replaced"
        return
    endif
    Actor holder = CachedHolder(leashed)
    if holder == None
        holder = LeashFramework.GetLeashHolder(leashed)
    endif
    String t = DetectType(holder, leashed)
    String phrase = TypePhrase(t)
    String leashedName = ActorLabel(leashed)
    Actor originator = holder
    Actor targetActor = leashed
    if originator == None
        originator = leashed
        targetActor = None
    endif
    String content = ""
    if holder == None
        content = "The " + phrase + " is unclipped from " + leashedName + "."
    else
        content = ActorLabel(holder) + " unclips the " + phrase + " from " + leashedName + "."
    endif
    Narrate(content, originator, targetActor)
    UnequipPairArmor(holder, leashed, t)
    ForgetPair(leashed)
EndFunction

Bool Function ConsumePullCooldown(Actor leashed)
    if leashed == None
        return false
    endif
    EnsureCache()
    Int i = FindLeashedIndex(leashed)
    if i < 0
        RememberPair(None, leashed, DetectType(None, leashed), "normal")
        i = FindLeashedIndex(leashed)
    endif
    if i < 0
        return true
    endif
    Float now = Utility.GetCurrentRealTime()
    if now - LastPullTimes[i] < PullCooldown
        return false
    endif
    LastPullTimes[i] = now
    return true
EndFunction

Function NarratePull(Actor leashed, Bool ragdoll)
    if leashed == None || !ConsumePullCooldown(leashed)
        return
    endif
    Actor holder = LeashFramework.GetLeashHolder(leashed)
    if holder == None
        holder = CachedHolder(leashed)
    endif
    String phrase = TypePhrase(DetectType(holder, leashed))
    String leashedName = ActorLabel(leashed)
    Actor originator = holder
    Actor targetActor = leashed
    if originator == None
        originator = leashed
        targetActor = None
    endif
    String content = ""
    if ragdoll
        if holder
            content = "The " + phrase + " yanks " + leashedName + " off their feet toward " + ActorLabel(holder) + "."
        else
            content = "The " + phrase + " yanks " + leashedName + " off their feet."
        endif
    else
        if holder
            content = "The " + phrase + " snaps taut and pulls " + leashedName + " toward " + ActorLabel(holder) + "."
        else
            content = "The " + phrase + " snaps taut and pulls " + leashedName + "."
        endif
    endif
    Narrate(content, originator, targetActor)
EndFunction

; The SKSE side cannot reach LeashFramework's leash table, so every lifecycle event pushes
; the authoritative holder across. This runs before the narration filters, which skip the
; "loaded" and "replaced" reasons that still need to update the recorded pair.
Event OnLeashFrameworkLeash(String eventName, String strArg, Float numArg, Form sender)
    Actor leashed = sender as Actor
    if leashed
        SkyrimNet_Leash_Native.NotifyLeash(LeashFramework.GetLeashHolder(leashed), leashed)
    endif
    NarrateLeash(leashed, strArg)
EndEvent

Event OnLeashFrameworkUnleash(String eventName, String strArg, Float numArg, Form sender)
    Actor leashed = sender as Actor
    ; A replacement sends OnUnleash before OnLeash; keep the old holder until the new one
    ; arrives so the pair never falls back to a guess in between.
    if leashed && strArg != "replaced"
        SkyrimNet_Leash_Native.NotifyUnleash(leashed)
    endif
    NarrateUnleash(leashed, strArg)
EndEvent

Event OnLeashFrameworkPulled(String eventName, String strArg, Float numArg, Form sender)
    NarratePull(sender as Actor, false)
EndEvent

Event OnLeashFrameworkRagdollPulled(String eventName, String strArg, Float numArg, Form sender)
    NarratePull(sender as Actor, true)
EndEvent
