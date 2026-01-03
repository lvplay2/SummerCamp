// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "GameFramework/PlayerInput.h"
#include "ILLPlayerController.h"
#include "ILLPlayerInput.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FILLPlayerInputUsingGamepad, bool, bUsingGamepad);

/**
 * @class UILLPlayerInput
 */
UCLASS(Within=ILLPlayerController, Config=Input, Transient)
class ILLGAMEFRAMEWORK_API UILLPlayerInput
: public UPlayerInput
{
	GENERATED_BODY()

public:
	UILLPlayerInput();

private:
	/** Clear the current cached key maps and rebuild from the source arrays. */
	virtual void ForceRebuildingKeyMaps(const bool bRestoreDefaults = false) override;

public:
	/** @return true if this player has recently used the gamepad. */
	FORCEINLINE bool IsUsingGamepad() const { return bUsingGamepad; }

	/** Updates bUsingGamepad. */
	void SetUsingGamepad(const bool bNewUsingGamepad);

	// Delegate for IsUsingGamepad changes
	UPROPERTY(BlueprintAssignable)
	FILLPlayerInputUsingGamepad OnUsingGamepadChanged;

protected:
	// Have we used the gamepad recently? Reset for mouse and keyboard events
	bool bUsingGamepad;
};
