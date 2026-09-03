Scriptname SkyrimNet_Leash_Actions extends Quest

Float Property PullCooldown = 8.0 Auto Hidden
Float Property NarrateSuppressWindow = 2.0 Auto Hidden

Actor[] CachedLeashed
Actor[] CachedHolders
String[] CachedKinds
String[] CachedBodyParts
String[] CachedDistances
String[] CachedStyles
Float[] LastPullTimes
Float[] LastNarrateTimes

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

String Function LeashBoneMatch()
    return "Leash1"
EndFunction

; Papyrus requires a literal array size, so this is the one place the cache length lives.
Int Function CacheSize()
    return 32
EndFunction

Function EnsureCache()
    if CachedLeashed.Length != CacheSize()
        CachedLeashed = new Actor[32]
        CachedHolders = new Actor[32]
        CachedKinds = new String[32]
        CachedBodyParts = new String[32]
        CachedDistances = new String[32]
        CachedStyles = new String[32]
        LastPullTimes = new Float[32]
        LastNarrateTimes = new Float[32]
    endif
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

String Function Possessive(Actor who)
    return ActorLabel(who) + "'s"
EndFunction

String Function NormalizeKind(String leashType)
    String t = SkyrimNet_Leash_Native.NormalizeToken(leashType)
    if t == "chain" || t == "neck_chain"
        return "chain"
    elseif t == "magic" || t == "magic_rope" || t == "runic" || t == "neck_magic"
        return "magic"
    elseif t == "holder_shield" || t == "shield"
        return "holder_shield"
    elseif t == "rope" || t == "body_rope" || t == "waist_rope" || t == "neck_rope" || t == "waist" || t == "body"
        return "rope"
    endif
    if t == ""
        return ""
    endif
    return "rope"
EndFunction

String Function NormalizeBodyPart(String bodyPart)
    String t = SkyrimNet_Leash_Native.NormalizeToken(bodyPart)
    if t == "waist" || t == "body" || t == "body_rope" || t == "waist_rope" || t == "waist_chain" || t == "waist_magic"
        return "waist"
    endif
    if t == "neck" || t == "neck_rope" || t == "neck_chain" || t == "neck_magic" || t == "collar"
        return "neck"
    endif
    if t == ""
        return ""
    endif
    return "neck"
EndFunction

String Function NormalizeDistance(String leashDistance)
    String t = SkyrimNet_Leash_Native.NormalizeToken(leashDistance)
    if t == "tight"
        return "tight"
    elseif t == "short" || t == "close"
        return "short"
    elseif t == "long"
        return "long"
    elseif t == "middle" || t == "near" || t == "normal" || t == "medium"
        return "middle"
    endif
    if t == ""
        return ""
    endif
    return "middle"
EndFunction

String Function StyleWord(String style)
    String s = SkyrimNet_Leash_Native.NormalizeToken(style)
    if s == "forceful" || s == "forcefully"
        return "forcefully"
    elseif s == "gentle" || s == "gently"
        return "gently"
    endif
    return "normally"
EndFunction

Float Function DistanceMin()
    return 50.0
EndFunction

Float Function DistanceMax(String leashDistance)
    String d = NormalizeDistance(leashDistance)
    if d == ""
        d = "middle"
    endif
    if d == "tight"
        return 80.0
    elseif d == "short"
        return 150.0
    elseif d == "long"
        return 300.0
    endif
    return 220.0
EndFunction

String Function DistanceFromLength(Float maxLength)
    if maxLength <= 0.0
        return "middle"
    elseif maxLength <= 100.0
        return "tight"
    elseif maxLength <= 180.0
        return "short"
    elseif maxLength <= 250.0
        return "middle"
    endif
    return "long"
EndFunction

String Function ParentBoneFor(String kind, String bodyPart)
    if kind == "holder_shield"
        return "NPC Spine2 [Spn2]"
    endif
    if bodyPart == "waist"
        return "NPC Spine1 [Spn1]"
    endif
    return "NPC Neck [Neck]"
EndFunction

Bool Function IsHolderOwnedKind(String kind)
    return kind == "holder_shield"
EndFunction

Actor Function MeshOwnerFor(Actor holder, Actor leashed, String kind)
    if IsHolderOwnedKind(kind)
        return holder
    endif
    return leashed
