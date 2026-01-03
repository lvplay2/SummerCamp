// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLBackendPlayerIdentifier.h"
#include "ILLUserWidget.h"
#include "ILLPlayerListItemWidget.generated.h"

class AILLLobbyBeaconMemberState;
class AILLPartyBeaconMemberState;
class AILLPlayerState;

/**
 * @class UILLPlayerListItemWidget
 * @brief Smart player-centric widget that can take a PlayerState, LobbyMemberState or PartyMemberState and track down the rest on it's own.
 *		  Provides useful lookup Blueprint bindings to reduce a lot of repeated work when bridging these 3 actors in the UI.
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLPlayerListItemWidget
: public UILLUserWidget
{
	GENERATED_UCLASS_BODY()

	/** Assigns a new LobbyMemberState for this widget, invalidating other identifiers before attempting to synchronize them back. */
	UFUNCTION(BlueprintCallable, Category="ILLPlayerListItemWidget")
	virtual void AssignLobbyMemberState(AILLLobbyBeaconMemberState* InLobbyMemberState)
	{
		if (SetLobbyMemberState(InLobbyMemberState))
		{
			SetPartyMemberState(nullptr);
			SetPlayerState(nullptr);
			AttemptToSynchronize();
		}
	}

	/** Assigns a new PartyMemberState for this widget, invalidating other identifiers before attempting to synchronize them back. */
	UFUNCTION(BlueprintCallable, Category="ILLPlayerListItemWidget")
	virtual void AssignPartyMemberState(AILLPartyBeaconMemberState* InPartyMemberState)
	{
		if (SetPartyMemberState(InPartyMemberState))
		{
			SetLobbyMemberState(nullptr);
			SetPlayerState(nullptr);
			AttemptToSynchronize();
		}
	}

	/** Assigns a new PlayerState for this widget, invalidating other identifiers before attempting to synchronize them back. */
	UFUNCTION(BlueprintCallable, Category="ILLPlayerListItemWidget")
	virtual void AssignPlayerState(AILLPlayerState* InPlayerState)
	{
		if (SetPlayerState(InPlayerState))
		{
			SetLobbyMemberState(nullptr);
			SetPartyMemberState(nullptr);
			AttemptToSynchronize();
		}
	}

	/**
	 * Helper function that attempts to synchronize based on any of the known member or player states below.
	 * Scours the owning player's game world, and grabs the beacon world to look them up.
	 */
	virtual void AttemptToSynchronize();

	/**
	 * Flushes all player bindings immediately.
	 */
	virtual void ClearPlayerBindings()
	{
		SetPlayerState(nullptr);
		SetLobbyMemberState(nullptr);
		SetPartyMemberState(nullptr);
	}

	/**
	 * Swaps the player actors in this widget with OtherWidget if the UniqueIds do not match.
	 * @return true if there was an exchange.
	 */
	virtual bool ExchangeWith(UILLPlayerListItemWidget* OtherWidget);

	// Lobby member state associated with this player widget
	FORCEINLINE AILLLobbyBeaconMemberState* GetLobbyMemberState() const { return LobbyMemberState; }

	// Party member state associated with this player widget
	FORCEINLINE AILLPartyBeaconMemberState* GetPartyMemberState() const { return PartyMemberState; }

	// PlayerState associated with this player widget
	FORCEINLINE AILLPlayerState* GetPlayerState() const { return PlayerState; }

	/** @return Display name for this player. */
	UFUNCTION(BlueprintPure, Category="ILLPlayerListItemWidget|Player")
	FString GetDisplayName() const;

	/** @return Actual ping. The property Ping is divided by 4 before being compressed into byte-form, and really should not be BlueprintReadOnly! */
	UFUNCTION(BlueprintPure, Category="ILLPlayerListItemWidget|Player")
	int32 GetRealPing() const;

	/** @return Actual ping in text display form. Clamped to "999ms" and will return an empty value until HasFullyTraveled returns true and the ping is non-zero. */
	UFUNCTION(BlueprintPure, Category="ILLPlayerListItemWidget|Player")
	FText GetPingDisplayText() const;

	/** @return true if this player is a local one. */
	UFUNCTION(BlueprintPure, Category="ILLPlayerListItemWidget|Player")
	bool IsLocalPlayer() const;

	/** Backend account identifier found in our LobbyMemberState, PartyMemberState or PlayerState. */
	const FILLBackendPlayerIdentifier& GetPlayerAccountId() const;

	/** Platform unique identifier found in our LobbyMemberState, PartyMemberState or PlayerState. */
	FUniqueNetIdRepl GetPlayerUniqueId() const;

	/** @return true if this player has fully traveled and synchronized. */
	UFUNCTION(BlueprintPure, Category="ILLPlayerListItemWidget|Travel")
	bool HasFullyTraveled() const;

	/** @return true if this player is talking. */
	UFUNCTION(BlueprintPure, Category="ILLPlayerListItemWidget|VOIP")
	bool IsTalking() const;

	/** @return true if this player is talking. */
	UFUNCTION(BlueprintPure, Category="ILLPlayerListItemWidget|VOIP")
	bool HasTalked() const;

