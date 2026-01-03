// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLSessionBeaconMemberState.h"
#include "ILLSessionBeaconState.generated.h"

class AILLSessionBeaconClient;
class AILLSessionBeaconState;
struct FILLBackendPlayerIdentifier;
struct FILLSessionBeaconMemberStateInfoArray;

/**
 * Delegate fired when the member state in the session beacon has changed (add/remove/etc)
 *
 * @param MemberState Member of interest (post-addition / pre-removal).
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FILLOnBeaconMemberStateChanged, AILLSessionBeaconMemberState* /*MemberState*/);

/**
 * @struct FILLSessionBeaconMemberStateActorInfo
 * @brief Replication structure for a single beacon member state.
 */
USTRUCT()
struct FILLSessionBeaconMemberStateActorInfo
: public FFastArraySerializerItem
{
	GENERATED_USTRUCT_BODY()

public:
	FILLSessionBeaconMemberStateActorInfo()
	: BeaconMemberState(nullptr) {}

	FILLSessionBeaconMemberStateActorInfo(AILLSessionBeaconMemberState* InMemberState)
	: BeaconMemberState(InMemberState) {}

	// Actual member state actor
	UPROPERTY()
	AILLSessionBeaconMemberState* BeaconMemberState;

	/** Member state removal */
	void PreReplicatedRemove(const FILLSessionBeaconMemberStateInfoArray& InArraySerializer);

	/**
	 * Member state additions 
	 * This may only be a notification that the array is growing and the actor data 
	 * isn't available yet, in which case the actual element will be null
	 */
	void PostReplicatedAdd(const FILLSessionBeaconMemberStateInfoArray& InArraySerializer);

	/**
	 * Member state element has changed
	 * In this specific case only happens when the actor pointer is set.
	 * This should only occur if the member state actor was already 
	 * serialized to the client at the time of the PostReplicatedAdd call
	 */
	void PostReplicatedChange(const FILLSessionBeaconMemberStateInfoArray& InArraySerializer);
};

/**
 * @struct FILLSessionBeaconMemberStateInfoArray
 * @brief Struct for fast TArray replication of session beacon member state.
 */
USTRUCT()
struct FILLSessionBeaconMemberStateInfoArray
: public FFastArraySerializer
{
	GENERATED_USTRUCT_BODY()

	/** Implement support for fast TArray replication */
	bool NetDeltaSerialize(FNetDeltaSerializeInfo & DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FILLSessionBeaconMemberStateActorInfo, FILLSessionBeaconMemberStateInfoArray>(Members, DeltaParms, *this);
	}
	
	/**
	 * Adds a previously spawned member state to the Members list.
	 *
	 * @param MemberState Member to add.
	 * @return Member after being added successfully.
	 */
	AILLSessionBeaconMemberState* AddMember(AILLSessionBeaconMemberState* MemberState);

	/** @return Arbitration member. */
	AILLSessionBeaconMemberState* GetArbitrationMember() const;

	/**
	 * Get an existing member.
	 *
	 * @param AccountId Backend account identifier of the member state.
	 * @return Member state representation of an existing member.
	 */
	AILLSessionBeaconMemberState* GetMember(const FILLBackendPlayerIdentifier& AccountId) const;

	/**
	 * Get an existing member.
	 *
	 * @param UniqueId Platform identifier for the member of interest.
	 * @return Member state representation of an existing member.
	 */
	AILLSessionBeaconMemberState* GetMember(const FUniqueNetIdRepl& UniqueId) const;

	/**
	 * Get an existing member.
	 *
	 * @param ClientActor beacon actor for the member of interest.
	 * @return Member state representation of an existing member.
	 */
	AILLSessionBeaconMemberState* GetMember(const AILLSessionBeaconClient* ClientActor) const;

	/**
	 * Remove an existing member state from the beacon state.
	 *
	 * @param ClientActor Client to remove.
	 * @param UniqueId Unique identifier of the member state to remove.
	 */
	void RemoveMember(AILLSessionBeaconClient* ClientActor);
	void RemoveMember(const FUniqueNetIdRepl& UniqueId);

	/**
	 * Remove an existing member at Index.
	 *
	 * @param Index Member index to remove.
	 */
	void RemoveMemberAtIndex(const int32 Index);

	/**
	 * Get all members in the beacon state.
	 *
	 * @return all member representations in the beacon.
	 */
	FORCEINLINE TArray<FILLSessionBeaconMemberStateActorInfo>& GetAllMembers() { return Members; }
	FORCEINLINE const TArray<FILLSessionBeaconMemberStateActorInfo>& GetAllMembers() const { return Members; }

	/** @return number of members currently in the lobby */
	FORCEINLINE int32 GetNumMembers() const { return Members.Num(); }

	/** Dumps state information to log. */
	void DumpState();

private:
	// All of the members in the lobby
	UPROPERTY()
	TArray<FILLSessionBeaconMemberStateActorInfo> Members;

	// Owning lobby beacon for this array of members
	UPROPERTY(NotReplicated)
	AILLSessionBeaconState* ParentState;

	friend AILLSessionBeaconState;
	friend FILLSessionBeaconMemberStateActorInfo;
};

