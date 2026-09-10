Scriptname SkyrimNet_Leashed_PlayerAlias extends ReferenceAlias

Event OnPlayerLoadGame()
    SkyrimNet_Leashed_Actions actions = GetOwningQuest() as SkyrimNet_Leashed_Actions
    if actions
        actions.RegisterLeashEvents()
    endif
EndEvent
