// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLPlayerSettingsSaveGame.h"
#include "ILLInputMappingsSaveGame.generated.h"

/**
 * @class UILLInputMappingsSaveGame
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLInputMappingsSaveGame
: public UILLPlayerSettingsSaveGame
{
	GENERATED_UCLASS_BODY()

	// Begin UILLPlayerSettingsSaveGame interface
	virtual void ApplyPlayerSettings(AILLPlayerController* PlayerController, const bool bAndSave = true) override;
	virtual bool HasChangedFrom(UILLPlayerSettingsSaveGame* OtherSave) const override;
	virtual void SaveGameLoaded(AILLPlayerController* PlayerController) override;
	// End UILLPlayerSettingsSaveGame interface

	// Update action mapping for selected action with the new chord
	UFUNCTION()
	virtual bool RemapPCInputAction(AILLPlayerController* PlayerController, const FName ActionName, const FInputChord NewChord);

	// Update axis mapping for selected axis with the key
	UFUNCTION()
	virtual bool RemapPCInputAxis(AILLPlayerController* PlayerController, const FName AxisName, const FKey Key, const float Scale);

	// Applies the current settings to the save file
	UFUNCTION()
	virtual void ApplySettings(AILLPlayerController* PlayerController);

	// Does not save the current changes and reloads the save to undo any input mapping changes
	UFUNCTION()
	virtual void DiscardSettings(AILLPlayerController* PlayerController);

	// Reset the save data to engine default input action bindings
	UFUNCTION()
	virtual void ResetSettings(AILLPlayerController* PlayerController);

	// Set the passed in action and axis mappings to the data in the save file rather than engine default
	UFUNCTION()
	virtual bool SetCustomKeybinds(TArray<FInputActionKeyMapping>& InActionMapping, TArray<FInputAxisKeyMapping>& InAxisMappings);

	/** Returns true if the user has custom keybindings set */
	UFUNCTION()
	virtual bool HasCustomKeybinds();

	// List of action mappings
	UPROPERTY()
	TArray<FInputActionKeyMapping> ActionMappings;

	// List of axis mappings
	UPROPERTY()
	TArray<FInputAxisKeyMapping> AxisMappings;
};
