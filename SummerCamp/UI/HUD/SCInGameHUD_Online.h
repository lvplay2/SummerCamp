// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCInGameHUD.h"
#include "SCInGameHUD_Online.generated.h"

class USCWaitingForPlayersWidget;

/**
 * @class ASCInGameHUD_Online
 */
UCLASS(Abstract)
class SUMMERCAMP_API ASCInGameHUD_Online
: public ASCInGameHUD
{
	GENERATED_UCLASS_BODY()

	// Begin AActor interface
	virtual void PostInitializeComponents() override;
	virtual void Destroyed() override;
	// End AActor interface

	// Begin ASCInGameHUD interface
	virtual void OnLevelIntroSequenceStarted() override;
	virtual void OnSpawnIntroSequenceStarted() override;
	virtual void OnSpawnIntroSequenceCompleted() override;
	virtual void UpdateCharacterHUD(const bool bForceSpectatorHUD = false) override;
	// End ASCInGameHUD interface

	//////////////////////////////////////////////////
	// Match status screens
protected:
	/** Helper function to remove the WaitingForPlayersScreenInstance if it exists. */
	virtual void RemoveWaitingForPlayers();

	// Class of the waiting for players screen
	UPROPERTY()
	TSubclassOf<USCWaitingForPlayersWidget> WaitingForPlayersScreenClass;

	// Instance of WaitingForPlayersScreenClass
	UPROPERTY(Transient)
	USCWaitingForPlayersWidget* WaitingForPlayersScreenInstance;
};
