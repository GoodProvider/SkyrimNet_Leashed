Scriptname SkyrimNet_Leash_Actions extends Quest

String Function ParentBone()
    return "NPC Spine2 [Spn2]"
EndFunction

String Function LeashBoneMatch()
    return "Leash1_1"
EndFunction

Bool Function ApplyDefaultLeash(Actor holder, Actor leashed, String style)
    if holder == None || leashed == None
        return false
    endif
    Debug.Trace("[SkyrimNet_Leash] ApplyLeash holder=" + holder.GetDisplayName() + " leashed=" + leashed.GetDisplayName() + " style=" + style)
    Bool ok = LeashFramework.ApplyLeash(holder, leashed, ParentBone(), LeashBoneMatch(), 200.0, 300.0, true)
    if !ok
        Debug.Trace("[SkyrimNet_Leash] ApplyLeash returned false")
    endif
    return ok
EndFunction

Function LeashExecute(Actor akActor, Actor target, String style)
    ApplyDefaultLeash(akActor, target, style)
EndFunction

Function TargetLeashesSpeakerExecute(Actor akActor, Actor target, String style)
    ApplyDefaultLeash(target, akActor, style)
EndFunction

Function UnleashExecute(Actor akActor, Actor target)
    if akActor == None
        return
    endif
    if target != None
        if LeashFramework.DisconnectLeash(akActor, target)
            return
        endif
        if LeashFramework.DisconnectLeash(target, akActor)
            return
        endif
    endif
    LeashFramework.UnleashAll(akActor)
EndFunction
