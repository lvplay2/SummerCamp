// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLUserWidget.h"
#include "ILLLobbyWidget.generated.h"

class AILLLobbyBeaconMemberState;
class AILLPlayerState;

/**
 * @class UILLLobbyWidget
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLLobbyWidget
: public UILLUserWidget
{
	GENERATED_UCLASS_BODY()

	/** Notification for receiving a lobby message from a player (or self). */
	UFUNCTION(BlueprintImplementableEvent, Category="Lobby")
	void LobbyPlayerMessage(AILLPlayerState* Sender, const FString& Message);

	/** Send a lobby message to all players in the lobby. */
	UFUNCTION(BlueprintCallable, Category="Lobby", meta=(DeprecatedFunction, DeprecationMessage="Use ILLPlayerController::Say"))
	void SendLobbyMessage(const FString& Message);

	/** @return Player state of the current lobby host. */
	UFUNCTION(BlueprintCallable, Category="Lobby")
	AILLPlayerState* GetLobbyHost() const;

	/** @return true if the owning player is the lobby host. */
	UFUNCTION(BlueprintCallable, Category="Lobby")
	bool IsLobbyHost() const;

	/** Attempts to change a lobby setting, if we are host. */
	UFUNCTION(BlueprintCallable, Category="Lobby")
	void ChangeLobbySetting(const FName& Name, const FString& Value);

	/** @return Value for a lobby setting Name. */
	UFUNCTION(BlueprintCallable, Category="Lobby")
	FString GetLobbySetting(const FName& Name) const;

	/** Notification for when lobby settings are replicated/updated. */
	UFUNCTION(BlueprintImplementableEvent, Category="Lobby")
	void LobbySettingsUpdated();

	/** Tells the server to travel to URL. */
	UFUNCTION(BlueprintCallable, Category="Lobby")
	void LobbyTravel(const FString& URL);

	/** Tells the server to start a travel countdown. */
	UFUNCTION(BlueprintCallable, Category="Lobby")
	void StartTravelCountdown(const int32 Countdown);

	/** Tells the server to cancel a previous travel countdown timer. */
	UFUNCTION(BlueprintCallable, Category="Lobby")
	void CancelTravelCountdown();

	/** Notification for when a previous travel countdown timer has started, called on ALL clients. */
	UFUNCTION(BlueprintImplementableEvent, Category="Lobby")
	void LobbyTravelCountdownStarted(const int32 TravelCountdown);

	/** Notification for when a previous travel countdown timer has been canceled, called on ALL clients. */
	UFUNCTION(BlueprintImplementableEvent, Category="Lobby")
	void LobbyTravelCountdownCancelled();
};