EndFunction

Armor Function ArmorForKind(String kind, String bodyPart)
    Int formId = 0x800
    if kind == "holder_shield"
        formId = 0xD69
    elseif bodyPart == "neck"
        if kind == "chain"
            formId = 0x806
        elseif kind == "magic"
            ; DOM uses 0x2CE for the same runic mesh; 0x32CE is not a Leash.esm armor.
            formId = 0x2CE
        else
            formId = 0x804
        endif
    endif
    return Game.GetFormFromFile(formId, "Leash.esm") as Armor
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

Function RememberPair(Actor holder, Actor leashed, String kind, String bodyPart, String style, String leashDistance)
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
    CachedKinds[i] = kind
    CachedBodyParts[i] = bodyPart
    CachedStyles[i] = style
    CachedDistances[i] = leashDistance
EndFunction

Function ForgetPair(Actor leashed)
    Int i = FindLeashedIndex(leashed)
    if i < 0
        return
    endif
    CachedLeashed[i] = None
    CachedHolders[i] = None
    CachedKinds[i] = ""
    CachedBodyParts[i] = ""
    CachedDistances[i] = ""
    CachedStyles[i] = ""
    LastPullTimes[i] = 0.0
    LastNarrateTimes[i] = 0.0
EndFunction

String Function CachedKind(Actor leashed)
    Int i = FindLeashedIndex(leashed)
    if i >= 0 && CachedKinds[i] != ""
        return CachedKinds[i]
    endif
    return ""
EndFunction

String Function CachedBodyPart(Actor leashed)
    Int i = FindLeashedIndex(leashed)
    if i >= 0 && CachedBodyParts[i] != ""
        return CachedBodyParts[i]
    endif
    return ""
EndFunction

String Function CachedDistance(Actor leashed)
    Int i = FindLeashedIndex(leashed)
    if i >= 0 && CachedDistances[i] != ""
        return CachedDistances[i]
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
    if i >= 0 && CachedStyles[i] != ""
        return CachedStyles[i]
    endif
    return "normally"
EndFunction

String Function DetectKind(Actor holder, Actor leashed)
    Armor shieldArmor = ArmorForKind("holder_shield", "waist")
    if holder && shieldArmor && holder.IsEquipped(shieldArmor)
        return "holder_shield"
    endif
    if leashed
        Armor chainArmor = ArmorForKind("chain", "neck")
        if chainArmor && leashed.IsEquipped(chainArmor)
            return "chain"
        endif
        Armor magicArmor = ArmorForKind("magic", "neck")
        if magicArmor && leashed.IsEquipped(magicArmor)
            return "magic"
        endif
        Armor neckArmor = ArmorForKind("rope", "neck")
        if neckArmor && leashed.IsEquipped(neckArmor)
            return "rope"
        endif
        Armor bodyArmor = ArmorForKind("rope", "waist")
        if bodyArmor && leashed.IsEquipped(bodyArmor)
            return "rope"
        endif
    endif
    String cached = CachedKind(leashed)
    if cached != ""
        return cached
    endif
    return "rope"
EndFunction

String Function DetectBodyPart(Actor holder, Actor leashed)
    Armor shieldArmor = ArmorForKind("holder_shield", "waist")
    if holder && shieldArmor && holder.IsEquipped(shieldArmor)
        return "waist"
    endif
    if leashed
        Armor chainArmor = ArmorForKind("chain", "neck")
        Armor magicArmor = ArmorForKind("magic", "neck")
        Armor neckArmor = ArmorForKind("rope", "neck")
        if (chainArmor && leashed.IsEquipped(chainArmor)) || (magicArmor && leashed.IsEquipped(magicArmor)) || (neckArmor && leashed.IsEquipped(neckArmor))
            return "neck"
        endif
        Armor bodyArmor = ArmorForKind("rope", "waist")
        if bodyArmor && leashed.IsEquipped(bodyArmor)
            return "waist"
        endif
    endif
    String cached = CachedBodyPart(leashed)
    if cached != ""
        return cached
    endif
    return "neck"
EndFunction

