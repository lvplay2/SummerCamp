// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "GameFramework/OnlineReplStructs.h"

#include "ILLBackendPlayerIdentifier.h"
#include "ILLSessionBeaconMemberState.generated.h"

class AILLSessionBeaconClient;
class AILLSessionBeaconHost;
class AILLSessionBeaconHostObject;
class UILLLocalPlayer;

/**
 * @struct FILLSessionBeaconPlayer
 */
USTRUCT()
struct FILLSessionBeaconPlayer
{
	GENERATED_USTRUCT_BODY()

	// Backend account identifier for this player
	UPROPERTY()
	FILLBackendPlayerIdentifier AccountId;

	// Unique identifier for this player
	UPROPERTY()
	FUniqueNetIdRepl UniqueId;

	// Display name for this player
	UPROPERTY()
	FString DisplayName;

	FILLSessionBeaconPlayer()
	{
	}

	FILLSessionBeaconPlayer(const FILLBackendPlayerIdentifier& InAccountId, const FUniqueNetIdRepl& InUniqueId, const FString& InDisplayName)
	: AccountId(InAccountId), UniqueId(InUniqueId), DisplayName(InDisplayName)
	{
	}

	FILLSessionBeaconPlayer(UILLLocalPlayer* LocalPlayer);

	/** @return true if this matches Other. */
	FORCEINLINE bool operator ==(const FILLSessionBeaconPlayer& Other) const
	{
		return (AccountId == Other.AccountId);
	}

	/** @return true if this does not match Other. */
	FORCEINLINE bool operator !=(const FILLSessionBeaconPlayer& Other) const
	{
		return (AccountId != Other.AccountId);
	}

	/** @return true if this player is valid. */
	bool IsValidPlayer() const;
};

/**
 * @struct FILLSessionBeaconReservation
 * @brief Represents an incoming reservation with a session beacon.
 */
USTRUCT()
struct FILLSessionBeaconReservation
{
	GENERATED_USTRUCT_BODY()

	// Leader backend account identifier
	UPROPERTY()
	FILLBackendPlayerIdentifier LeaderAccountId;

	// Players of this reservation, possibly including the leader
	UPROPERTY()
	TArray<FILLSessionBeaconPlayer> Players;

	FILLSessionBeaconReservation() {}

	/** @return true if this matches Other. */
	FORCEINLINE bool operator ==(const FILLSessionBeaconReservation& Other) const
	{
		if (LeaderAccountId != Other.LeaderAccountId)
			return false;

		return (Players == Other.Players);
	}

	/** @return true if this does not match Other. */
	FORCEINLINE bool operator !=(const FILLSessionBeaconReservation& Other) const
	{
		return !operator ==(Other);
	}

	/** @return true if this reservation is valid. */
	bool IsValidReservation() const;

	/** @return Debug string representation for this reservation, for logging generally. */
	FString ToDebugString() const;
};

/**
 * @class AILLSessionBeaconMemberState
 */
