// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCUserWidget.h"
#include "SCInputMappingsSaveGame.h"
#include "SCRebindingWidget.generated.h"

class UILLLocalPlayer;

USTRUCT(BlueprintType)
struct FBPInputChord
{
	GENERATED_USTRUCT_BODY()

	/** The Key is the core of the chord. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FKey Key;

	/** Whether the shift key is part of the chord.  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint32 bShift:1;

	/** Whether the control key is part of the chord.  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint32 bCtrl:1;

	/** Whether the alt key is part of the chord.  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint32 bAlt:1;

	/** Whether the command key is part of the chord.  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint32 bCmd:1;

	// Converts the Blueprint Input Chord to a native FInputChord
	FInputChord ToChord()
	{
		return FInputChord(Key, bShift, bCtrl, bAlt, bCmd);
	}
};

/**
* @class USCCustomInput
* @Menu class for modifying custom inputs for end user.
*/
UCLASS()
class SUMMERCAMP_API USCRebindingWidget
	: public USCUserWidget
{
	GENERATED_UCLASS_BODY()

	// The keybinding action(s) that this widget is displaying
	UPROPERTY(BlueprintReadWrite, Category = "Input Mapping")
	FILLCustomKeyBinding TiedBinding;

	// Attempt to bind the new input chord to the action(s)
	UFUNCTION(BlueprintCallable, Category = "Input Mapping")
	void BeginRebind(FBPInputChord NewChord);

	// Return the full string name for the input chord (E.G. Ctrl + Shift + A)
	UFUNCTION(BlueprintCallable, Category = "Input Mapping")
	FText GetBoundKeyName();

	// Initialize widget with the given keybinding action(s)
	UFUNCTION(BlueprintCallable, Category = "Input Mapping")
	void InitializeBinding(FILLCustomKeyBinding InKeyBinding);

	// The key this binding is currently set to (Set from playerInput until manually changed.
	UPROPERTY(BlueprintReadOnly, Category = "Input Mapping")
	FKey ReboundToKey;
};
