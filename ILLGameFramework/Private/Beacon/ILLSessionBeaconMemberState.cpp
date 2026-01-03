// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLSessionBeaconMemberState.h"

#include "OnlineBeacon.h"

#include "ILLBackendPlayer.h"
#include "ILLLocalPlayer.h"
#include "ILLSessionBeaconClient.h"
#include "ILLSessionBeaconState.h"

FILLSessionBeaconPlayer::FILLSessionBeaconPlayer(UILLLocalPlayer* LocalPlayer)
{
	if (LocalPlayer->GetBackendPlayer() && LocalPlayer->GetBackendPlayer()->IsLoggedIn())
	{
		AccountId = LocalPlayer->GetBackendPlayer()->GetAccountId();
	}
	else
	{
		AccountId.Reset();
	}
	UniqueId = LocalPlayer->GetPreferredUniqueNetId();
	DisplayName = LocalPlayer->GetNickname();
}

bool FILLSessionBeaconPlayer::IsValidPlayer() const
{
	return (AccountId.IsValid());
}

bool FILLSessionBeaconReservation::IsValidReservation() const
{
	// Verify a member list is there
	if (Players.Num() <= 0)
		return false;

	// Only allow no LeaderAccountId for one-person reservations
	if (Players.Num() != 1 && !LeaderAccountId.IsValid())
		return false;

	// Verify each Player
	for (const FILLSessionBeaconPlayer& Player : Players)
	{
		if (!Player.IsValidPlayer())
		{
			return false;
		}
	}

	return true;
}

FString FILLSessionBeaconReservation::ToDebugString() const
{
	return FString::Printf(TEXT("Leader:%s NumMembers:%d"), *LeaderAccountId.ToDebugString(), Players.Num());
}

AILLSessionBeaconMemberState::AILLSessionBeaconMemberState(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bReplicates = true;
	bAlwaysRelevant = true;
	NetDriverName = NAME_BeaconNetDriver;
}

void AILLSessionBeaconMemberState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AILLSessionBeaconMemberState, bArbitrationMember);
	DOREPLIFETIME(AILLSessionBeaconMemberState, BeaconClientActor);
	DOREPLIFETIME(AILLSessionBeaconMemberState, AccountId);
	DOREPLIFETIME(AILLSessionBeaconMemberState, LeaderAccountId);
	DOREPLIFETIME(AILLSessionBeaconMemberState, UniqueId);
	DOREPLIFETIME(AILLSessionBeaconMemberState, DisplayName);
	DOREPLIFETIME(AILLSessionBeaconMemberState, bReservationAccepted);
}

bool AILLSessionBeaconMemberState::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
	UClass* BeaconClass = RealViewer->GetClass();
	return (BeaconClass && BeaconClass->IsChildOf(AILLSessionBeaconClient::StaticClass()));
}

void AILLSessionBeaconMemberState::BeginPlay()
{
	Super::BeginPlay();

	CheckHasSynchronized();
}

void AILLSessionBeaconMemberState::PostNetInit()
{
	Super::PostNetInit();

	CheckHasSynchronized();
}

void AILLSessionBeaconMemberState::OnRep_Owner()
{
	Super::OnRep_Owner();

	CheckHasSynchronized();
}

bool AILLSessionBeaconMemberState::HasFullySynchronized() const
{
	if (AILLSessionBeaconState* StateOwner = Cast<AILLSessionBeaconState>(GetOwner()))
	{
		// Wait for a UniqueId and DisplayName
		if (AccountId.IsValid() && !DisplayName.IsEmpty())
		{
			// Wait until they are present in the StateOwner's member list too
			if (StateOwner->FindMember(AccountId))
				return true;
		}
	}

	return false;
}

FString AILLSessionBeaconMemberState::ToDebugString() const
{
	return FString::Printf(TEXT("%s%s \"%s\" Leader:%s Synched:%s"), *GetName(), bReservationAccepted ? TEXT("") : TEXT(" [PENDING]"), *DisplayName, *LeaderAccountId.ToDebugString(), bHasSynchronized ? TEXT("true") : TEXT("false"));
}

void AILLSessionBeaconMemberState::ReceiveClient(AILLSessionBeaconClient* BeaconClient)
{
	check(!bReservationAccepted);
	check(!BeaconClientActor || BeaconClientActor == BeaconClient);
	check(AccountId.IsValid() && AccountId == BeaconClient->GetClientAccountId());
	check(!UniqueId.IsValid() || *UniqueId == *BeaconClient->GetClientUniqueId());
	BeaconClientActor = BeaconClient;
}

void AILLSessionBeaconMemberState::CheckHasSynchronized()
{
	if (!bHasSynchronized && HasFullySynchronized())
	{
		if (AILLSessionBeaconState* StateOwner = Cast<AILLSessionBeaconState>(GetOwner()))
		{
			UE_LOG(LogBeacon, Verbose, TEXT("%s (%s) has fully synchronized"), *GetName(), *DisplayName);

			bHasSynchronized = true;

			StateOwner->DumpState();

			// Broadcast our addition
			StateOwner->OnBeaconMemberAdded().Broadcast(this);

			// Check if we have received everyone
			StateOwner->CheckHasReceivedMembers();
		}
	}
}

void AILLSessionBeaconMemberState::OnRep_DisplayName()
{
	CheckHasSynchronized();
}

void AILLSessionBeaconMemberState::OnRep_LeaderAccountId()
{
}

void AILLSessionBeaconMemberState::OnRep_ReservationAccepted()
{
}

void AILLSessionBeaconMemberState::OnRep_AccountId()
{
	CheckHasSynchronized();
}

void AILLSessionBeaconMemberState::OnRep_UniqueId()
{
	CheckHasSynchronized();
}

bool AILLSessionBeaconMemberState::IsPlayerTalking() const
{
	if (bHasTalked)
	{
		const float Seconds = FPlatformTime::Seconds();
		if (StartTalkTime != 0.0)
		{
			// Must transmit for more than 0.1s
			if (Seconds-StartTalkTime > 0.1)
				return true;
		}
		if (LastTalkTime != 0.0)
		{
			// Linger for 0.2s
			if (Seconds-LastTalkTime < 0.2)
				return true;
		}
	}

	return false;
}
