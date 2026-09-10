Scriptname SkyrimNet_Leashed_Actions extends Quest

Float Property PullCooldown = 8.0 Auto Hidden
Float Property NarrateSuppressWindow = 2.0 Auto Hidden

Actor[] CachedLeashed
Actor[] CachedHolders
String[] CachedKinds
String[] CachedBodyParts
String[] CachedDistances
String[] CachedStyles
Bool[] CachedTied
Float[] LastPullTimes
Float[] LastTautTimes
Actor[] SuppressActors
Float[] SuppressTimes
Actor[] StrugglingActors
Float[] StruggleLastNarrate

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
    RepushCachedPairs()
    RepushStruggling()
EndFunction

Function RepushCachedPairs()
    EnsureCache()
    Int i = 0
    while i < CachedLeashed.Length
        Actor leashed = CachedLeashed[i]
        if leashed
            Actor holder = CachedHolders[i]
            Bool tied = CachedTied[i]
            String kind = CachedKinds[i]
            String bodyPart = CachedBodyParts[i]
            String distance = CachedDistances[i]
            SkyrimNet_Leashed_Native.NotifyLeash(holder, leashed, kind, distance, bodyPart, tied)
            if bodyPart == "wrists"
                ApplyWristBind(leashed)
            endif
        endif
        i += 1
    endwhile
EndFunction

Function RepushStruggling()
    EnsureCache()
    Int i = 0
    while i < StrugglingActors.Length
        Actor who = StrugglingActors[i]
        if who
            if who.IsDead() || (!LeashFramework.IsLeashed(who) && FindLeashedIndex(who) < 0)
                EndStruggle(who, false)
            else
                SkyrimNet_Leashed_Native.NotifyStruggle(who, true)
            endif
        endif
        i += 1
    endwhile
    RefreshStruggleUpdates()
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
        CachedTied = new Bool[32]
        LastPullTimes = new Float[32]
        LastTautTimes = new Float[32]
    endif
    if CachedTied.Length != CacheSize()
        CachedTied = new Bool[32]
    endif
    if LastTautTimes.Length != CacheSize()
        LastTautTimes = new Float[32]
    endif
    if SuppressActors.Length != CacheSize()
        SuppressActors = new Actor[32]
        SuppressTimes = new Float[32]
    endif
    if StrugglingActors.Length != CacheSize()
        StrugglingActors = new Actor[32]
        StruggleLastNarrate = new Float[32]
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

String Function PossessivePronoun(Actor who)
    if who == None
        return "their"
    endif
    ActorBase base = who.GetLeveledActorBase()
    if base == None
        base = who.GetActorBase()
    endif
    if base == None
        return "their"
    endif
    Int sex = base.GetSex()
    if sex == 1
        return "her"
    endif
    if sex == 0
        return "his"
    endif
    return "their"
EndFunction

String Function NormalizeKind(String leashType)
    String t = SkyrimNet_Leashed_Native.NormalizeToken(leashType)
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
    String t = SkyrimNet_Leashed_Native.NormalizeToken(bodyPart)
    if t == "waist" || t == "body" || t == "body_rope" || t == "waist_rope" || t == "waist_chain" || t == "waist_magic"
        return "waist"
    endif
    if t == "wrists" || t == "wrist" || t == "hands" || t == "cuffs"
        return "wrists"
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
    String t = SkyrimNet_Leashed_Native.NormalizeToken(leashDistance)
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
    String s = SkyrimNet_Leashed_Native.NormalizeToken(style)
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
    return SkyrimNet_Leashed_Native.DistanceMax(d)
EndFunction

String Function DistanceFromLength(Float maxLength)
    return SkyrimNet_Leashed_Native.DistanceFromLength(maxLength)
EndFunction

String Function ParentBoneFor(String kind, String bodyPart)
    if kind == "holder_shield"
        return "NPC Spine2 [Spn2]"
    endif
    String b = NormalizeBodyPart(bodyPart)
    if b == "waist"
        return "NPC Spine1 [Spn1]"
    endif
    if b == "wrists"
        return "NPC Spine2 [Spn2]"
    endif
    return "NPC Spine2 [Spn2]"
EndFunction

String Function KindForDangling(String kind)
    if IsHolderOwnedKind(kind)
        return "rope"
    endif
    if kind == ""
        return "rope"
    endif
    return kind
EndFunction

; Leash.esm only ships Leash_hand_chain (0xD69) for wrists. Rope/magic wrist meshes do not exist.
String Function KindForBodyPart(String kind, String bodyPart)
    if NormalizeBodyPart(bodyPart) == "wrists"
        return "chain"
    endif
    return kind
EndFunction

String Function SpokenKind(String kind)
    if kind == "holder_shield"
        return "shield"
    endif
    return kind
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
    elseif bodyPart == "wrists"
        ; Only Leash_hand_chain exists; KindForBodyPart already coerced kind to chain.
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

Armor Function PrisonerCuffsArmor()
    return Game.GetFormFromFile(0x10E039, "Skyrim.esm") as Armor
EndFunction

Idle Function BoundStandingIdle()
    ; OffsetBoundStandingStart (0xB600A) only applies while walking.
    return Game.GetFormFromFile(0x109837, "Skyrim.esm") as Idle
EndFunction

Idle Function BoundStandingCutIdle()
    return Game.GetFormFromFile(0x109B6A, "Skyrim.esm") as Idle
