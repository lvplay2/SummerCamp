// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCHUDWidget.h"
#include "SCMatchOutcomeWidget.generated.h"

/**
 * @class USCMatchOutcomeWidget
 */
UCLASS()
class SUMMERCAMP_API USCMatchOutcomeWidget
: public USCHUDWidget
{
	GENERATED_UCLASS_BODY()

public:
	/** Play the initial spectator intro transition */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Spectator")
	void OnPlaySpectatorIntro();

	/** Play the waiting for outro transition */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Spectator")
	void OnPlayWaitingPreOutro();
};