String Function ResolveKind(Actor holder, Actor leashed, String leashType)
    String k = NormalizeKind(leashType)
    if k != ""
        return k
    endif
    return DetectKind(holder, leashed)
EndFunction

String Function ResolveBodyPart(Actor holder, Actor leashed, String bodyPart)
    String b = NormalizeBodyPart(bodyPart)
    if b != ""
        return b
    endif
    return DetectBodyPart(holder, leashed)
EndFunction

String Function ResolveDistance(Actor leashed, String leashDistance)
    String d = NormalizeDistance(leashDistance)
    if d != ""
        return d
    endif
    String cached = CachedDistance(leashed)
    if cached != ""
        return cached
    endif
    if leashed
        Float maxLength = LeashFramework.GetMaxLeashLength(leashed)
        if maxLength > 0.0
            return DistanceFromLength(maxLength)
        endif
    endif
    return "middle"
EndFunction

Function UnequipPairArmor(Actor holder, Actor leashed, String kind, String bodyPart)
    UnequipTypeArmor(MeshOwnerFor(holder, leashed, kind), ArmorForKind(kind, bodyPart))
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
            UnequipPairArmor(holder, leashed, CachedKinds[i], CachedBodyParts[i])
            CachedLeashed[i] = None
            CachedHolders[i] = None
            CachedKinds[i] = ""
            CachedBodyParts[i] = ""
            CachedDistances[i] = ""
            CachedStyles[i] = ""
            LastPullTimes[i] = 0.0
            LastNarrateTimes[i] = 0.0
        endif
        i += 1
    endwhile
EndFunction

Function MarkSuppressed(Actor leashed)
    if leashed == None
        return
    endif
    Int i = FindLeashedIndex(leashed)
    if i < 0
        RememberPair(None, leashed, DetectKind(None, leashed), DetectBodyPart(None, leashed), "normally", "middle")
        i = FindLeashedIndex(leashed)
    endif
    if i < 0
        return
    endif
    LastNarrateTimes[i] = Utility.GetCurrentRealTime()
EndFunction

Bool Function IsSuppressed(Actor leashed)
    if leashed == None
        return false
    endif
    Int i = FindLeashedIndex(leashed)
    if i < 0
        return false
    endif
    Float last = LastNarrateTimes[i]
    if last <= 0.0
        return false
    endif
    return Utility.GetCurrentRealTime() - last < NarrateSuppressWindow
EndFunction

Function OffsetTiePoint(Actor leashed, String tiePoint, Float[] xyz)
    Float x = leashed.GetPositionX()
    Float y = leashed.GetPositionY()
    Float z = leashed.GetPositionZ()
    Float a = leashed.GetAngleZ()
    Float h = 100.0
    Float d = 50.0
    String t = SkyrimNet_Leash_Native.NormalizeToken(tiePoint)
    if t == "wall"
        t = "front"
    endif
    if t == "left"
        x += d * Math.Cos(a)
        y -= d * Math.Sin(a)
    elseif t == "back"
        x -= d * Math.Sin(a)
        y -= d * Math.Cos(a)
    elseif t == "front"
        x += d * Math.Sin(a)
        y += d * Math.Cos(a)
    elseif t == "right"
        x -= d * Math.Cos(a)
        y += d * Math.Sin(a)
    else
        h = 1.0
    endif
    xyz[0] = x
    xyz[1] = y
    xyz[2] = z + h
EndFunction

String Function NormalizeTiePoint(String tiePoint)
    String t = SkyrimNet_Leash_Native.NormalizeToken(tiePoint)
    if t == "left" || t == "back" || t == "front" || t == "right" || t == "floor" || t == "wall"
        return t
    endif
    return "floor"
EndFunction