EndFunction

Function ApplyWristBind(Actor leashed)
    if leashed == None
        return
    endif
    Armor cuffs = PrisonerCuffsArmor()
    if cuffs
        Armor wristLeash = ArmorForKind("rope", "wrists")
        Armor worn = leashed.GetWornForm(cuffs.GetSlotMask()) as Armor
        if worn && worn != cuffs && worn != wristLeash
            leashed.UnequipItem(worn, false, true)
        endif
        if leashed.GetItemCount(cuffs) < 1
            leashed.AddItem(cuffs, 1, true)
        endif
        if !leashed.IsEquipped(cuffs)
            leashed.EquipItem(cuffs, true, true)
        endif
    else
        Debug.Trace("[SkyrimNet_Leashed] ApplyWristBind missing PrisonerCuffsPlayer 0x10E039")
    endif
    Idle boundIdle = BoundStandingIdle()
    Bool played = false
    if boundIdle
        played = leashed.PlayIdle(boundIdle)
    endif
    if !played
        Debug.SendAnimationEvent(leashed, "OffsetBoundStandingPlayerInstant")
    endif
EndFunction

Function ClearWristBind(Actor leashed)
    if leashed == None
        return
    endif
    UnregisterForAnimationEvent(leashed, "GetUpEnd")
    Idle cutIdle = BoundStandingCutIdle()
    Bool cut = false
    if cutIdle
        cut = leashed.PlayIdle(cutIdle)
    endif
    if !cut
        Debug.SendAnimationEvent(leashed, "BoundStandingCut")
    endif
    leashed.SetRestrained(false)
    Armor cuffs = PrisonerCuffsArmor()
    if cuffs == None
        return
    endif
    if leashed.IsEquipped(cuffs)
        leashed.UnequipItem(cuffs, false, true)
    endif
    if leashed.GetItemCount(cuffs) > 0
        leashed.RemoveItem(cuffs, 1, true)
    endif
EndFunction

Function EquipTypeArmor(Actor meshOwner, Armor leashArmor)
    if meshOwner == None || leashArmor == None
        return
    endif
    Armor worn = meshOwner.GetWornForm(leashArmor.GetSlotMask()) as Armor
    if worn && worn != leashArmor
        meshOwner.UnequipItem(worn, false, true)
    endif
    if meshOwner.GetItemCount(leashArmor) < 1
        meshOwner.AddItem(leashArmor, 1, true)
    endif
    if !meshOwner.IsEquipped(leashArmor)
        meshOwner.EquipItem(leashArmor, true, true)
    endif
EndFunction

Bool Function WaitForLeashMesh(Actor meshOwner, Armor leashArmor)
    if meshOwner == None || leashArmor == None
        Debug.Trace("[SkyrimNet_Leashed] WaitForLeashMesh skipped: missing owner or armor")
        return false
    endif
    Int tries = 0
    while tries < 4 && !meshOwner.IsEquipped(leashArmor)
        Utility.Wait(0.25)
        tries += 1
    endwhile
    Bool worn = meshOwner.IsEquipped(leashArmor)
    Debug.Trace("[SkyrimNet_Leashed] WaitForLeashMesh armor=" + leashArmor + " equipped=" + worn)
    if worn
        SkyrimNet_Leashed_Native.TraceLeashBones(meshOwner)
    endif
    return worn
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

Int Function FindStruggleIndex(Actor who)
    EnsureCache()
    Int i = 0
    while i < StrugglingActors.Length
        if StrugglingActors[i] == who
            return i
        endif
        i += 1
    endwhile
    return -1
EndFunction

Int Function EnsureStruggleIndex(Actor who)
    Int i = FindStruggleIndex(who)
    if i >= 0
        return i
    endif
    i = FindStruggleIndex(None)
    if i < 0
        return -1
    endif
    StrugglingActors[i] = who
    StruggleLastNarrate[i] = 0.0
    return i
EndFunction

Function ClearStruggleSlot(Int i)
    if i < 0 || i >= StrugglingActors.Length
        return
    endif
    StrugglingActors[i] = None
    StruggleLastNarrate[i] = 0.0
EndFunction

Int Function CountStruggling()
    EnsureCache()
    Int n = 0
    Int i = 0
    while i < StrugglingActors.Length
        if StrugglingActors[i]
            n += 1
        endif
        i += 1
    endwhile
    return n
EndFunction

Function RefreshStruggleUpdates()
    if CountStruggling() > 0
        RegisterForUpdate(1.0)
    else
        UnregisterForUpdate()
    endif
EndFunction

Function RememberPair(Actor holder, Actor leashed, String kind, String bodyPart, String style, String leashDistance, Bool tied)
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
    CachedTied[i] = holder == None && tied
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
    CachedTied[i] = false
    LastPullTimes[i] = 0.0
    LastTautTimes[i] = 0.0
EndFunction

Int Function EnsureLeashedSlot(Actor leashed)
    if leashed == None
        return -1
    endif
    EnsureCache()
    Int i = FindLeashedIndex(leashed)
    if i >= 0
        return i
    endif
    Actor holder = LeashFramework.GetLeashHolder(leashed)
    Bool tied = holder == None && LeashFramework.IsLeashed(leashed)
    RememberPair(holder, leashed, DetectKind(holder, leashed), DetectBodyPart(holder, leashed), "normally", ResolveDistance(leashed, ""), tied)
    return FindLeashedIndex(leashed)
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

