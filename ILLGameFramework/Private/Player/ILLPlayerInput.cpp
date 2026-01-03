// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLPlayerInput.h"
#include "ILLLocalPlayer.h"
#include "ILLInputMappingsSaveGame.h"

UILLPlayerInput::UILLPlayerInput()
{
#if PLATFORM_XBOXONE || PLATFORM_PS4
	bUsingGamepad = true;
#endif
}

void UILLPlayerInput::SetUsingGamepad(const bool bNewUsingGamepad)
{
	if (bUsingGamepad != bNewUsingGamepad)
	{
		bUsingGamepad = bNewUsingGamepad;

		// Broadcast the change
		OnUsingGamepadChanged.Broadcast(bUsingGamepad);
	}
}

void UILLPlayerInput::ForceRebuildingKeyMaps(const bool bRestoreDefaults)
{
	Super::ForceRebuildingKeyMaps(bRestoreDefaults);

	if (!GIsEditor)// This shit breaks when loading up the editor... So don't let it.
	{
		// Load our custom Action mappings.
		if (AILLPlayerController* PC = Cast<AILLPlayerController>(GetOuterAPlayerController())) // Manual cast catches if the controller is garbage, that way we don't crash when trying to access it.
		{
			if (UILLLocalPlayer* LocalPlayer = Cast<UILLLocalPlayer>(PC->GetLocalPlayer()))
			{
				if (UILLInputMappingsSaveGame* SaveGame = LocalPlayer->GetLoadedSettingsSave<UILLInputMappingsSaveGame>())
				{
					SaveGame->SetCustomKeybinds(ActionMappings, AxisMappings);
				}
			}
		}
	}
}
