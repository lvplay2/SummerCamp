// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLInputMappingTypes.generated.h"

UENUM(BlueprintType)
enum class EInputType : uint8
{
	Keyboard	UMETA(DisplayName="Keyboard/Mouse"),
	Gamepad		UMETA(DisplayName="Gamepad"),
	Both		UMETA(DisplayName="Both"),

	MAX			UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct ILLGAMEFRAMEWORK_API FILLInputActionKeyMapping
{
	GENERATED_USTRUCT_BODY()

	/** Name of the mapping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	FText MappingName;

	/** Key to bind it to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	FKey Key;

	/** true if one of the Shift keys must be down when the KeyEvent is received to be acknowledged */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	uint32 bShift : 1;

	/** true if one of the Ctrl keys must be down when the KeyEvent is received to be acknowledged */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	uint32 bCtrl : 1;

	/** true if one of the Alt keys must be down when the KeyEvent is received to be acknowledged */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	uint32 bAlt : 1;

	/** true if one of the Cmd keys must be down when the KeyEvent is received to be acknowledged */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	uint32 bCmd : 1;

	/* Alternative Key Index (for things like multiple mouse mappings) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	int32 AltKeyIndex;

};

USTRUCT(BlueprintType)
struct ILLGAMEFRAMEWORK_API FILLInputAxisMapping
{
	GENERATED_USTRUCT_BODY()

	/** Name of the mapping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	FText MappingName;

	/** Key to bind it to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	FKey Key;

	/** Multiplier to use for the mapping when accumulating the axis value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	float Scale;
};