UCLASS(Config=Game, NotPlaceable, Transient)
class ILLGAMEFRAMEWORK_API AILLSessionBeaconMemberState
: public AInfo
{
	GENERATED_UCLASS_BODY()

	// Begin AActor interface
	virtual void BeginPlay() override;
	virtual void PostNetInit() override;
	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;
	virtual void OnRep_Owner() override;
	// End AActor interface

	/** @return Backend account identifier for this member. */
	FORCEINLINE const FILLBackendPlayerIdentifier& GetAccountId() const { return AccountId; }

	/** @return Associated beacon client actor if available. */
	FORCEINLINE AILLSessionBeaconClient* GetBeaconClientActor() const { return BeaconClientActor; }

	/** @return Display name for this member. */
	FORCEINLINE const FString& GetDisplayName() const { return DisplayName; }

	/** @return Backend account identifier for this member's party leader. */
	FORCEINLINE const FILLBackendPlayerIdentifier& GetLeaderAccountId() const { return LeaderAccountId; }

	/** @return Platform account identifier for this member. */
	FORCEINLINE FUniqueNetIdRepl GetMemberUniqueId() const { return UniqueId; }

	/** @return true if all required replicated information has been received for this member. */
	virtual bool HasFullySynchronized() const;

	/** @return true if this member has had their reservation accepted. */
	FORCEINLINE bool HasReservationBeenAccepted() const { return bReservationAccepted; }

	/** @return Debug string for this member. */
	virtual FString ToDebugString() const;
	
	//////////////////////////////////////////////////////////////////////////
	// State & registration
public:
	/** [Authority] Called from AILLSessionBeaconHostObject::InitHostBeacon for a seed reservation. */
	virtual void InitializeArbiterLeader()
	{
		SetReservationAccepted(true);
		bArbitrationMember = true;
	}

	/** [Authority] Called from AILLSessionBeaconHostObject::InitHostBeacon for a seed reservation. */
	virtual void InitializeArbiterFollower(const FILLBackendPlayerIdentifier& InLeaderAccountId)
	{
		SetLeaderAccountId(InLeaderAccountId);
	}

	/** [Authority] Notification for when this member is accepted server-side. */
	virtual void NotifyAccepted()
	{
		SetReservationAccepted(true);
	}

	/** [Authority] Called when we receive a BeaconClient for an already-existing member state. */
	virtual void ReceiveClient(AILLSessionBeaconClient* BeaconClient);

	/** [Authority] Called to assign our leader and BeaconClient. */
	virtual void ReceiveLeadClient(const FILLBackendPlayerIdentifier& InLeaderAccountId, AILLSessionBeaconClient* BeaconClient)
	{
		SetLeaderAccountId(InLeaderAccountId);
		ReceiveClient(BeaconClient);
	}
	virtual void ReceiveLeadClient(const FILLBackendPlayerIdentifier& InLeaderAccountId)
	{
		SetLeaderAccountId(InLeaderAccountId);
	}

	/** [Authority] Used by the host to assign our DisplayName. */
	virtual void SetDisplayName(const FString& InDisplayName)
	{
		if (DisplayName != InDisplayName)
		{
			DisplayName = InDisplayName;
			OnRep_DisplayName();
			ForceNetUpdate();
		}
	}

	/** [Authority] Used by the host to assign the party leader's backend account identifier. */
	virtual void SetLeaderAccountId(const FILLBackendPlayerIdentifier& InLeaderAccountId)
	{
		if (LeaderAccountId != InLeaderAccountId)
		{
			LeaderAccountId = InLeaderAccountId;
			OnRep_LeaderAccountId();
			ForceNetUpdate();
		}
	}

	/** [Authority] Helper function for changing AccountId and simulating OnRep_AccountId. */
	virtual void SetAccountId(const FILLBackendPlayerIdentifier& NewAccountId)
	{
		if (AccountId != NewAccountId)
		{
			AccountId = NewAccountId;
			OnRep_AccountId();
			ForceNetUpdate();
		}
	}

	/** [Authority] Used by the host to assign our UniqueId. */
	virtual void SetUniqueId(const FUniqueNetIdRepl& InUniqueId)
	{
		UniqueId = InUniqueId;
		OnRep_UniqueId();
		ForceNetUpdate();
	}

	/** Check HasFullySynchronized then flag this member as added. */
	virtual void CheckHasSynchronized();

	// Duration they have been timing out when pending
	float TimeoutDuration;

	// Does this member serve as arbiter/owner of this session beacon?
	UPROPERTY(Replicated, Transient)
	bool bArbitrationMember;

	// Has our addition been broadcasted yet?
	bool bHasSynchronized;

protected:
	/** Helper function for changing bReservationAccepted and simulating OnRep_ReservationAccepted. */
	void SetReservationAccepted(const bool bNewState)
	{
		if (bReservationAccepted != bNewState)
		{
			bReservationAccepted = bNewState;
			OnRep_ReservationAccepted();
			ForceNetUpdate();
		}
	}

	/** Replication handler for DisplayName. */
	UFUNCTION()
	virtual void OnRep_DisplayName();

	/** Replication handler for LeaderAccountId. */
	UFUNCTION()
	virtual void OnRep_LeaderAccountId();

	/** Replication handler for bReservationAccepted. */
	UFUNCTION()
	virtual void OnRep_ReservationAccepted();

	/** Replication handler for AccountId. */
	UFUNCTION()
	virtual void OnRep_AccountId();

	/** Replication handler for UniqueId. */
	UFUNCTION()
	virtual void OnRep_UniqueId();

	// Beacon client related to this member
	UPROPERTY(Replicated, Transient)
	AILLSessionBeaconClient* BeaconClientActor;

	// AccountId for this member
	UPROPERTY(ReplicatedUsing=OnRep_AccountId, Transient)
	FILLBackendPlayerIdentifier AccountId;

	// Leader backend account identifier
	UPROPERTY(ReplicatedUsing=OnRep_LeaderAccountId, Transient)
	FILLBackendPlayerIdentifier LeaderAccountId;

	// Platform unique identifier
	UPROPERTY(ReplicatedUsing=OnRep_UniqueId, Transient)
	FUniqueNetIdRepl UniqueId;

	// Display name for this member
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_DisplayName, Transient)
	FString DisplayName;

	// Has this user's reservation been accepted?
	UPROPERTY(ReplicatedUsing=OnRep_ReservationAccepted, Transient)
	bool bReservationAccepted;

	//////////////////////////////////////////////////////////////////////////
	// Voice
public:
	/** @return true if this player is talking. */
	UFUNCTION(BlueprintPure, Category="Voice")
	virtual bool IsPlayerTalking() const;

	// FPlatformTime::Seconds when this member was last speaking
	double LastTalkTime;

	// FPlatformTime::Seconds when this member started speaking
	double StartTalkTime;

	// Has this player ever talked before?
	UPROPERTY(BlueprintReadOnly, Category="Voice", Transient)
	bool bHasTalked;
};
