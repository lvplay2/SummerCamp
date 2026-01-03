// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLPartyBeaconState.h"

#include "ILLPartyBeaconMemberState.h"

#define LOCTEXT_NAMESPACE "ILLFonic.IGF.ILLPartyBeaconState"

FName ILLPartyLeaderState::Idle(NAME_None);
FName ILLPartyLeaderState::Matchmaking(TEXT("Matchmaking"));
FName ILLPartyLeaderState::CreatingPrivateMatch(TEXT("CreatingPrivateMatch"));
FName ILLPartyLeaderState::CreatingPublicMatch(TEXT("CreatingPublicMatch"));
FName ILLPartyLeaderState::JoiningGame(TEXT("JoiningGame"));
FName ILLPartyLeaderState::InMatch(TEXT("InMatch"));

AILLPartyBeaconState::AILLPartyBeaconState(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	MemberStateClass = AILLPartyBeaconMemberState::StaticClass();
}

void AILLPartyBeaconState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AILLPartyBeaconState, LeaderStateName);
}

TArray<FILLBackendPlayerIdentifier> AILLPartyBeaconState::GenerateAccountIdList() const
{
	TArray<FILLBackendPlayerIdentifier> Result;
	for (int32 MemberIndex = 0; MemberIndex < GetNumMembers(); ++MemberIndex)
	{
		if (AILLPartyBeaconMemberState* PartyMember = Cast<AILLPartyBeaconMemberState>(GetMemberAtIndex(MemberIndex)))
		{
			Result.Add(PartyMember->GetAccountId());
		}
	}

	return Result;
}

FText AILLPartyBeaconState::GetLeaderStateDescription() const
{
	if (LeaderStateName == ILLPartyLeaderState::Matchmaking)
		return LOCTEXT("PartyLeaderState_Matchmaking", "Matchmaking");
	if (LeaderStateName == ILLPartyLeaderState::CreatingPrivateMatch)
		return LOCTEXT("PartyLeaderState_CreatingPrivateMatch", "Creating a private match");
	if (LeaderStateName == ILLPartyLeaderState::CreatingPublicMatch)
		return LOCTEXT("PartyLeaderState_CreatingPublicMatch", "Creating a public match");
	if (LeaderStateName == ILLPartyLeaderState::JoiningGame)
		return LOCTEXT("PartyLeaderState_JoiningGame", "Joining a game");
	if (LeaderStateName == ILLPartyLeaderState::InMatch)
		return LOCTEXT("PartyLeaderState_InMatch", "In a match");
	return LOCTEXT("PartyLeaderState_Idle", "Idle"); // Assume idle
}

void AILLPartyBeaconState::OnRep_LeaderStateName()
{
}

#undef LOCTEXT_NAMESPACE
