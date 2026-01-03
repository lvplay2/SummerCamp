// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCMenuWidget.h"
#include "SCInputMappingsSaveGame.h"
#include "SCCustomInput.generated.h"

class UILLLocalPlayer;

/**
 * @class USCCustomInput
 * @Menu class for modifying custom inputs for end user.
 */
UCLASS()
class SUMMERCAMP_API USCCustomInput // CLEANUP: cpederson: This is named wrong
: public USCMenuWidget
{
	GENERATED_UCLASS_BODY()

	// Applies the current settings to the save file
	UFUNCTION(BlueprintCallable, Category = "Input Mapping")
	void ApplySettings();

	// Does not save the current changes and reloads the save to undo any input mapping changes.
	UFUNCTION(BlueprintCallable, Category = "Input Mapping")
	void DiscardSettings();

	// Restore engine default mappings.
	UFUNCTION(BlueprintCallable, Category = "Input Mapping")
	void ResetSettings();

protected:

	// This is our list of editable commands.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Mapping")
	TArray<struct FILLCustomKeyBinding> EditableActionMappings;
};
