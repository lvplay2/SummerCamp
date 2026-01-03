// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "GameFramework/PlayerState.h"

#include "ILLBackendPlayerIdentifier.h"
#include "ILLPlayerState.generated.h"

class AILLLobbyBeaconMemberState;
class AILLPartyBeaconMemberState;
class UILLPlayerStateLocalMessage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FILLPlayerStatePlayerSettingsUpdated, AILLPlayerState*, UpdatedPlayerState);

/**
 * @class AILLPlayerState
 */
UCLASS()
class ILLGAMEFRAMEWORK_API AILLPlayerState
: public APlayerState
{
	GENERATED_UCLASS_BODY()

	// Begin AActor interface
	virtual void Destroyed() override;
	virtual void OnRep_Owner() override;
	virtual void PostNetReceive() override;
	// End AActor interface

	// Begin APlayerState interface
	virtual void OnRep_PlayerName() override;
	virtual void OnRep_UniqueId() override;
	virtual void SetPlayerName(const FString& S) override;
	virtual void SetUniqueId(const FUniqueNetIdRepl& InUniqueId) override;
	virtual bool ShouldBroadCastWelcomeMessage(bool bExiting = false) override { return false; }
	virtual void UpdatePing(float InPing) override;
	virtual void OverrideWith(APlayerState* PlayerState) override;
	virtual void CopyProperties(APlayerState* PlayerState) override;
	virtual void SeamlessTravelTo(APlayerState* NewPlayerState) override;
	virtual void RegisterPlayerWithSession(bool bWasFromInvite) override;
	virtual void UnregisterPlayerWithSession() override;
	// End APlayerState interface

	/** @return Actual ping. The Ping property is divided by 4 before being compressed into byte-form, and really should not be BlueprintReadOnly! Clamped to 999. */
	UFUNCTION(BlueprintPure, Category="PlayerState")
	int32 GetRealPing() const;

	/** @return Actual ping in text display form. Clamped to "999ms" and will return an empty value until HasFullyTraveled returns true and the ping is non-zero. */
	UFUNCTION(BlueprintPure, Category="PlayerState")
	FText GetPingDisplayText() const;

protected:
	/** Called from OnRep_UniqueId and SetUniqueId. */
	virtual void OnReceivedUniqueId();

	// Localized message class for important events
	UPROPERTY()
	TSubclassOf<UILLPlayerStateLocalMessage> PlayerStateMessageClass;

	//////////////////////////////////////////////////////////////////////////
	// Backend
public:
	/** @return Backend account identifier for this player. */
	FORCEINLINE const FILLBackendPlayerIdentifier& GetAccountId() const { return AccountId; }

	/** Assigns the backend account identifier for this player. */
	virtual void SetAccountId(const FILLBackendPlayerIdentifier& InAccountId);

protected:
	/** Replication handler for AccountId. */
	UFUNCTION()
	virtual void OnRep_AccountId();

	// Backend account identifier
	UPROPERTY(Transient, ReplicatedUsing=OnRep_AccountId)
	FILLBackendPlayerIdentifier AccountId;
	
	//////////////////////////////////////////////////////////////////////////
	// Beacon associations
public:
	/** @return Lobby beacon member state for this player. */
	UFUNCTION(BlueprintPure, Category="Beacon")
	AILLLobbyBeaconMemberState* GetLobbyMemberState() const;

	/** @return Party beacon member state for this player. Only available when this is a local player or a local player is in the same party. */
	UFUNCTION(BlueprintPure, Category="Beacon")
	AILLPartyBeaconMemberState* GetPartyMemberState() const;

	/** Helper function to update LobbyMemberState and PartyMemberState. */
	virtual void SynchronizeBeaconActors()
	{
		GetLobbyMemberState();
		GetPartyMemberState();
	}

protected:
	// NOTE: Beacon actors live in their own custom UWorld, so do NOT replicate it! There should be no reason to!
	// Lobby beacon member actor
	UPROPERTY(BlueprintReadOnly, Transient, Category="Beacon")
	mutable AILLLobbyBeaconMemberState* LobbyMemberState;

	// Party beacon member actor
	UPROPERTY(BlueprintReadOnly, Transient, Category="Beacon")
	mutable AILLPartyBeaconMemberState* PartyMemberState;

	//////////////////////////////////////////////////////////////////////////
	// Registration
public:
	/**
	 * @param bCanUpdate Allow this call to update the cached bHasFullyTraveled status if not already true.
	 * @return true if this player has fully traveled and synchronized.
	 */
	UFUNCTION(BlueprintPure, Category="Travel")
	bool HasFullyTraveled(const bool bCanUpdate = false) const;

	// Has our PlayerController acknowledged this PlayerState as theirs?
	UPROPERTY(Transient, Replicated)
	bool bHasAckedPlayerState;

protected:
	/** @return true if this player is registered with SessionName. */
	bool HasRegisteredWithSession() const;

	/** Called from HasFullyTraveled when all checks complete. */
	virtual void PlayerStateFullyTraveled();

	// Has this player state fully traveled into the current match?
	mutable bool bHasFullyTraveled;

	// Has this player state instance registered with the NAME_GameSession?
	bool bHasRegisteredWithGameSession;

	//////////////////////////////////////////////////////////////////////////
	// Game session host
public:
	/** @return true if this player is allowed to be host. */
	virtual bool CanBeHost() const;

	/** @return true if this player is the host. */
	UFUNCTION(BlueprintCallable, Category="Lobby")
	virtual bool IsHost() const { return bIsHost; }

	/** Promotes this player to host. */
	virtual void PromoteToHost();

	/** Demotes this player from host. */
	virtual void DemoteFromHost();

private:
	// true if this player is the lobby host
	UPROPERTY(Transient, ReplicatedUsing=OnRep_bIsHost)
	uint8 bIsHost : 1;

	/** Replication handler for bIsHost. */
	UFUNCTION()
	void OnRep_bIsHost();

	//////////////////////////////////////////////////////////////////////////
	// Lobby settings management
public:
	UPROPERTY()
	FILLPlayerStatePlayerSettingsUpdated OnPlayerSettingsUpdated;

protected:
	/** Helper for notifying local players that a player state has updated. */
	void PlayerSettingsUpdated();

	//////////////////////////////////////////////////////////////////////////
	// Voice
public:
	/** @return true if this player is talking. */
	UFUNCTION(BlueprintPure, Category="Voice")
	virtual bool IsPlayerTalking() const;

	/** @return true if this player has ever talked. */
	UFUNCTION(BlueprintPure, Category="Voice")
	virtual bool HasPlayerTalked() const;

protected:
	/** Helper function to replicate the mute state for this player on any local players. */
	void ReplicateMuteStates() const;
};
