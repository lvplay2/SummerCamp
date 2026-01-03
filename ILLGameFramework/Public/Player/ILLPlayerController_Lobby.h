// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLPlayerController.h"
#include "ILLPlayerController_Lobby.generated.h"

/**
 * @class AILLPlayerController_Lobby
 */
UCLASS(Config=Game)
class ILLGAMEFRAMEWORK_API AILLPlayerController_Lobby
: public AILLPlayerController
{
	GENERATED_UCLASS_BODY()

	// End APlayerController interface
	virtual void ClientTeamMessage_Implementation(APlayerState* SenderPlayerState, const FString& S, FName Type, float MsgLifeTime) override;
	// End APlayerController interface

	/** Changes a lobby setting on the server. */
	UFUNCTION(Reliable, Server, WithValidation)
	void ServerChangeLobbySetting(const FName& Name, const FString& Value);

	/** Tells the server to travel to URL. */
	UFUNCTION(Reliable, Server, WithValidation)
	void ServerStartLobbyGame(const FString& URL);

	/** Requests to start the travel countdown timer. */
	UFUNCTION(Reliable, Server, WithValidation)
	void ServerStartTravelCountdown(const int32 Countdown);

	/** Requests to cancel a previously-started travel countdown timer. */
	UFUNCTION(Reliable, Server, WithValidation)
	void ServerCancelTravelCountdown();

	/** Client notification for travel countdown starts. */
	UFUNCTION(Client, Reliable)
	void ClientStartTravelCountdown(const int32 TravelCountdown);

	/** Client notification for travel countdown cancellation. */
	UFUNCTION(Client, Reliable)
	void ClientCancelledTravelCountdown();
};