Bool Function CachedIsTied(Actor leashed)
    Int i = FindLeashedIndex(leashed)
    if i >= 0
        return CachedTied[i]
    endif
    return false
EndFunction

String Function DetectKind(Actor holder, Actor leashed)
    Armor shieldArmor = ArmorForKind("holder_shield", "waist")
    if holder && shieldArmor && holder.IsEquipped(shieldArmor)
        return "holder_shield"
    endif
    if leashed
        ; Leashed-worn 0xD69 is wrists chain, not holder_shield.
        if shieldArmor && leashed.IsEquipped(shieldArmor)
            return "chain"
        endif
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
    ; Leashed-worn 0xD69 is the wrists body part, not holder_shield.
    if leashed && shieldArmor && leashed.IsEquipped(shieldArmor)
        return "wrists"
    endif
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
    if NormalizeBodyPart(bodyPart) == "wrists"
        ClearWristBind(leashed)
    endif
EndFunction

Function UnequipStaleBodyArmor(Actor leashed, String kind, String bodyPart)
    String prevKind = CachedKind(leashed)
    String prevBody = CachedBodyPart(leashed)
    if prevKind == "" && prevBody == ""
        return
    endif
    String nextBody = NormalizeBodyPart(bodyPart)
    if prevBody == "wrists" && nextBody != "wrists"
        ClearWristBind(leashed)
    endif
    Armor prevArmor = ArmorForKind(prevKind, prevBody)
    Armor nextArmor = ArmorForKind(kind, bodyPart)
    Actor prevOwner = MeshOwnerFor(CachedHolder(leashed), leashed, prevKind)
    if prevArmor == None
        return
    endif
    ; 0xD69 can be holder-owned (legacy shield) or leashed-worn (wrists).
    if prevArmor == nextArmor && prevOwner == leashed
        return
    endif
    UnequipTypeArmor(prevOwner, prevArmor)
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
            CachedTied[i] = false
            LastPullTimes[i] = 0.0
            LastTautTimes[i] = 0.0
            SkyrimNet_Leashed_Native.NotifyUnleash(leashed)
        endif
        i += 1
    endwhile
EndFunction

Int Function FindSuppressIndex(Actor who)
    EnsureCache()
    Int i = 0
    while i < SuppressActors.Length
        if SuppressActors[i] == who
            return i
        endif
        i += 1
    endwhile
    return -1
EndFunction

Int Function FindSuppressSlot(Actor leashed)
    Int existing = FindSuppressIndex(leashed)
    if existing >= 0
        return existing
    endif
    Int empty = FindSuppressIndex(None)
    if empty >= 0
        return empty
    endif
    Float now = Utility.GetCurrentRealTime()
    Int best = 0
    Float bestTime = SuppressTimes[0]
    Int i = 0
    while i < SuppressActors.Length
        Float t = SuppressTimes[i]
        if now - t >= NarrateSuppressWindow
            return i
        endif
        if t < bestTime
            bestTime = t
            best = i
        endif
        i += 1
    endwhile
    return best
EndFunction

Function MarkSuppressed(Actor leashed)
    if leashed == None
        return
    endif
    Int i = FindSuppressSlot(leashed)
    if i < 0
        return
    endif
    SuppressActors[i] = leashed
    SuppressTimes[i] = Utility.GetCurrentRealTime()
EndFunction

Bool Function IsSuppressed(Actor leashed)
    if leashed == None
        return false
    endif
    Int i = FindSuppressIndex(leashed)
    if i < 0
        return false
    endif
    Float last = SuppressTimes[i]
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
    String t = SkyrimNet_Leashed_Native.NormalizeToken(tiePoint)
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
    String t = SkyrimNet_Leashed_Native.NormalizeToken(tiePoint)
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
        Debug.Trace("[SkyrimNet_Leashed] ApplyToHolder refused: " + ActorLabel(holder) + " cannot leash themselves")
        return false
    endif
    bodyPart = NormalizeBodyPart(bodyPart)
    if bodyPart == ""
        bodyPart = "neck"
    endif
    kind = KindForBodyPart(kind, bodyPart)
    Armor leashArmor = ArmorForKind(kind, bodyPart)
    if leashArmor == None
        Debug.Trace("[SkyrimNet_Leashed] ApplyToHolder missing armor kind=" + kind + " body=" + bodyPart)
        return false
    endif
    UnequipStaleBodyArmor(leashed, kind, bodyPart)
    Actor meshOwner = MeshOwnerFor(holder, leashed, kind)
    EquipTypeArmor(meshOwner, leashArmor)
    if !WaitForLeashMesh(meshOwner, leashArmor)
        Debug.Trace("[SkyrimNet_Leashed] ApplyToHolder armor not worn on " + ActorLabel(meshOwner))
        UnequipTypeArmor(meshOwner, leashArmor)
        return false
    endif
    RememberPair(holder, leashed, kind, bodyPart, style, leashDistance, false)
    MarkSuppressed(leashed)
    Debug.Trace("[SkyrimNet_Leashed] ApplyToHolder holder=" + ActorLabel(holder) + " leashed=" + ActorLabel(leashed) + " kind=" + kind + " body=" + bodyPart + " distance=" + leashDistance)
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
        Debug.Trace("[SkyrimNet_Leashed] ApplyToHolder returned false")
        UnequipTypeArmor(meshOwner, leashArmor)
        ForgetPair(leashed)
        return false
    endif
    SkyrimNet_Leashed_Native.NotifyLeash(holder, leashed, kind, leashDistance, bodyPart, false)
    if bodyPart == "wrists"
        ApplyWristBind(leashed)
    endif
    return true
