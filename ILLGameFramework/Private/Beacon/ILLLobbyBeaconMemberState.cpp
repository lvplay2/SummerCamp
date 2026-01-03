// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLLobbyBeaconMemberState.h"

#include "ILLLobbyBeaconClient.h"
#include "ILLLobbyBeaconState.h"

AILLLobbyBeaconMemberState::AILLLobbyBeaconMemberState(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void AILLLobbyBeaconMemberState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AILLLobbyBeaconMemberState, OnlinePlatformName);
}

void AILLLobbyBeaconMemberState::ReceiveClient(AILLSessionBeaconClient* BeaconClient)
{
	Super::ReceiveClient(BeaconClient);

	// Assign the online platform subsystem name and simulate the on-rep
	OnlinePlatformName = BeaconClient->GetNetConnection()->GetPlayerOnlinePlatformName();
	OnRep_OnlinePlatformName();
}

void AILLLobbyBeaconMemberState::OnRep_LeaderAccountId()
{
	Super::OnRep_LeaderAccountId();

	// Clear the cached pointer and attempt to find the new leader
	PartyLeaderMemberState = nullptr;
	if (LeaderAccountId.IsValid())
	{
		GetPartyLeaderMemberState();
	}
}

void AILLLobbyBeaconMemberState::OnRep_OnlinePlatformName()
{
}

AILLLobbyBeaconMemberState* AILLLobbyBeaconMemberState::GetPartyLeaderMemberState() const
{
	if (!PartyLeaderMemberState && LeaderAccountId.IsValid())
	{
		if (AILLLobbyBeaconState* StateOwner = Cast<AILLLobbyBeaconState>(GetOwner()))
		{
			PartyLeaderMemberState = Cast<AILLLobbyBeaconMemberState>(StateOwner->FindMember(LeaderAccountId));
		}
	}

	return PartyLeaderMemberState;
}