Bool Function ApplyToHolder(Actor holder, Actor leashed, String style, String leashDistance, String kind, String bodyPart)
    if holder == None || leashed == None
        return false
    endif
    if holder == leashed
        Debug.Trace("[SkyrimNet_Leash] ApplyToHolder refused: " + ActorLabel(holder) + " cannot leash themselves")
        return false
    endif
    Armor leashArmor = ArmorForKind(kind, bodyPart)
    Actor meshOwner = MeshOwnerFor(holder, leashed, kind)
    EquipTypeArmor(meshOwner, leashArmor)
    RememberPair(holder, leashed, kind, bodyPart, style, leashDistance)
    MarkSuppressed(leashed)
    Debug.Trace("[SkyrimNet_Leash] ApplyToHolder holder=" + ActorLabel(holder) + " leashed=" + ActorLabel(leashed) + " kind=" + kind + " body=" + bodyPart + " distance=" + leashDistance)
    String bone = ParentBoneFor(kind, bodyPart)
    Float minLen = DistanceMin()
    Float maxLen = DistanceMax(leashDistance)
    Bool ok = false
    if IsHolderOwnedKind(kind)
        ok = LeashFramework.ApplyHolderOwnedLeashToBone(holder, leashed, bone, bone, LeashBoneMatch(), minLen, maxLen, true, 0.0, 0.0, 0.0, 2)
    else
        ok = LeashFramework.ApplyLeashToHand(holder, leashed, bone, LeashBoneMatch(), minLen, maxLen, true, false)
    endif
    if !ok
        Debug.Trace("[SkyrimNet_Leash] ApplyToHolder returned false")
        UnequipTypeArmor(meshOwner, leashArmor)
        ForgetPair(leashed)
        return false
    endif
    SkyrimNet_Leash_Native.NotifyLeash(holder, leashed, kind, leashDistance, bodyPart)
    return true
EndFunction

Bool Function ApplyToTiePoint(Actor leashed, String style, String leashDistance, String kind, String bodyPart, String tiePoint)
    if leashed == None
        return false
    endif
    Cell parentCell = leashed.GetParentCell()
    if parentCell == None
        return false
    endif
    Armor leashArmor = ArmorForKind(kind, bodyPart)
    EquipTypeArmor(leashed, leashArmor)
    RememberPair(None, leashed, kind, bodyPart, style, leashDistance)
    MarkSuppressed(leashed)
    Float[] xyz = new Float[3]
    OffsetTiePoint(leashed, tiePoint, xyz)
    String bone = ParentBoneFor(kind, bodyPart)
    Bool ok = LeashFramework.ApplyLeashAtPosition(leashed, parentCell, xyz[0], xyz[1], xyz[2], bone, LeashBoneMatch(), DistanceMin(), DistanceMax(leashDistance), true)
    if !ok
        Debug.Trace("[SkyrimNet_Leash] ApplyToTiePoint returned false")
        UnequipTypeArmor(leashed, leashArmor)
        ForgetPair(leashed)
        return false
    endif
    SkyrimNet_Leash_Native.NotifyLeash(None, leashed, kind, leashDistance, bodyPart)
    return true
EndFunction

Function Narrate(String content, Actor originator, Actor target)
    if content == ""
        return
    endif
    SkyrimNetApi.DirectNarration(content, originator, target)
EndFunction

Actor Function NarrateListener(Actor subject, Actor leashed, Actor holder)
    if leashed && leashed != subject
        return leashed
    endif
    if holder && holder != subject
        return holder
    endif
    return None
EndFunction

Function NarrateApply(Actor subject, Actor leashed, Actor holder, String style, String leashDistance, String kind, String bodyPart)
    String spokenKind = kind
    if spokenKind == "holder_shield"
        spokenKind = "shield"
    endif
    String content = ActorLabel(subject) + " " + StyleWord(style) + " leashes " + Possessive(leashed) + " " + bodyPart + " with a " + leashDistance + " " + spokenKind + " leash."
    Narrate(content, subject, NarrateListener(subject, leashed, holder))
EndFunction

Function LeashedToHolder(Actor subject, Actor leashed, Actor holder, String style, String leashDistance, String leashType, String body_part)
    if leashed == None || holder == None
        Debug.Trace("[SkyrimNet_Leash] LeashedToHolder skipped: missing leashed or holder")
        return
    endif
    String kind = ResolveKind(holder, leashed, leashType)
    String bodyPart = ResolveBodyPart(holder, leashed, body_part)
    String distance = ResolveDistance(leashed, leashDistance)
    if subject == None
        subject = holder
    endif
    if !ApplyToHolder(holder, leashed, style, distance, kind, bodyPart)
        Debug.Trace("[SkyrimNet_Leash] LeashedToHolder failed for " + ActorLabel(holder) + " -> " + ActorLabel(leashed))
        return
    endif
    NarrateApply(subject, leashed, holder, style, distance, kind, bodyPart)