EndFunction

Function DisconnectFramework(Actor leashed)
    if leashed == None || !LeashFramework.IsLeashed(leashed)
        return
    endif
    Actor holder = LeashFramework.GetLeashHolder(leashed)
    if holder
        LeashFramework.DisconnectLeash(holder, leashed)
    else
        LeashFramework.DisconnectLeash(None, leashed)
    endif
EndFunction

Bool Function ApplyDangling(Actor leashed, String style, String leashDistance, String kind, String bodyPart)
    if leashed == None
        return false
    endif
    kind = KindForDangling(kind)
    bodyPart = NormalizeBodyPart(bodyPart)
    if bodyPart == ""
        bodyPart = "neck"
    endif
    kind = KindForBodyPart(kind, bodyPart)
    MarkSuppressed(leashed)
    DisconnectFramework(leashed)
    Armor leashArmor = ArmorForKind(kind, bodyPart)
    UnequipStaleBodyArmor(leashed, kind, bodyPart)
    EquipTypeArmor(leashed, leashArmor)
    RememberPair(None, leashed, kind, bodyPart, style, leashDistance, false)
    MarkSuppressed(leashed)
    Debug.Trace("[SkyrimNet_Leashed] ApplyDangling leashed=" + ActorLabel(leashed) + " kind=" + kind + " body=" + bodyPart + " distance=" + leashDistance)
    SkyrimNet_Leashed_Native.NotifyLeash(None, leashed, kind, leashDistance, bodyPart, false)
    if bodyPart == "wrists"
        ApplyWristBind(leashed)
    endif
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
    bodyPart = NormalizeBodyPart(bodyPart)
    if bodyPart == ""
        bodyPart = "neck"
    endif
    kind = KindForBodyPart(kind, bodyPart)
    Armor leashArmor = ArmorForKind(kind, bodyPart)
    if leashArmor == None
        Debug.Trace("[SkyrimNet_Leashed] ApplyToTiePoint missing armor kind=" + kind + " body=" + bodyPart)
        return false
    endif
    UnequipStaleBodyArmor(leashed, kind, bodyPart)
    EquipTypeArmor(leashed, leashArmor)
    if !WaitForLeashMesh(leashed, leashArmor)
        Debug.Trace("[SkyrimNet_Leashed] ApplyToTiePoint armor not worn on " + ActorLabel(leashed))
        UnequipTypeArmor(leashed, leashArmor)
        return false
    endif
    RememberPair(None, leashed, kind, bodyPart, style, leashDistance, true)
    MarkSuppressed(leashed)
    Float[] xyz = new Float[3]
    OffsetTiePoint(leashed, tiePoint, xyz)
    String bone = ParentBoneFor(kind, bodyPart)
    Bool ok = LeashFramework.ApplyLeashAtPosition(leashed, parentCell, xyz[0], xyz[1], xyz[2], bone, LeashBoneMatch(), DistanceMin(), DistanceMax(leashDistance), true)
    if !ok
        Debug.Trace("[SkyrimNet_Leashed] ApplyToTiePoint returned false")
        UnequipTypeArmor(leashed, leashArmor)
        ForgetPair(leashed)
        return false
    endif
    SkyrimNet_Leashed_Native.NotifyLeash(None, leashed, kind, leashDistance, bodyPart, true)
    if bodyPart == "wrists"
        ApplyWristBind(leashed)
    endif
    return true
EndFunction

; kind: "playerSensitive" (1 if player involved else 2), "stumble" (always 2), "taut" (short-lived event),
; "optional" (DirectNarration only when the player can see and the speech queue is empty).
; extra is the holder or give-receiver so a player in that role counts as involved / able to see.
Function Narrate(String content, Actor originator, Actor target, String kind, Actor leashed, Actor extra)
    if content == ""
        return
    endif
    if kind == "taut"
        String eventId = "leash_taut"
        if leashed
            eventId = "leash_taut_" + leashed.GetFormID()
        endif
        Int ttlMs = (PullCooldown * 1000.0) as Int
        if ttlMs < 1000
            ttlMs = 8000
        endif
        Actor source = originator
        if source == None
            source = leashed
        endif
        SkyrimNetApi.RegisterShortLivedEvent(eventId, "leash", content, "", ttlMs, source, target)
        return
    endif
    Actor player = Game.GetPlayer()
    Bool includesPlayer = originator == player || target == player || leashed == player || extra == player
    Int importance = 2
    if kind != "stumble" && kind != "optional" && includesPlayer
        importance = 1
    endif
    Bool playerCanSee = originator == player || leashed == player || extra == player
    if !playerCanSee && leashed && player.HasLOS(leashed)
        playerCanSee = true
    endif
    if !playerCanSee
        SkyrimNetApi.RegisterEvent("leash", content, originator, target)
        return
    endif
    if kind == "optional"
        if SkyrimNetApi.GetSpeechQueueSize() == 0
            SkyrimNetApi.DirectNarration(content, originator, target)
        else
            SkyrimNetApi.RegisterEvent("leash", content, originator, target)
        endif
        return
    endif
    if importance == 1 || SkyrimNetApi.GetSpeechQueueSize() == 0
        SkyrimNetApi.DirectNarration(content, originator, target)
        return
    endif
    SkyrimNetApi.RegisterEvent("leash", content, originator, target)
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
    String spokenKind = SpokenKind(kind)
    String content = ActorLabel(subject) + " " + StyleWord(style) + " leashes " + Possessive(leashed) + " " + bodyPart + " with a " + leashDistance + " " + spokenKind + " leash"
    if holder
        content += "."
    else
        content += " and leaves it hanging."
    endif
    Narrate(content, subject, NarrateListener(subject, leashed, holder), "playerSensitive", leashed, holder)
