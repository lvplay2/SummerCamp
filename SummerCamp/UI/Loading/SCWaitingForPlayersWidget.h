// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCMenuWidget.h"
#include "SCWaitingForPlayersWidget.generated.h"

/**
 * @class USCWaitingForPlayersWidget
 */
UCLASS()
class SUMMERCAMP_API USCWaitingForPlayersWidget
: public USCMenuWidget
{
	GENERATED_UCLASS_BODY()

	/** Event fired when this menu is first pushed, for animation purposes. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category="WaitingForPlayers", meta=(BlueprintProtected))
	void OnFirstDisplayed();
};
