Scriptname SkyrimNet_Leash_PlayerAlias extends ReferenceAlias

Event OnPlayerLoadGame()
    SkyrimNet_Leash_Actions actions = GetOwningQuest() as SkyrimNet_Leash_Actions
    if actions
        actions.RegisterLeashEvents()
    endif
EndEvent