EndFunction

Function LeashedToHolder(Actor subject, Actor leashed, Actor holder, String style, String leashDistance, String leashType, String body_part)
    if leashed == None
        Debug.Trace("[SkyrimNet_Leashed] LeashedToHolder skipped: missing leashed")
        return
    endif
    if holder == leashed
        Debug.Trace("[SkyrimNet_Leashed] LeashedToHolder refused: " + ActorLabel(holder) + " cannot leash themselves")
        return
    endif
    String kind = ResolveKind(holder, leashed, leashType)
    String bodyPart = ResolveBodyPart(holder, leashed, body_part)
    String distance = ResolveDistance(leashed, leashDistance)
    if subject == None
        if holder
            subject = holder
        else
            subject = leashed
        endif
    endif
    if holder == None
        kind = KindForDangling(kind)
        if !ApplyDangling(leashed, style, distance, kind, bodyPart)
            Debug.Trace("[SkyrimNet_Leashed] LeashedToHolder dangling failed for " + ActorLabel(leashed))
            return
        endif
        NarrateApply(subject, leashed, None, style, distance, kind, bodyPart)
        return
    endif
    if !ApplyToHolder(holder, leashed, style, distance, kind, bodyPart)
        Debug.Trace("[SkyrimNet_Leashed] LeashedToHolder failed for " + ActorLabel(holder) + " -> " + ActorLabel(leashed))
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
        Debug.Trace("[SkyrimNet_Leashed] TakeLeash failed for " + ActorLabel(subject) + " -> " + ActorLabel(leashed))
        return
    endif
    Narrate(ActorLabel(subject) + " takes " + Possessive(leashed) + " leash.", subject, NarrateListener(subject, leashed, subject), "playerSensitive", leashed, subject)
EndFunction

Function GiveLeash(Actor subject, Actor leashed, Actor receiver)
    if subject == None || leashed == None
        return
    endif
    if receiver == leashed
        Debug.Trace("[SkyrimNet_Leashed] GiveLeash skipped: receiver cannot be the leashed actor")
        return
    endif
    if receiver && receiver == subject
        Debug.Trace("[SkyrimNet_Leashed] GiveLeash skipped: receiver cannot be the subject")
        return
    endif
    String kind = DetectKind(receiver, leashed)
    String bodyPart = DetectBodyPart(receiver, leashed)
    String distance = ResolveDistance(leashed, "")
    String style = CachedStyle(leashed)
    if receiver == None
        kind = KindForDangling(kind)
        if !ApplyDangling(leashed, style, distance, kind, bodyPart)
            Debug.Trace("[SkyrimNet_Leashed] GiveLeash dangling failed for " + ActorLabel(leashed))
            return
        endif
        Narrate(ActorLabel(subject) + " drops " + Possessive(leashed) + " leash.", subject, NarrateListener(subject, leashed, None), "playerSensitive", leashed, None)
        return
    endif
    if !ApplyToHolder(receiver, leashed, style, distance, kind, bodyPart)
        Debug.Trace("[SkyrimNet_Leashed] GiveLeash failed for " + ActorLabel(subject) + " -> " + ActorLabel(receiver))
        return
    endif
    Narrate(ActorLabel(subject) + " gives " + Possessive(leashed) + " leash to " + ActorLabel(receiver) + ".", subject, leashed, "playerSensitive", leashed, receiver)
EndFunction

Function LeashedToTiePoint(Actor subject, Actor leashed, String style, String leashDistance, String leashType, String body_part, String tiePoint)
    if leashed == None
        return
    endif
    ; Retie only moves the world anchor. Keep the current mesh/length even if the
    ; panel sent its default type (rope) because the type pulldown is hidden.
    Bool alreadyLeashed = LeashFramework.IsLeashed(leashed) || FindLeashedIndex(leashed) >= 0
    String kind = ""
    String bodyPart = ""
    String distance = ""
    if alreadyLeashed
        kind = DetectKind(None, leashed)
        bodyPart = DetectBodyPart(None, leashed)
        distance = ResolveDistance(leashed, "")
    else
        kind = ResolveKind(None, leashed, leashType)
        bodyPart = ResolveBodyPart(None, leashed, body_part)
        distance = ResolveDistance(leashed, leashDistance)
    endif
    String point = NormalizeTiePoint(tiePoint)
    if subject == None
        subject = leashed
    endif
    if !ApplyToTiePoint(leashed, style, distance, kind, bodyPart, point)
        Debug.Trace("[SkyrimNet_Leashed] LeashedToTiePoint failed for " + ActorLabel(leashed))
        return
    endif
    Narrate(ActorLabel(subject) + " ties " + Possessive(leashed) + " leash to the " + point + ".", subject, NarrateListener(subject, leashed, None), "playerSensitive", leashed, None)