template<>
struct TStructOpsTypeTraits<FILLSessionBeaconMemberStateInfoArray>
: public TStructOpsTypeTraitsBase2<FILLSessionBeaconMemberStateInfoArray>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

/**
 * @class AILLSessionBeaconState
 */
UCLASS(Config=Game, NotPlaceable, Transient)
class ILLGAMEFRAMEWORK_API AILLSessionBeaconState
: public AInfo
{
	GENERATED_UCLASS_BODY()

	// Begin AActor interface
	virtual void PostInitializeComponents() override;
	virtual void Destroyed() override;
	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;
	// End AActor interface

	/**
	 * [Authority] Spawns a new member state for UniqueId.
	 *
	 * @param AccountId Backend account identifier for the member.
	 * @param UniqueId Unique identifier for the member.
	 * @param DisplayName Display name for the member.
	 * @return Beacon member state instance.
	 */
	AILLSessionBeaconMemberState* SpawnMember(const FILLBackendPlayerIdentifier& AccountId, const FUniqueNetIdRepl& UniqueId, const FString& DisplayName);
	
	/**
	 * [Authority] Adds a previously spawned member to the Members array.
	 *
	 * @param MemberState Member to add.
	 * @return Added member pointer if successful.
	 */
	AILLSessionBeaconMemberState* AddMember(AILLSessionBeaconMemberState* MemberState);

	/**
	 * Finds a member state by AccountId.
	 *
	 * @param AccountId Backend account identifier of the member state.
	 * @return Found member state for AccountId.
	 */
	AILLSessionBeaconMemberState* FindMember(const FILLBackendPlayerIdentifier& AccountId) const;

	/**
	 * Finds a member state by UniqueId.
	 *
	 * @param UniqueId Unique identifier of the member state.
	 * @return Found member state for UniqueId.
	 */
	AILLSessionBeaconMemberState* FindMember(const FUniqueNetIdRepl& UniqueId) const;

	/**
	 * [Authority] Find a member state by ClientActor.
	 *
	 * @param ClientActor Actor to look for.
	 * @return Found member state for ClientActor. NULL on clients.
	 */
	AILLSessionBeaconMemberState* FindMember(const AILLSessionBeaconClient* ClientActor) const;

	/** @return Arbitration member, if any. Dedicated servers will not have one. */
	AILLSessionBeaconMemberState* GetArbitrationMember() const;

	/** @return Member state at Index. */
	AILLSessionBeaconMemberState* GetMemberAtIndex(const int32 Index) const;

	/**
	 * [Authority] Remove an existing member state from the session beacon.
	 *
	 * @param ClientActor Client to remove.
	 * @param UniqueId Unique identifier of the member state to remove.
	 */
	void RemoveMember(AILLSessionBeaconClient* ClientActor);
	void RemoveMember(const FUniqueNetIdRepl& UniqueId);

	/**
	 * [Authority] Remove an existing member at Index.
	 */
	void RemoveMemberAtIndex(const int32 Index);

	/** @return Number of members currently in the lobby. */
	FORCEINLINE int32 GetNumMembers() const { return Members.GetNumMembers(); }

	/** @return Number of fully accepted members. */
	int32 GetNumAcceptedMembers() const;

	/** @return true if any of our members are not fully accepted yet. */
	bool HasAnyPendingMembers() const;

	/** @return Beacon member delegates. */
	FORCEINLINE FILLOnBeaconMemberStateChanged& OnBeaconMemberAdded() { return BeaconMemberAdded; }
	FORCEINLINE FILLOnBeaconMemberStateChanged& OnBeaconMemberRemoved() { return BeaconMemberRemoved; }
	FORCEINLINE FSimpleMulticastDelegate& OnBeaconMembersReceived() { return BeaconMembersReceived; }

	/** Dumps state information to log. */
	virtual void DumpState();

	/** Helper function for updating bHasReceivedMembers. */
	virtual bool CheckHasReceivedMembers();

protected:
	/** Replication handler for Members. */
	UFUNCTION()
	void OnRep_Members();

	/** Called when we first receive all beacon members. */
	virtual void BroadcastBeaconMembersReceived();

	// Member state class
	UPROPERTY()
	TSubclassOf<AILLSessionBeaconMemberState> MemberStateClass;

	// Array of members currently in the session beacon
	UPROPERTY(ReplicatedUsing=OnRep_Members)
	FILLSessionBeaconMemberStateInfoArray Members;

	// Beacon member delegates
	FILLOnBeaconMemberStateChanged BeaconMemberAdded;
	FILLOnBeaconMemberStateChanged BeaconMemberRemoved;
	FSimpleMulticastDelegate BeaconMembersReceived;

	// Have we received a member list update? Only set on clients
	bool bHasReceivedMembers;

	friend FILLSessionBeaconMemberStateInfoArray;

	//////////////////////////////////////////////////////////////////////////
	// Voice
protected:
	/** Callback for voice state changes for players. */
	virtual void OnPlayerTalkingStateChanged(TSharedRef<const FUniqueNetId> TalkerId, bool bIsTalking);

	// Delegate handle for OnPlayerTalkingStateChanged
	FDelegateHandle PlayerTalkingStateChangedDelegateHandle;
};
