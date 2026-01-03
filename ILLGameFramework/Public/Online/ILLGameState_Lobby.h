// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLGameState.h"
#include "ILLGameState_Lobby.generated.h"

/**
 * @struct FILLLobbySetting
 */
USTRUCT()
struct FILLLobbySetting
{
	GENERATED_USTRUCT_BODY()

	// Name for the setting
	UPROPERTY()
	FName Name;

	// Current value
	UPROPERTY()
	FString Value;
};

/**
 * @class AILLGameState_Lobby
 */
UCLASS()
class ILLGAMEFRAMEWORK_API AILLGameState_Lobby
: public AILLGameState
{
	GENERATED_UCLASS_BODY()

	// Begin AGameState interface
	virtual void HandleMatchHasEnded() override;
	// End AGameState interface

	// Begin AILLGameState interface
	virtual bool IsUsingMultiplayerFeatures() const { return false; }
	// End AILLGameState interface

	//////////////////////////////////////////////////
	// Lobby settings
public:
	/** Changes a lobby setting on the server. */
	void ChangeLobbySetting(const FName& Name, const FString& Value);

	/** @return Value for lobby setting Name. */
	FString GetLobbySetting(const FName& Name) const;

	/** Notifies local players that lobby settings have updated, to update the UI. */
	void LobbySettingsUpdated();

protected:
	/** LobbySettings replication handler. */
	UFUNCTION()
	void OnRep_LobbySettings();

private:
	// Replicated lobby settings
	UPROPERTY(ReplicatedUsing=OnRep_LobbySettings, Transient)
	TArray<FILLLobbySetting> LobbySettings;

	//////////////////////////////////////////////////
	// Auto-starting
public:
	/**
	 * Starts the travel countdown because this lobby got full.
	 *
	 * @param Countdown New TravelCountdown time.
	 * @param bAllowAutoTimerIncrease Accept Countdown even if it is larger than our current countdown.
	 */
	void AutoStartTravelCountdown(const int Countdown, const bool bAllowAutoTimerIncrease = false);

	/** Cancels a previously requested auto-start countdown. */
	void CancelAutoStartCountdown();

	/** Starts the travel countdown timer for this lobby. */
	void StartTravelCountdown(const int Countdown);

	/** @return TravelCountdown if active, otherwise -1. */
	FORCEINLINE int32 GetTravelCountdown() const { return bTravelCountingDown ? TravelCountdown : -1; }

	/** @return true if our travel countdown was an auto-start based on player requirements. */
	FORCEINLINE bool IsAutoTravelCountdownActive() const { return bAutoTravelCountdown; }

	/** @return true if our travel countdown timer is active, auto-travel or not. */
	FORCEINLINE bool IsTravelCountingDown() const { return bTravelCountingDown; }

	/** Cancels the travel countdown timer for this lobby. */
	void CancelTravelCountdown();

protected:
	/** TravelCountdown replication handler. */
	UFUNCTION()
	virtual void OnRep_TravelCountdown();

	/** Timer callback for TravelCountdown. */
	virtual void TravelCountdownTimer();

	// Replicated travel countdown timer
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_TravelCountdown, Transient)
	int32 TravelCountdown;

	// true when bTravelCountingDown is true because of an auto-start
	UPROPERTY(BlueprintReadOnly, Transient, ReplicatedUsing=OnRep_LobbySettings)
	bool bAutoTravelCountdown;

	// true when a TravelCountdown timer is active
	UPROPERTY(BlueprintReadOnly, Transient, ReplicatedUsing=OnRep_LobbySettings)
	bool bTravelCountingDown;

	// Timer handle for TravelCountdown
	FTimerHandle TimerHandle_TravelCountdown;

	//////////////////////////////////////////////////
	// Travel
protected:
	/** @return Additional travel options when we lobby travel. */
	virtual FString GetLevelLoadOptions() const;

	/** @return Map to load when we lobby travel. */
	virtual FString GetSelectedLobbyMap() const { return FString(); }

	/** @return Game mode to use when we lobby travel. */
	virtual FString GetSelectedGameMode() const { return FString(); }
};