EndFunction

Function LeashedRefused(Actor subject, Actor leashed)
    if subject == None || leashed == None
        return
    endif
    Narrate(ActorLabel(leashed) + " refused to be leashed by " + ActorLabel(subject) + ".", leashed, subject, "playerSensitive", leashed, subject)
EndFunction

String Function StruggleAnimEvent(String bodyPart)
    if bodyPart == "wrists"
        return "DDRegCuffsFrontStruggle01"
    elseif bodyPart == "waist"
        return "DDChastityBeltStruggle01"
    endif
    return "DDCollarStruggle01"
EndFunction

Function PlayStruggleAnim(Actor who)
    if who == None
        return
    endif
    String eventName = StruggleAnimEvent(CachedBodyPart(who))
    Debug.Trace("[SkyrimNet_Leashed] PlayStruggleAnim " + ActorLabel(who) + " " + eventName)
    Debug.SendAnimationEvent(who, eventName)
EndFunction

Function StopStruggleAnim(Actor who)
    if who
        Debug.SendAnimationEvent(who, "IdleForceDefaultState")
        if CachedBodyPart(who) == "wrists"
            ApplyWristBind(who)
        endif
    endif
EndFunction

Bool Function ActorIsLocomoting(Actor who)
    if who == None
        return false
    endif
    if who.IsRunning() || who.IsSprinting() || who.IsSwimming()
        return true
    endif
    return who.GetAnimationVariableFloat("Speed") > 10.0
EndFunction

Function EndStruggle(Actor who, Bool narrateStop)
    if who == None
        return
    endif
    Int i = FindStruggleIndex(who)
    if i < 0
        return
    endif
    ClearStruggleSlot(i)
    SkyrimNet_Leashed_Native.NotifyStruggle(who, false)
    StopStruggleAnim(who)
    RefreshStruggleUpdates()
    if narrateStop
        Actor holder = LeashFramework.GetLeashHolder(who)
        if holder == None
            holder = CachedHolder(who)
        endif
        Narrate(ActorLabel(who) + " stops struggling against the leash.", who, NarrateListener(who, who, holder), "playerSensitive", who, holder)
    endif
EndFunction

Float Function EscapeDirectNarrateWait()
    Float interval = SkyrimNet_Leashed_Native.StruggleNarrationInterval()
    if interval < 1.0
        interval = 5.0
    endif
    Float cooldown = SkyrimNet_Leashed_Native.StruggleCooldown()
    if cooldown < 1.0
        cooldown = 20.0
    endif
    if cooldown > interval
        return cooldown
    endif
    return interval
EndFunction

Function PulseStruggleEvent(Actor subject)
    if subject == None
        return
    endif
    String eventId = "leash_struggle_" + subject.GetFormID()
    Actor holder = LeashFramework.GetLeashHolder(subject)
    if holder == None
        holder = CachedHolder(subject)
    endif
    Actor target = holder
    if holder == None || holder == subject
        target = None
    endif
    String content = ActorLabel(subject) + " continues to struggle with " + PossessivePronoun(subject) + " leash."
    Debug.Trace("[SkyrimNet_Leashed] PulseStruggleEvent " + content)
    SkyrimNetApi.RegisterShortLivedEvent(eventId, "leash", content, "", 1500, subject, target)
EndFunction

Function NarrateStillStruggling(Actor subject)
    if subject == None
        return
    endif
    Actor holder = LeashFramework.GetLeashHolder(subject)
    if holder == None
        holder = CachedHolder(subject)
    endif
    String content = "Despite " + Possessive(subject) + " attempts, the leash holds."
    Debug.Trace("[SkyrimNet_Leashed] NarrateStillStruggling " + content)
    Narrate(content, subject, NarrateListener(subject, subject, holder), "optional", subject, holder)
EndFunction

Event OnUpdate()
    EnsureCache()
    Float now = Utility.GetCurrentRealTime()
    Float wait = EscapeDirectNarrateWait()
    Int i = 0
    while i < StrugglingActors.Length
        Actor who = StrugglingActors[i]
        if who
            if who.IsDead() || (!LeashFramework.IsLeashed(who) && FindLeashedIndex(who) < 0)
                EndStruggle(who, false)
            else
                PlayStruggleAnim(who)
                PulseStruggleEvent(who)
                if (now - StruggleLastNarrate[i]) >= wait
                    StruggleLastNarrate[i] = now
                    NarrateStillStruggling(who)
                endif
            endif
        endif
        i += 1
    endwhile
EndEvent

Function StruggleExecute(Actor subject)
    if subject == None
        Debug.Trace("[SkyrimNet_Leashed] StruggleExecute skipped: missing subject")
        return
    endif
    Debug.Trace("[SkyrimNet_Leashed] StruggleExecute " + ActorLabel(subject) + " dd")
    if !LeashFramework.IsLeashed(subject) && FindLeashedIndex(subject) < 0
        Debug.Trace("[SkyrimNet_Leashed] StruggleExecute skipped: " + ActorLabel(subject) + " is not leashed")
        return
    endif
    if FindStruggleIndex(subject) >= 0
        Debug.Trace("[SkyrimNet_Leashed] StruggleExecute skipped: already struggling " + ActorLabel(subject))
        return
    endif
    Int i = EnsureStruggleIndex(subject)
    if i < 0
        Debug.Trace("[SkyrimNet_Leashed] StruggleExecute skipped: no struggle slot for " + ActorLabel(subject))
        return
    endif
    Actor holder = LeashFramework.GetLeashHolder(subject)
    if holder == None
        holder = CachedHolder(subject)
    endif
    String kind = SpokenKind(DetectKind(holder, subject))
    String bodyPart = DetectBodyPart(holder, subject)
    StruggleLastNarrate[i] = Utility.GetCurrentRealTime()
    SkyrimNet_Leashed_Native.NotifyStruggle(subject, true)
    RefreshStruggleUpdates()
    PlayStruggleAnim(subject)
    PulseStruggleEvent(subject)
    Narrate(ActorLabel(subject) + " struggles against the " + kind + " leash at their " + bodyPart + ", but it holds.", subject, NarrateListener(subject, subject, holder), "playerSensitive", subject, holder)
