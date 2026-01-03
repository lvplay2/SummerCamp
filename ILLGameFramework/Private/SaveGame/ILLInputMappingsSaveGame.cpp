// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLInputMappingsSaveGame.h"

#include "ILLPlayerController.h"
#include "ILLLocalPlayer.h"
#include "ILLActionMapWidget.h"

UILLInputMappingsSaveGame::UILLInputMappingsSaveGame(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void UILLInputMappingsSaveGame::ApplyPlayerSettings(AILLPlayerController* PlayerController, const bool bAndSave)
{
	// There's no saved mappings... This means the save game was just created, set the defaults.
	if (ActionMappings.Num() == 0)
	{
		ActionMappings = PlayerController->PlayerInput->ActionMappings;
	}

	if (AxisMappings.Num() == 0)
	{
		AxisMappings = PlayerController->PlayerInput->AxisMappings;
	}

	Super::ApplyPlayerSettings(PlayerController, bAndSave);

	PlayerController->PlayerInput->ForceRebuildingKeyMaps(false);

	if (UWorld* World = PlayerController->GetWorld())
	{
		// Iterate over the active widgets and update their mappings.
		for (TObjectIterator<UILLActionMapWidget> It; It; ++It)
		{
			if (It->GetWorld() == World)
				It->CacheKeyInfo();
		}
	}
}

bool UILLInputMappingsSaveGame::HasChangedFrom(UILLPlayerSettingsSaveGame* OtherSave) const
{
	return false;
}

void UILLInputMappingsSaveGame::SaveGameLoaded(AILLPlayerController* PlayerController)
{
	// There's no saved action mappings, set the defaults.
	if (ActionMappings.Num() == 0)
	{
		ActionMappings = PlayerController->PlayerInput->ActionMappings;
	}
	else
	{
		const TArray<FInputActionKeyMapping> Defaults = PlayerController->PlayerInput->ActionMappings;

		// Remove deprecated bindings
		for (int ActionIndex = ActionMappings.Num() -1; ActionIndex > 0; --ActionIndex)
		{
			bool FoundMatch = false;
			for (int DefaultIndex = Defaults.Num() -1; DefaultIndex > 0; --DefaultIndex)
			{
				if (ActionMappings[ActionIndex].ActionName == Defaults[DefaultIndex].ActionName)
				{
					FoundMatch = true;
					break;
				}
			}

			// Didn't find a match, remove this boy.
			if (!FoundMatch)
			{
				ActionMappings.RemoveAtSwap(ActionIndex);
			}
		}

		// Add unknown actions
		for (int DefaultIndex = Defaults.Num() -1; DefaultIndex > 0; --DefaultIndex)
		{
			bool FoundMatch = false;
			for (int ActionIndex = ActionMappings.Num() -1; ActionIndex > 0; --ActionIndex)
			{
				if (ActionMappings[ActionIndex].ActionName == Defaults[DefaultIndex].ActionName)
				{
					FoundMatch = true;
					break;
				}
			}

			// Didn't find a match, this must be new.
			if (!FoundMatch)
			{
				ActionMappings.Add(Defaults[DefaultIndex]);
			}
		}
	}

	if (AxisMappings.Num() == 0)
	{
		AxisMappings = PlayerController->PlayerInput->AxisMappings;
	}
	else
	{
		const TArray<FInputAxisKeyMapping> Defaults = PlayerController->PlayerInput->AxisMappings;

		// Remove deprecated bindings
		for (int ActionIndex = AxisMappings.Num() -1; ActionIndex > 0; --ActionIndex)
		{
			bool FoundMatch = false;
			for (int DefaultIndex = Defaults.Num() -1; DefaultIndex > 0; --DefaultIndex)
			{
				if (AxisMappings[ActionIndex].AxisName == Defaults[DefaultIndex].AxisName)
				{
					FoundMatch = true;
					break;
				}
			}

			// Didn't find a match, remove this boy.
			if (!FoundMatch)
			{
				AxisMappings.RemoveAtSwap(ActionIndex);
			}
		}

		// Add unknown actions
		for (int DefaultIndex = Defaults.Num() -1; DefaultIndex > 0; --DefaultIndex)
		{
			bool FoundMatch = false;
			for (int ActionIndex = AxisMappings.Num() -1; ActionIndex > 0; --ActionIndex)
			{
				if (AxisMappings[ActionIndex].AxisName == Defaults[DefaultIndex].AxisName)
				{
					FoundMatch = true;
					break;
				}
			}

			// Didn't find a match, this must be new.
			if (!FoundMatch)
			{
				AxisMappings.Add(Defaults[DefaultIndex]);
			}
		}
	}

	Super::SaveGameLoaded(PlayerController);
}

bool UILLInputMappingsSaveGame::RemapPCInputAction(AILLPlayerController* PlayerController, const FName ActionName, const FInputChord NewChord)
{
	if (!PlayerController)
		return false;

	if (ActionName.IsNone())
		return false;

	// remove any actions currently bound to the Action.
	for (int idx = ActionMappings.Num() - 1; idx >= 0; --idx)
	{
		FInputActionKeyMapping& Mapping = ActionMappings[idx];

		// We don't want to remove gamepad controls here.
		if (Mapping.ActionName == ActionName && !Mapping.Key.IsGamepadKey())
		{
			// RemoveAtSwap by index is faster and cheaper than RemoveSwap by object...
			ActionMappings.RemoveAtSwap(idx, 1, false);
		}
	}

	// Add the new action to our map.
	ActionMappings.Add(FInputActionKeyMapping(ActionName, NewChord.Key, NewChord.bShift, NewChord.bCtrl, NewChord.bAlt, NewChord.bCmd));

	return true;
}

bool UILLInputMappingsSaveGame::RemapPCInputAxis(AILLPlayerController* PlayerController, const FName AxisName, const FKey Key, const float Scale)
{
	if (!PlayerController)
		return false;

	if (AxisName.IsNone())
		return false;

	// remove any actions currently bound to the Action.
	for (int idx = AxisMappings.Num() - 1; idx >= 0; --idx)
	{
		FInputAxisKeyMapping& Mapping = AxisMappings[idx];

		// We don't want to remove gamepad controls here.
		if (Mapping.AxisName == AxisName && Mapping.Scale == Scale && !Mapping.Key.IsGamepadKey())
		{
			// RemoveAtSwap by index is faster and cheaper than RemoveSwap by object...
			AxisMappings.RemoveAtSwap(idx, 1, false);
		}
	}

	// Add the new action to our map.
	AxisMappings.Add(FInputAxisKeyMapping(AxisName, Key, Scale));

	return true;
}

void UILLInputMappingsSaveGame::ApplySettings(AILLPlayerController* PlayerController)
{
	ApplyPlayerSettings(PlayerController, true);
}

void UILLInputMappingsSaveGame::DiscardSettings(AILLPlayerController* PlayerController)
{
	// Reload our action mappings from the player controller since we're discarding changes.
	ActionMappings = PlayerController->PlayerInput->ActionMappings;
	AxisMappings = PlayerController->PlayerInput->AxisMappings;
}

void UILLInputMappingsSaveGame::ResetSettings(AILLPlayerController* PlayerController)
{
	// Load default settings from engine defaults and save.
	ActionMappings = GetDefault<UInputSettings>()->ActionMappings;
	AxisMappings = GetDefault<UInputSettings>()->AxisMappings;
	ApplyPlayerSettings(PlayerController, true);
}

bool UILLInputMappingsSaveGame::SetCustomKeybinds(TArray<FInputActionKeyMapping>& InActionMapping, TArray<FInputAxisKeyMapping>& InAxisMappings)
{
	// If we don't have any save data leave the action and axis mappings as they are from the engine.
	if (ActionMappings.Num() == 0 || AxisMappings.Num() == 0)
		return false;

	// Set the pass in binding arrays to the dave data values.
	InActionMapping = ActionMappings;
	InAxisMappings = AxisMappings;
	return true;
}

bool UILLInputMappingsSaveGame::HasCustomKeybinds()
{
	return !(ActionMappings == GetDefault<UInputSettings>()->ActionMappings &&
			AxisMappings == GetDefault<UInputSettings>()->AxisMappings);
}