EndFunction

Function TakeLeash(Actor subject, Actor leashed)
    if subject == None || leashed == None
        return
    endif
    String kind = DetectKind(subject, leashed)
    String bodyPart = DetectBodyPart(subject, leashed)
    String distance = ResolveDistance(leashed, "")
    String style = CachedStyle(leashed)
    if !ApplyToHolder(subject, leashed, style, distance, kind, bodyPart)
        Debug.Trace("[SkyrimNet_Leash] TakeLeash failed for " + ActorLabel(subject) + " -> " + ActorLabel(leashed))
        return
    endif
    Narrate(ActorLabel(subject) + " takes " + Possessive(leashed) + " leash.", subject, NarrateListener(subject, leashed, subject))
EndFunction

Function GiveLeash(Actor subject, Actor leashed, Actor receiver)
    if subject == None || leashed == None || receiver == None
        return
    endif
    if receiver == leashed || receiver == subject
        Debug.Trace("[SkyrimNet_Leash] GiveLeash skipped: receiver cannot be the leashed actor or the subject")
        return
    endif
    String kind = DetectKind(receiver, leashed)
    String bodyPart = DetectBodyPart(receiver, leashed)
    String distance = ResolveDistance(leashed, "")
    String style = CachedStyle(leashed)
    if !ApplyToHolder(receiver, leashed, style, distance, kind, bodyPart)
        Debug.Trace("[SkyrimNet_Leash] GiveLeash failed for " + ActorLabel(subject) + " -> " + ActorLabel(receiver))
        return
    endif
    Narrate(ActorLabel(subject) + " gives " + Possessive(leashed) + " leash to " + ActorLabel(receiver) + ".", subject, leashed)
EndFunction

Function LeashedToTiePoint(Actor subject, Actor leashed, String style, String leashDistance, String leashType, String body_part, String tiePoint)
    if leashed == None
        return
    endif
    String kind = ResolveKind(None, leashed, leashType)
    String bodyPart = ResolveBodyPart(None, leashed, body_part)
    String distance = ResolveDistance(leashed, leashDistance)
    String point = NormalizeTiePoint(tiePoint)
    if subject == None
        subject = leashed
    endif
    if !ApplyToTiePoint(leashed, style, distance, kind, bodyPart, point)
        Debug.Trace("[SkyrimNet_Leash] LeashedToTiePoint failed for " + ActorLabel(leashed))
        return
    endif
    Narrate(ActorLabel(subject) + " ties " + Possessive(leashed) + " leash to the " + point + ".", subject, NarrateListener(subject, leashed, None))
EndFunction

Function LeashedRefused(Actor subject, Actor leashed)
    if subject == None || leashed == None
        return
    endif
    Narrate(ActorLabel(leashed) + " refused to be leashed by " + ActorLabel(subject) + ".", leashed, subject)
EndFunction

Function UnleashTargetExecute(Actor subject, Actor target)
    if subject == None || target == None
        return
    endif
    Actor holder = subject
    Actor leashed = target
    if LeashFramework.IsLeashed(subject) && LeashFramework.GetLeashHolder(subject) == target
        holder = target
        leashed = subject
    endif
    String kind = DetectKind(holder, leashed)
    String bodyPart = DetectBodyPart(holder, leashed)
    MarkSuppressed(leashed)
    Bool ok = LeashFramework.DisconnectLeash(subject, target)
    if !ok
        ok = LeashFramework.DisconnectLeash(target, subject)
    endif
    if ok
        Narrate(ActorLabel(subject) + " unclips the " + kind + " leash from " + Possessive(leashed) + " " + bodyPart + ".", subject, NarrateListener(subject, leashed, holder))
        UnequipPairArmor(holder, leashed, kind, bodyPart)
        ForgetPair(leashed)
        SkyrimNet_Leash_Native.NotifyUnleash(leashed)
    endif
EndFunction