EndFunction

Function StopStruggleExecute(Actor subject)
    if subject == None
        Debug.Trace("[SkyrimNet_Leashed] StopStruggleExecute skipped: missing subject")
        return
    endif
    Debug.Trace("[SkyrimNet_Leashed] StopStruggleExecute " + ActorLabel(subject))
    if FindStruggleIndex(subject) < 0
        Debug.Trace("[SkyrimNet_Leashed] StopStruggleExecute skipped: not struggling " + ActorLabel(subject))
        return
    endif
    Actor holder = LeashFramework.GetLeashHolder(subject)
    if holder == None
        holder = CachedHolder(subject)
    endif
    EndStruggle(subject, false)
    Narrate(ActorLabel(subject) + " stops struggling to remove their leash.", subject, NarrateListener(subject, subject, holder), "optional", subject, holder)
EndFunction

Function UnleashTargetExecute(Actor subject, Actor target)
    if subject == None || target == None
        return
    endif
    Actor holder = None
    Actor leashed = None
    if LeashFramework.IsLeashed(subject) && LeashFramework.GetLeashHolder(subject) == target
        holder = target
        leashed = subject
    elseif LeashFramework.IsLeashed(target)
        leashed = target
        holder = LeashFramework.GetLeashHolder(target)
        if holder == None
            holder = CachedHolder(target)
        endif
    elseif FindLeashedIndex(subject) >= 0 && CachedHolder(subject) == None
        leashed = subject
        holder = None
    elseif FindLeashedIndex(target) >= 0
        leashed = target
        holder = CachedHolder(target)
    endif
    if leashed == None
        Debug.Trace("[SkyrimNet_Leashed] UnleashTargetExecute skipped: could not resolve leashed")
        return
    endif
    if leashed == subject
        Debug.Trace("[SkyrimNet_Leashed] UnleashTargetExecute rejected: speaker cannot free themselves")
        return
    endif
    String kind = DetectKind(holder, leashed)
    String bodyPart = DetectBodyPart(holder, leashed)
    MarkSuppressed(leashed)
    Bool ok = false
    if holder == None
        ok = LeashFramework.DisconnectLeash(None, leashed)
    else
        ok = LeashFramework.DisconnectLeash(holder, leashed)
    endif
    if !ok
        ok = LeashFramework.DisconnectLeash(subject, target)
    endif
    if !ok
        ok = LeashFramework.DisconnectLeash(target, subject)
    endif
    Bool recorded = FindLeashedIndex(leashed) >= 0
    if ok || recorded
        EndStruggle(leashed, false)
        Narrate(ActorLabel(subject) + " unclips the " + kind + " leash from " + Possessive(leashed) + " " + bodyPart + ".", subject, NarrateListener(subject, leashed, holder), "playerSensitive", leashed, holder)
        UnequipPairArmor(holder, leashed, kind, bodyPart)
        ForgetPair(leashed)
        SkyrimNet_Leashed_Native.NotifyUnleash(leashed)
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
            EndStruggle(leashed, false)
            Narrate(ActorLabel(originator) + " unclips the " + kind + " leash from " + Possessive(leashed) + " " + bodyPart + ".", originator, NarrateListener(originator, leashed, holder), "playerSensitive", leashed, holder)
        endif
        i += 1
    endwhile
    if LeashFramework.IsLeashed(subject) && FindLeashedIndex(subject) < 0
        Actor holder = LeashFramework.GetLeashHolder(subject)
        String kind = DetectKind(holder, subject)
        String bodyPart = DetectBodyPart(holder, subject)
        MarkSuppressed(subject)
        Narrate(ActorLabel(subject) + " unclips the " + kind + " leash from " + Possessive(subject) + " " + bodyPart + ".", subject, holder, "playerSensitive", subject, holder)
    endif
    LeashFramework.UnleashAll(subject)
    UnequipTrackedFor(subject)
    EndStruggle(subject, false)
    SkyrimNet_Leashed_Native.NotifyUnleash(subject)
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
        if CachedIsTied(leashed) || FindLeashedIndex(leashed) < 0
            content = "A " + distance + " " + kind + " leash is tied to " + Possessive(leashed) + " " + bodyPart + "."
        else
            content = "A " + distance + " " + kind + " leash hangs from " + Possessive(leashed) + " " + bodyPart + "."
        endif
        originator = None
        targetActor = leashed
    else
        content = ActorLabel(holder) + " " + StyleWord(style) + " leashes " + Possessive(leashed) + " " + bodyPart + " with a " + distance + " " + kind + " leash."
    endif
    Narrate(content, originator, targetActor, "playerSensitive", leashed, holder)
