// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "GameFramework/GameState.h"
#include "ILLGameState.generated.h"

class AGameMode;
class APlayerState;

class AILLCharacter;
class AILLLobbyBeaconMemberState;
class AILLPlayerState;
class UILLGameStateLocalMessage;

/**
 * Delegate triggered when an ILLCharacter initializes.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FILLCharacterPostInitialized, AILLCharacter*, Character);

/**
 * Delegate triggered when an ILLCharacter destroys.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FILLCharacterDestroyed, AILLCharacter*, Character);

/**
 * @class AILLGameState
 */
UCLASS(Config=Game)
class ILLGAMEFRAMEWORK_API AILLGameState
: public AGameState
{
	GENERATED_UCLASS_BODY()

	// Begin AActor interface
	virtual void PostInitializeComponents() override;
	// End AActor interface

	// Begin AGameState interface
	virtual void HandleMatchIsWaitingToStart() override;
	virtual void HandleMatchHasStarted() override;
	virtual void OnRep_MatchState() override;
	// End AGameState interface

	/** @return Game mode class replicated to all clients. Replicated, so may not be available immediately after joining a match. */
	UFUNCTION(BlueprintPure, Category="GameState")
	TSubclassOf<AGameModeBase> GetGameModeClass() const { return GameModeClass; }

	// Defer broadcasting NotifyBeginPlay on the server until AILLGameMode::ReadyToStartMatch returns true
	// Clients will wait for the MatchState to be replicated as InProgress to broadcast it locally
	bool bDeferBeginPlay;

	//////////////////////////////////////////////////////////////////////////
	// Instance management
public:
	/** @return true if we are pending an instance recycle. */
	FORCEINLINE bool IsPendingInstanceRecycle() const { return bPendingInstanceRecycle; }

	/** Notification from AILLGameMode when CheckInstanceRecycle tells it that it needs to die. */
	virtual void HandlePendingInstanceRecycle();

protected:
	/** Replication handler for bPendingInstanceRecycle. */
	UFUNCTION()
	virtual void OnRep_bPendingInstanceRecycle();

	// Is this server pending a shutdown, which will send us back to matchmaking?
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_bPendingInstanceRecycle, Transient, Category="GameState")
	bool bPendingInstanceRecycle;
	
	//////////////////////////////////////////////////////////////////////////
	// Match states
public:
	/** @return true if we are waiting to start. Generally means waiting for players to load. */
	UFUNCTION(BlueprintPure, Category="GameState")
	virtual bool IsMatchWaitingToStart() const;

	/** @return true if we are waiting after a match. Generally at a match summary screen of some sort. */
	UFUNCTION(BlueprintPure, Category="GameState")
	virtual bool IsMatchWaitingPostMatch() const;

	/** @return true if players should be flagging themselves as using multiplayer features locally. */
	virtual bool IsUsingMultiplayerFeatures() const;

protected:
	// Message class to use for the game state
	UPROPERTY()
	TSubclassOf<UILLGameStateLocalMessage> GameStateMessageClass;

	//////////////////////////////////////////////////////////////////////////
	// Game session host
public:
	/** @return Current host player state. */
	virtual AILLPlayerState* GetCurrentHost() const;

	//////////////////////////////////////////////////////////////////////////
	// Character creation/destruction events
public:
	/** Triggers OnCharacterInitialized. */
	virtual void CharacterInitialized(AILLCharacter* Character);

	/** Triggers OnCharacterDestroyed. */
	virtual void CharacterDestroyed(AILLCharacter* Character);

protected:
	// Broadcast when an ILLCharacter finishes PostInitializeComponents
	UPROPERTY(BlueprintAssignable)
	FILLCharacterPostInitialized OnCharacterInitialized;

	// Broadcast when an ILLCharacter hits BeginDestroy
	UPROPERTY(BlueprintAssignable)
	FILLCharacterDestroyed OnCharacterDestroyed;
};
