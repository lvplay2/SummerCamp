// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLHUD.h"
#include "ILLHUD_Lobby.generated.h"

class AILLPlayerState;
class UILLLobbyWidget;

/**
 * @class AILLHUD_Lobby
 */
UCLASS()
class ILLGAMEFRAMEWORK_API AILLHUD_Lobby
: public AILLHUD
{
	GENERATED_UCLASS_BODY()

	// Begin UObject interface
	virtual void Destroyed() override;
	// End UObject interface

	// Begin AActor interface
	virtual void BeginPlay() override;
	// End AActor interface

	/** Forcibly hides the Lobby UI. */
	virtual void HideLobbyUI();

	/** Notification for when lobby settings are replicated. */
	void LobbySettingsUpdated();

	/** Notification for a player message. */
	void PlayerMessage(AILLPlayerState* Sender, const FString& Message);

	/** Notification for travel countdown is started. */
	void TravelCountdownStarted(const int32 TravelCountdown);

	/** Notification for travel countdown is canceled. */
	void TravelCountdownCancelled();

protected:
	/** @return Existing or newly spawned ILLLobbyWidget. */
	virtual void SpawnLobbyWidget() {}

	/** @return Spawned ILLLobbyWidget. */
	virtual UILLLobbyWidget* GetLobbyWidget() const { return nullptr; }
};