EndFunction

Function NarrateUnleash(Actor leashed, String reason)
    if leashed == None || reason == "replaced"
        return
    endif
    ; ApplyDangling / explicit unleash MarkSuppressed then Disconnect. The delayed
    ; OnUnleash must not unequip the new collar or wipe the dangling/held cache.
    if IsSuppressed(leashed)
        return
    endif
    Actor holder = CachedHolder(leashed)
    if holder == None
        holder = LeashFramework.GetLeashHolder(leashed)
    endif
    String kind = DetectKind(holder, leashed)
    String bodyPart = DetectBodyPart(holder, leashed)
    Actor originator = holder
    Actor targetActor = leashed
    String content = ""
    if holder == None
        content = "The " + kind + " leash is unclipped from " + Possessive(leashed) + " " + bodyPart + "."
        originator = None
    else
        content = ActorLabel(holder) + " unclips the " + kind + " leash from " + Possessive(leashed) + " " + bodyPart + "."
    endif
    Narrate(content, originator, targetActor, "playerSensitive", leashed, holder)
    UnequipPairArmor(holder, leashed, kind, bodyPart)
    ForgetPair(leashed)
EndFunction

Bool Function ConsumePullCooldown(Actor leashed, Bool ragdoll)
    if leashed == None
        return false
    endif
    EnsureCache()
    Int i = FindLeashedIndex(leashed)
    if i < 0
        Bool tied = leashed && LeashFramework.IsLeashed(leashed) && LeashFramework.GetLeashHolder(leashed) == None
        RememberPair(None, leashed, DetectKind(None, leashed), DetectBodyPart(None, leashed), "normally", ResolveDistance(leashed, ""), tied)
        i = FindLeashedIndex(leashed)
    endif
    if i < 0
        return true
    endif
    Float now = Utility.GetCurrentRealTime()
    if ragdoll
        if now - LastPullTimes[i] < PullCooldown
            return false
        endif
        LastPullTimes[i] = now
        return true
    endif
    if now - LastTautTimes[i] < PullCooldown
        return false
    endif
    LastTautTimes[i] = now
    return true
EndFunction

Function NarratePull(Actor leashed, Bool ragdoll)
    if leashed == None
        return
    endif
    if !ConsumePullCooldown(leashed, ragdoll)
        if ragdoll
            Debug.Trace("[SkyrimNet_Leashed] ragdoll pull skipped: cooldown " + ActorLabel(leashed))
        else
            Debug.Trace("[SkyrimNet_Leashed] taut pull skipped: cooldown " + ActorLabel(leashed))
        endif
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
    String pullKind = "taut"
    if ragdoll
        pullKind = "stumble"
    endif
    if ragdoll
        Debug.Trace("[SkyrimNet_Leashed] ragdoll pull " + leashedName)
    else
        Debug.Trace("[SkyrimNet_Leashed] taut pull " + leashedName)
    endif
    Narrate(content, originator, targetActor, pullKind, leashed, holder)
EndFunction

; The SKSE side cannot reach LeashFramework's leash table, so every lifecycle event pushes
; the authoritative holder across. This runs before the narration filters, which skip the
; "loaded" and "replaced" reasons that still need to update the recorded pair.
Event OnLeashFrameworkLeash(String eventName, String strArg, Float numArg, Form sender)
    Actor leashed = sender as Actor
    if leashed
        Actor holder = LeashFramework.GetLeashHolder(leashed)
        Bool tied = holder == None
        SkyrimNet_Leashed_Native.NotifyLeash(holder, leashed, DetectKind(holder, leashed), ResolveDistance(leashed, ""), DetectBodyPart(holder, leashed), tied)
    endif
    NarrateLeash(leashed, strArg)
EndEvent

Event OnLeashFrameworkUnleash(String eventName, String strArg, Float numArg, Form sender)
    Actor leashed = sender as Actor
    ; A replacement sends OnUnleash before OnLeash; keep the old holder until the new one
    ; arrives so the pair never falls back to a guess in between.
    ; Suppressed disconnects (ApplyDangling) already wrote the new native record.
    if leashed && strArg != "replaced" && !IsSuppressed(leashed)
        EndStruggle(leashed, false)
        SkyrimNet_Leashed_Native.NotifyUnleash(leashed)
    endif
    NarrateUnleash(leashed, strArg)
EndEvent

Event OnLeashFrameworkPulled(String eventName, String strArg, Float numArg, Form sender)
    Actor leashed = sender as Actor
    EndStruggle(leashed, false)
    NarratePull(leashed, false)
EndEvent

Event OnLeashFrameworkRagdollPulled(String eventName, String strArg, Float numArg, Form sender)
    Actor leashed = sender as Actor
    EndStruggle(leashed, false)
    NarratePull(leashed, true)
    if leashed && CachedBodyPart(leashed) == "wrists"
        RegisterForAnimationEvent(leashed, "GetUpEnd")
    endif
EndEvent

Event OnAnimationEvent(ObjectReference akSource, String asEventName)
    if asEventName != "GetUpEnd"
        return
    endif
    Actor leashed = akSource as Actor
    UnregisterForAnimationEvent(akSource, "GetUpEnd")
    if leashed && CachedBodyPart(leashed) == "wrists"
        ApplyWristBind(leashed)
    endif
EndEvent