protected:
	FORCEINLINE bool SetItemStates(AILLLobbyBeaconMemberState* InLobbyMemberState, AILLPartyBeaconMemberState* InPartyMemberState, AILLPlayerState* InPlayerState)
	{
		const bool LobbyChanged = LobbyMemberState != InLobbyMemberState;
		LobbyMemberState = InLobbyMemberState;
		const bool PartyChanged = PartyMemberState != InPartyMemberState;
		PartyMemberState = InPartyMemberState;
		const bool PlayerStateChanged = PlayerState != InPlayerState;
		PlayerState = InPlayerState;

		if (LobbyChanged)
			OnLobbyMemberStateChanged();
		if (LobbyChanged)
			OnPartyMemberStateChanged();
		if (LobbyChanged)
			OnPlayerStateChanged();

		return LobbyChanged || PartyChanged || PlayerStateChanged;
	}
	FORCEINLINE bool SetLobbyMemberState(AILLLobbyBeaconMemberState* InLobbyMemberState)
	{
		if (LobbyMemberState != InLobbyMemberState)
		{
			LobbyMemberState = InLobbyMemberState;
			OnLobbyMemberStateChanged();
			return true;
		}

		return false;
	}
	FORCEINLINE bool SetPartyMemberState(AILLPartyBeaconMemberState* InPartyMemberState)
	{
		if (PartyMemberState != InPartyMemberState)
		{
			PartyMemberState = InPartyMemberState;
			OnPartyMemberStateChanged();
			return true;
		}

		return false;
	}
	FORCEINLINE bool SetPlayerState(AILLPlayerState* InPlayerState)
	{
		if (PlayerState != InPlayerState)
		{
			PlayerState = InPlayerState;
			OnPlayerStateChanged();
			return true;
		}

		return false;
	}

	/** Notification for when LobbyMemberState changes. */
	UFUNCTION(BlueprintImplementableEvent, Category="ILLPlayerListItemWidget")
	void OnLobbyMemberStateChanged();

	/** Notification for when PartyMemberState changes. */
	UFUNCTION(BlueprintImplementableEvent, Category="ILLPlayerListItemWidget")
	void OnPartyMemberStateChanged();

	/** Notification for when PlayerState changes. */
	UFUNCTION(BlueprintImplementableEvent, Category="ILLPlayerListItemWidget")
	void OnPlayerStateChanged();

	// Lobby member state associated with this player widget
	UPROPERTY(BlueprintReadOnly, Transient, Category="ILLPlayerListItemWidget")
	AILLLobbyBeaconMemberState* LobbyMemberState;

	// Party member state associated with this player widget
	UPROPERTY(BlueprintReadOnly, Transient, Category="ILLPlayerListItemWidget")
	AILLPartyBeaconMemberState* PartyMemberState;

	// PlayerState associated with this player widget
	UPROPERTY(BlueprintReadOnly, Transient, Category="ILLPlayerListItemWidget")
	AILLPlayerState* PlayerState;
};
