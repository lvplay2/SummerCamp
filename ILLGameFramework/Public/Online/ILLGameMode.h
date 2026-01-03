// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "GameFramework/GameMode.h"
#include "ILLGameMode.generated.h"

class AGameSession;
class AILLPlayerController;

/**
 * @class AILLGameMode
 */
UCLASS(Abstract)
class ILLGAMEFRAMEWORK_API AILLGameMode
: public AGameMode
{
	GENERATED_UCLASS_BODY()

	// Begin AActor interface
	virtual void Destroyed() override;
	virtual void Tick(float DeltaSeconds) override;
	// End AActor interface

	// Begin AGameModeBase interface
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual APlayerController* Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal, const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT("")) override;
	virtual bool SetPause(APlayerController* PC, FCanUnpause CanUnpauseDelegate = FCanUnpause()) override;
	virtual bool ClearPause() override;
	virtual void ForceClearUnpauseDelegates(AActor* PauseActor) override;
	// End AGameModeBase interface

	// Begin AGameMode interface
	virtual void OnMatchStateSet() override;
	virtual void StartPlay() override;
	virtual void RestartGame() override;
	virtual TSubclassOf<AGameSession> GetGameSessionClass() const override;
	virtual bool ReadyToStartMatch_Implementation() override;
	virtual void Logout(AController* Exiting) override;
	virtual void GetSeamlessTravelActorList(bool bToTransition, TArray<AActor*>& ActorList) override;
	virtual void PostSeamlessTravel() override;
	virtual void GenericPlayerInitialization(AController* C) override;
	virtual void HandleMatchIsWaitingToStart() override;
	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;
	virtual void HandleLeavingMap() override;
	virtual bool AllowCheats(APlayerController* P) override;
	virtual void AddInactivePlayer(APlayerState* PlayerState, APlayerController* PC) override;
	// End AGameMode interface

	/** Version of Broadcast which checks */
	virtual void ConditionalBroadcast(AActor* Sender, const FString& Msg, FName Type);

	/**
	 * @param bWaitForFullyTraveled Include players that have finished logging in but have not finished loading and/or synchronizing.
	 * @return Number of players pending connection with this server (connecting and/or loading the map).
	 */
	int32 GetNumPendingConnections(const bool bWaitForFullyTraveled = true) const;

	/** @return true if there are no pending players, everbody is loaded and acked in. */
	virtual bool HasEveryPlayerLoadedIn(const bool bCheckLobbyBeacon = true) const;

	/** Notification that a seamless travel has happened and is now finished */
	UFUNCTION(BlueprintImplementableEvent, Category="Game", meta=(DisplayName="PostSeamlessTravel"))
	void K2_PostSeamlessTravel();

	// Amount of time to hold a QuickPlay match as not IsOpenToMatchmaking after creation to allow Play Together party members in
	UPROPERTY(Config)
	float PlayTogetherHoldMatchmakingTime;

	//////////////////////////////////////////////////
	// Instance management
public:
	/** Callback from UILLOnlineMatchmakingClient when instance replacement fails. */
	virtual void RecycleReplacementFailed();

	/** Helper function that sets a timer to call RecycleExit in RecycleExitDelay seconds. */
	virtual void DelayedRecycleExit();

protected:
	/** Simply quits after a delay. */
	virtual void RecycleExit();

	// Delay before exiting after everyone was told to leave
	UPROPERTY(Config)
	float RecycleExitDelay;

	// Timer handle for quitting after recycling
	FTimerHandle TimerHandle_RecycleExit;

	// Amount of time an in-progress match can be empty before it automatically ends, to return to lobby etc or whatever
	UPROPERTY(Config)
	int32 TimeEmptyBeforeEndMatch;

	// Number of seconds and in-progress match has been empty
	int32 SecondsWithoutPlayers;
	
	//////////////////////////////////////////////////
	// Pause management
public:
	/** Attempts to pause the game if any players are wanting it to be. */
	virtual void AttemptFlushPendingPause();

	// People who desire pausing once the game can be paused
	TMap<AILLPlayerController*, FCanUnpause> PendingPausers;

	//////////////////////////////////////////////////
	// Game second timer
protected:
	/** Called once per game second. */
	virtual void GameSecondTimer();

	/** Forces GameSecondTimer to fire next tick. */
	FORCEINLINE void ForceGameSecondTimerNextTick() { TimerDeltaSeconds = 0.f; }

private:
	// GameSecondTimer delta seconds
	float TimerDeltaSeconds;

	//////////////////////////////////////////////////
	// Session management settings
public:
	/** @return true if our game session should be open to matchmaking. */
	virtual bool IsOpenToMatchmaking() const;

	/** Called every SessionUpdateDelay seconds. */
	virtual void UpdateGameSession();

	/** Resets SessionUpdateTimer so UpdateGameSession will be called soon. Useful for mitigating rate limits on the shitty consoles. */
	virtual void QueueGameSessionUpdate();

	// Friendly name for this game mode, reported to the OSS
	FString ModeName;

protected:
	// Time between session updates
	UPROPERTY(Config)
	float SessionUpdateDelay;

	// Time between session updates
	UPROPERTY(Config)
	float SessionInitialUpdateDelay;

	// Number of seconds until a session update is performed
	float SessionUpdateTimer;

	// Has the initial game session update been sent?
	bool bHasSentInitialSessionUpdate;

	//////////////////////////////////////////////////////////////////////////
	// Lobby member registration
protected:
	/** Called whenever a lobby beacon member state is created. */
	UFUNCTION()
	virtual void AddLobbyMemberState(AILLLobbyBeaconMemberState* MemberState);

	/** Called whenever a lobby beacon member state is destroyed. */
	UFUNCTION()
	virtual void RemoveLobbyMemberState(AILLLobbyBeaconMemberState* MemberState);
};