Function UnleashSpeakerExecute(Actor subject)
    if subject == None
        return
    endif
    EnsureCache()
    Int i = 0
    while i < CachedLeashed.Length
        Actor leashed = CachedLeashed[i]
        Actor holder = CachedHolders[i]
        if leashed && (leashed == subject || holder == subject)
            String kind = CachedKinds[i]
            if kind == ""
                kind = DetectKind(holder, leashed)
            endif
            String bodyPart = CachedBodyParts[i]
            if bodyPart == ""
                bodyPart = DetectBodyPart(holder, leashed)
            endif
            MarkSuppressed(leashed)
            Actor originator = subject
            Narrate(ActorLabel(originator) + " unclips the " + kind + " leash from " + Possessive(leashed) + " " + bodyPart + ".", originator, NarrateListener(originator, leashed, holder))
        endif
        i += 1
    endwhile
    if LeashFramework.IsLeashed(subject) && FindLeashedIndex(subject) < 0
        Actor holder = LeashFramework.GetLeashHolder(subject)
        String kind = DetectKind(holder, subject)
        String bodyPart = DetectBodyPart(holder, subject)
        MarkSuppressed(subject)
        Narrate(ActorLabel(subject) + " unclips the " + kind + " leash from " + Possessive(subject) + " " + bodyPart + ".", subject, holder)
    endif
    LeashFramework.UnleashAll(subject)
    UnequipTrackedFor(subject)
EndFunction

Function NarrateLeash(Actor leashed, String reason)
    if leashed == None || reason == "loaded" || IsSuppressed(leashed)
        return
    endif
    Actor holder = LeashFramework.GetLeashHolder(leashed)
    if holder == None
        holder = CachedHolder(leashed)
    endif
    String kind = DetectKind(holder, leashed)
    String bodyPart = DetectBodyPart(holder, leashed)
    String distance = ResolveDistance(leashed, "")
    String style = CachedStyle(leashed)
    Actor originator = holder
    Actor targetActor = leashed
    String content = ""
    if holder == None
        content = "A " + distance + " " + kind + " leash is tied to " + Possessive(leashed) + " " + bodyPart + "."
        originator = None
        targetActor = leashed
    else
        content = ActorLabel(holder) + " " + StyleWord(style) + " leashes " + Possessive(leashed) + " " + bodyPart + " with a " + distance + " " + kind + " leash."
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
    String kind = DetectKind(holder, leashed)
    String bodyPart = DetectBodyPart(holder, leashed)
    Bool suppressed = IsSuppressed(leashed)
    if !suppressed
        Actor originator = holder
        Actor targetActor = leashed
        String content = ""
        if holder == None
            content = "The " + kind + " leash is unclipped from " + Possessive(leashed) + " " + bodyPart + "."
            originator = None
        else
            content = ActorLabel(holder) + " unclips the " + kind + " leash from " + Possessive(leashed) + " " + bodyPart + "."
        endif
        Narrate(content, originator, targetActor)
    endif
    UnequipPairArmor(holder, leashed, kind, bodyPart)
    ForgetPair(leashed)
EndFunction

Bool Function ConsumePullCooldown(Actor leashed)
    if leashed == None
        return false
    endif
    EnsureCache()
    Int i = FindLeashedIndex(leashed)
    if i < 0
        RememberPair(None, leashed, DetectKind(None, leashed), DetectBodyPart(None, leashed), "normally", ResolveDistance(leashed, ""))
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
    String kind = DetectKind(holder, leashed)
    String leashedName = ActorLabel(leashed)
    Actor originator = holder
    Actor targetActor = leashed
    if originator == None
        originator = None
        targetActor = leashed
    endif
    String content = ""
    if ragdoll
        if holder
            content = "The " + kind + " leash yanks " + leashedName + " off their feet toward " + ActorLabel(holder) + "."
        else
            content = "The " + kind + " leash yanks " + leashedName + " off their feet."
        endif
    else
        if holder
            content = "The " + kind + " leash snaps taut and pulls " + leashedName + " toward " + ActorLabel(holder) + "."
        else
            content = "The " + kind + " leash snaps taut and pulls " + leashedName + "."
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
        Actor holder = LeashFramework.GetLeashHolder(leashed)
        SkyrimNet_Leash_Native.NotifyLeash(holder, leashed, DetectKind(holder, leashed), ResolveDistance(leashed, ""), DetectBodyPart(holder, leashed))
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
