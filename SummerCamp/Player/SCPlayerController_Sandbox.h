// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCPlayerController_Offline.h"
#include "SCPlayerController_Sandbox.generated.h"

/**
 * @class ASCPlayerController_Sandbox
 */
UCLASS(Config=Game)
class SUMMERCAMP_API ASCPlayerController_Sandbox
: public ASCPlayerController_Offline
{
	GENERATED_UCLASS_BODY()

	// Begin APlayerController interface
	virtual void SetupInputComponent() override;
	// Begin APlayerController interface

protected:
	/** Requests to switch to the next character. */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_RequestNextCharacter();

	/** Requests to switch to the previous character. */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_RequestPreviousCharacter();

	/** Requests to switch to the first Jason. */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_RequestFirstJason();
};
