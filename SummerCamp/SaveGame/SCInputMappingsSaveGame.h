// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLInputMappingsSaveGame.h"
#include "SCInputMappingsSaveGame.generated.h"

// F13 cusom keybinding categories
UENUM(BlueprintType)
enum class ESCCustomKeybindCategory : uint8
{
	 Global,
	 Counselor,
	 Killer,
	 Vehicle,
	 Spectator,
	 VirtualCabin,
	 CounselorCombat,
	 KillerCombat,
	 Minigame,
};

// Custom keybinding object so that actions and axes can be rebound to new keys
USTRUCT( BlueprintType )
struct FILLCustomKeyBinding
{
	GENERATED_USTRUCT_BODY()

	// Is this binding for an axis or an action?
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InputMapping")
	bool bIsAxisBinding;

	// The displayed name for this binding
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Mapping")
	FText DisplayName;

	// A list of actions to rebind to the new key
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Mapping")
	TArray<FName> BoundActions;

	// Allows the Shift, Ctrl, Alt, and Cmd keys to be pressed as modifiers for other key chords (making A, Shift + A, and Ctrl + A all valid and separate key chords)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Mapping", meta=(EditCondition="!bIsAxisBinding"))
	bool bAllowModifiers;

	// If this is an axis what scale does this binding require?
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Mapping", meta=(EditCondition="bIsAxisBinding"))
	float Scale;

	// Which input category does the binding fall under? This keeps overlapping keys from separate categories from unbinding eachother.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Mapping")
	ESCCustomKeybindCategory Category;

	// Build default empty binding.
	FILLCustomKeyBinding()
	: Scale(1.0f)
	{}
};

/**
 * @class USCInputMappingsSaveGame
 */
UCLASS()
class USCInputMappingsSaveGame
: public UILLInputMappingsSaveGame
{
	GENERATED_UCLASS_BODY()

	// Update action mapping for selected action with the new chord within the given category
	virtual bool RemapPCInputActionByCategory(AILLPlayerController* PlayerController, const FName ActionName, const FInputChord NewChord, FILLCustomKeyBinding Binding);

	// Update axis mapping for selected axis with the key within the given category
	virtual bool RemapPCInputAxisByCategory(AILLPlayerController* PlayerController, const FName AxisName, const FKey Key, FILLCustomKeyBinding Binding);

	FString CategoryToLiteral(ESCCustomKeybindCategory InCategory);
};
