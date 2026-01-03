// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLGame_Menu.h"

#include "SCMapRegistry.h"
#include "SCMenuWidget.h"
#include "SCGame_Menu.generated.h"

/**
 * @class ASCGame_Menu
 */
UCLASS(Abstract)
class ASCGame_Menu
: public AILLGame_Menu
{
	GENERATED_UCLASS_BODY()

	// Begin AGameMode interface
	virtual void RestartPlayer(class AController* NewPlayer) override;
	virtual TSubclassOf<AGameSession> GetGameSessionClass() const override;
	// End AGameMode interface

	// Pending offline map
	UPROPERTY(BlueprintReadWrite, Category="Offline")
	TSubclassOf<USCMapDefinition> PendingOfflineMap;

	// Pending offline game mode
	UPROPERTY(BlueprintReadWrite, Category="Offline")
	TSubclassOf<USCModeDefinition> PendingOfflineMode;

	// Pending offline settings menu
	UPROPERTY(BlueprintReadWrite, Category="Offline")
	TSubclassOf<USCMenuWidget> PendingOfflineSettingsWidget;
};
