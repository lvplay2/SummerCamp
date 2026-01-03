// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLLobbyWidget.h"
#include "SCPlayerState.h"
#include "SCLobbyWidget.generated.h"

/**
 * @class USCLobbyWidget
 */
UCLASS()
class SUMMERCAMP_API USCLobbyWidget
: public UILLLobbyWidget
{
	GENERATED_UCLASS_BODY()

	/** Sends the score event catagory totals to the Lobby Widget */
	UFUNCTION(BlueprintImplementableEvent, Category = "End Match")
	void OnShowEndMatchMenus(bool bShow);
};