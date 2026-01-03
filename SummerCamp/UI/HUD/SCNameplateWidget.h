// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCUserWidget.h"
#include "SCNameplateWidget.generated.h"

class ASCPlayerState;

/**
 * @class USCNameplateWidget
 */
UCLASS()
class SUMMERCAMP_API USCNameplateWidget
: public USCUserWidget
{
	GENERATED_UCLASS_BODY()

	/** @return Character this nameplate belongs to. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Nameplate")
	ASCCharacter* GetOwningCharacter();

	/** @return PlayerState this nameplate belongs to. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Nameplate")
	ASCPlayerState* GetOwningPlayerState();
};
