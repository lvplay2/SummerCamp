// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLSessionBeaconMemberState.h"
#include "ILLLobbyBeaconMemberState.generated.h"

/**
 * @class AILLLobbyBeaconMemberState
 */
UCLASS(Config=Engine)
class ILLGAMEFRAMEWORK_API AILLLobbyBeaconMemberState
: public AILLSessionBeaconMemberState
{
	GENERATED_UCLASS_BODY()

public:
	// Begin AILLSessionBeaconMemberState interface
	virtual void ReceiveClient(AILLSessionBeaconClient* BeaconClient) override;
	// End AILLSessionBeaconMemberState interface

protected:
	// Begin AILLSessionBeaconMemberState interface
	virtual void OnRep_LeaderAccountId() override;
	// End AILLSessionBeaconMemberState interface

	//////////////////////////////////////////////////////////////////////////
	// Platform
public:
	/** @return Online platform subsystem name for this member. */
	FORCEINLINE const FName& GetOnlinePlatformName() const { return OnlinePlatformName; }

protected:
	/** Replication handler for OnlinePlatformName. */
	UFUNCTION()
	virtual void OnRep_OnlinePlatformName();

	// Online platform subsystem name
	UPROPERTY(Transient, ReplicatedUsing=OnRep_OnlinePlatformName)
	FName OnlinePlatformName;

	//////////////////////////////////////////////////////////////////////////
	// Party affiliation
public:
	/** @return Lobby beacon member state that is the party leader for this member. */
	AILLLobbyBeaconMemberState* GetPartyLeaderMemberState() const;

protected:
	// Cached member state for this member's party leader
	UPROPERTY(Transient)
	mutable AILLLobbyBeaconMemberState* PartyLeaderMemberState;
};
