// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLSessionBeaconState.h"

#include "Online.h"

#include "ILLSessionBeaconClient.h"

void FILLSessionBeaconMemberStateActorInfo::PreReplicatedRemove(const FILLSessionBeaconMemberStateInfoArray& InArraySerializer)
{
	check(InArraySerializer.ParentState);
	if (BeaconMemberState)
	{
		InArraySerializer.ParentState->OnBeaconMemberRemoved().Broadcast(BeaconMemberState);
	}
}

void FILLSessionBeaconMemberStateActorInfo::PostReplicatedAdd(const FILLSessionBeaconMemberStateInfoArray& InArraySerializer)
{
	check(InArraySerializer.ParentState);
	if (BeaconMemberState)
	{
	}
	else
	{
		UE_LOG(LogBeacon, Verbose, TEXT("PostReplicatedAdd with a null BeaconMemberState actor, expect a future PostReplicatedChange"));
	}
}

void FILLSessionBeaconMemberStateActorInfo::PostReplicatedChange(const FILLSessionBeaconMemberStateInfoArray& InArraySerializer)
{
	check(InArraySerializer.ParentState);
	if (BeaconMemberState)
	{
	}
	else
	{
		UE_LOG(LogBeacon, Verbose, TEXT("PostReplicatedChange to a null BeaconMemberState actor"));
	}
}

AILLSessionBeaconMemberState* FILLSessionBeaconMemberStateInfoArray::AddMember(AILLSessionBeaconMemberState* MemberState)
{
	if (MemberState)
	{
		int32 Index = Members.Add(MemberState);
		MarkItemDirty(Members[Index]);

		// server calls "on rep" also
		Members[Index].PostReplicatedAdd(*this);

		return Members[Index].BeaconMemberState;
	}

	return nullptr;
}

AILLSessionBeaconMemberState* FILLSessionBeaconMemberStateInfoArray::GetArbitrationMember() const
{
	for (const FILLSessionBeaconMemberStateActorInfo& Member : Members)
	{
		if (Member.BeaconMemberState && Member.BeaconMemberState->bArbitrationMember)
		{
			return Member.BeaconMemberState;
		}
	}

	return nullptr;
}

AILLSessionBeaconMemberState* FILLSessionBeaconMemberStateInfoArray::GetMember(const FILLBackendPlayerIdentifier& AccountId) const
{
	if (AccountId.IsValid())
	{
		for (const FILLSessionBeaconMemberStateActorInfo& Member : Members)
		{
			if (Member.BeaconMemberState && Member.BeaconMemberState->GetAccountId() == AccountId)
			{
				return Member.BeaconMemberState;
			}
		}
	}

	return nullptr;
}

AILLSessionBeaconMemberState* FILLSessionBeaconMemberStateInfoArray::GetMember(const FUniqueNetIdRepl& UniqueId) const
{
	if (UniqueId.IsValid())
	{
		for (const FILLSessionBeaconMemberStateActorInfo& Member : Members)
		{
			if (Member.BeaconMemberState && Member.BeaconMemberState->GetMemberUniqueId().IsValid() && *Member.BeaconMemberState->GetMemberUniqueId() == *UniqueId)
			{
				return Member.BeaconMemberState;
			}
		}
	}

	return nullptr;
}

AILLSessionBeaconMemberState* FILLSessionBeaconMemberStateInfoArray::GetMember(const AILLSessionBeaconClient* ClientActor) const
{
	if (ClientActor && ClientActor->GetNetMode() < NM_Client)
	{
		for (const FILLSessionBeaconMemberStateActorInfo& Member : Members)
		{
			if (Member.BeaconMemberState && Member.BeaconMemberState->GetBeaconClientActor() == ClientActor)
			{
				return Member.BeaconMemberState;
			}
		}
	}

	return nullptr;
}

void FILLSessionBeaconMemberStateInfoArray::RemoveMember(AILLSessionBeaconClient* ClientActor)
{
	for (int32 MemberIdx = 0; MemberIdx < Members.Num(); MemberIdx++)
	{
		FILLSessionBeaconMemberStateActorInfo& Member = Members[MemberIdx];
		if (Member.BeaconMemberState->GetBeaconClientActor() == ClientActor)
		{
			RemoveMemberAtIndex(MemberIdx);
			break;
		}
	}
}

void FILLSessionBeaconMemberStateInfoArray::RemoveMember(const FUniqueNetIdRepl& UniqueId)
{
	for (int32 MemberIdx = 0; MemberIdx < Members.Num(); MemberIdx++)
	{
		FILLSessionBeaconMemberStateActorInfo& Member = Members[MemberIdx];
		if (Member.BeaconMemberState && *Member.BeaconMemberState->GetMemberUniqueId() == *UniqueId)
		{
			RemoveMemberAtIndex(MemberIdx);
			break;
		}
	}
}

void FILLSessionBeaconMemberStateInfoArray::RemoveMemberAtIndex(const int32 Index)
{
	check(Index >= 0 && Index < Members.Num());

	FILLSessionBeaconMemberStateActorInfo& Member = Members[Index];
	if (Member.BeaconMemberState->GetNetMode() < NM_Client)
	{
		// Server calls "on rep" also
		Member.PreReplicatedRemove(*this);

		// Destroy it
		Member.BeaconMemberState->Destroy();
		Member.BeaconMemberState = nullptr;

		// Remove and swap
		Members.RemoveAtSwap(Index);

		// Replicate
		MarkArrayDirty();
	}
}

void FILLSessionBeaconMemberStateInfoArray::DumpState()
{
	int32 Count = 0;
	for (const FILLSessionBeaconMemberStateActorInfo& MemberState : Members)
	{
		if (const AILLSessionBeaconMemberState* Member = MemberState.BeaconMemberState)
		{
			UE_LOG(LogBeacon, Verbose, TEXT("[%d] %s"), ++Count, *Member->ToDebugString());
		}
	}
}

AILLSessionBeaconState::AILLSessionBeaconState(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bReplicates = true;
	bAlwaysRelevant = true;
	NetDriverName = NAME_BeaconNetDriver;

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		Members.ParentState = this;
	}

	MemberStateClass = AILLSessionBeaconMemberState::StaticClass();
}

void AILLSessionBeaconState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AILLSessionBeaconState, Members);
}

void AILLSessionBeaconState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Listen for voice state changes
	if (IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get())
	{
		IOnlineVoicePtr VoiceInt = OnlineSub->GetVoiceInterface();
		if (VoiceInt.IsValid())
		{
			PlayerTalkingStateChangedDelegateHandle = VoiceInt->AddOnPlayerTalkingStateChangedDelegate_Handle(FOnPlayerTalkingStateChangedDelegate::CreateUObject(this, &ThisClass::OnPlayerTalkingStateChanged));
		}
	}
}

void AILLSessionBeaconState::Destroyed()
{
	// No longer listen for voice state changes
	if (IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get())
	{
		IOnlineVoicePtr VoiceInt = OnlineSub->GetVoiceInterface();
		if (VoiceInt.IsValid())
		{
			VoiceInt->ClearOnPlayerTalkingStateChangedDelegate_Handle(PlayerTalkingStateChangedDelegateHandle);
		}
	}

	Super::Destroyed();
}

bool AILLSessionBeaconState::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
	UClass* BeaconClass = RealViewer->GetClass();
	return (BeaconClass && BeaconClass->IsChildOf(AILLSessionBeaconClient::StaticClass()));
}

AILLSessionBeaconMemberState* AILLSessionBeaconState::SpawnMember(const FILLBackendPlayerIdentifier& AccountId, const FUniqueNetIdRepl& UniqueId, const FString& DisplayName)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *AccountId.ToDebugString());

	check(!FindMember(AccountId));
	if (UniqueId.IsValid())
	{
		check(!FindMember(UniqueId));
	}

	UWorld* World = GetWorld();
	check(World);

	AILLSessionBeaconMemberState* NewMember = nullptr;
	if (ensure(MemberStateClass))
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.ObjectFlags |= EObjectFlags::RF_Transient;
		SpawnParams.Owner = this;
		NewMember = World->SpawnActor<AILLSessionBeaconMemberState>(MemberStateClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (NewMember)
		{
			// Initialize the member
			NewMember->SetAccountId(AccountId);
			NewMember->SetDisplayName(DisplayName);
			NewMember->SetUniqueId(UniqueId);

			// Associate with this objects net driver for proper replication
			NewMember->SetNetDriverName(GetNetDriverName());
		}
	}

	return NewMember;
}

AILLSessionBeaconMemberState* AILLSessionBeaconState::AddMember(AILLSessionBeaconMemberState* MemberState)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *MemberState->ToDebugString());

	check(!FindMember(MemberState->GetMemberUniqueId()));

	if (Role == ROLE_Authority)
	{
		// Add the member to the list
		Members.AddMember(MemberState);

		// Check that they have fully synchronized
		MemberState->CheckHasSynchronized();
	}

	return nullptr;
}

AILLSessionBeaconMemberState* AILLSessionBeaconState::FindMember(const FILLBackendPlayerIdentifier& AccountId) const
{
	return Members.GetMember(AccountId);
}

AILLSessionBeaconMemberState* AILLSessionBeaconState::FindMember(const FUniqueNetIdRepl& UniqueId) const
{
	return Members.GetMember(UniqueId);
}

AILLSessionBeaconMemberState* AILLSessionBeaconState::FindMember(const AILLSessionBeaconClient* ClientActor) const
{
	if (Role == ROLE_Authority)
	{
		return Members.GetMember(ClientActor);
	}

	return nullptr;
}

AILLSessionBeaconMemberState* AILLSessionBeaconState::GetArbitrationMember() const
{
	return Members.GetArbitrationMember();
}

AILLSessionBeaconMemberState* AILLSessionBeaconState::GetMemberAtIndex(const int32 Index) const
{
	if (Index >= 0 && Index < Members.GetNumMembers())
	{
		return Members.GetAllMembers()[Index].BeaconMemberState;
	}

	return nullptr;
}

void AILLSessionBeaconState::RemoveMember(AILLSessionBeaconClient* ClientActor)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *ClientActor->GetName());

	if (Role == ROLE_Authority)
	{
		return Members.RemoveMember(ClientActor);
	}
}

void AILLSessionBeaconState::RemoveMember(const FUniqueNetIdRepl& UniqueId)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *UniqueId->ToDebugString());

	if (Role == ROLE_Authority)
	{
		Members.RemoveMember(UniqueId);
	}
}

void AILLSessionBeaconState::RemoveMemberAtIndex(const int32 Index)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: %i"), ANSI_TO_TCHAR(__FUNCTION__), Index);

	if (Role == ROLE_Authority)
	{
		Members.RemoveMemberAtIndex(Index);
	}
}

int32 AILLSessionBeaconState::GetNumAcceptedMembers() const
{
	int32 Result = 0;
	for (int32 MemberIndex = 0; MemberIndex < Members.GetNumMembers(); ++MemberIndex)
	{
		AILLSessionBeaconMemberState* MemberState = Members.GetAllMembers()[MemberIndex].BeaconMemberState;
		if (MemberState && MemberState->HasReservationBeenAccepted())
			++Result;
	}

	return Result;
}

bool AILLSessionBeaconState::HasAnyPendingMembers() const
{
	for (int32 MemberIndex = 0; MemberIndex < Members.GetNumMembers(); ++MemberIndex)
	{
		AILLSessionBeaconMemberState* MemberState = Members.GetAllMembers()[MemberIndex].BeaconMemberState;
		if (MemberState && !MemberState->HasReservationBeenAccepted())
			return true;
	}

	return false;
}

void AILLSessionBeaconState::DumpState()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s members:"), *GetName());
	Members.DumpState();
}

bool AILLSessionBeaconState::CheckHasReceivedMembers()
{
	if (!bHasReceivedMembers)
	{
		// Have we received data on all members?
		bool bHasAddedAllMembers = (Members.GetNumMembers() >= 1);
		for (int32 MemberIndex = 0; MemberIndex < Members.GetNumMembers(); ++MemberIndex)
		{
			AILLSessionBeaconMemberState* MemberState = Members.GetAllMembers()[MemberIndex].BeaconMemberState;
			if (MemberState && MemberState->bHasSynchronized)
				continue;

			// Not synchronized
			bHasAddedAllMembers = false;
			break;
		}

		if (bHasAddedAllMembers)
		{
			// Notify objects that were waiting on this
			BroadcastBeaconMembersReceived();
		}
	}

	return bHasReceivedMembers;
}

void AILLSessionBeaconState::OnRep_Members()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: bHasReceivedMembers: %s"), ANSI_TO_TCHAR(__FUNCTION__), bHasReceivedMembers ? TEXT("true") : TEXT("false"));

	// Ensure members are synchronized
	// We do this here primarily for parties, since they re-use CheckHasSynchronized for party session registration, and sometimes a member can be replicated before their entry in the list replicates
	for (int32 MemberIndex = 0; MemberIndex < Members.GetNumMembers(); ++MemberIndex)
	{
		AILLSessionBeaconMemberState* MemberState = Members.GetAllMembers()[MemberIndex].BeaconMemberState;
		if (MemberState)
			MemberState->CheckHasSynchronized();
	}

	if (bHasReceivedMembers)
	{
		// Previous members were all synchronized, so just report the new member list
		DumpState();
	}
	else
	{
		// Now check if we have synchronized all of them
		CheckHasReceivedMembers();
	}
}

void AILLSessionBeaconState::BroadcastBeaconMembersReceived()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	DumpState();

	check(!bHasReceivedMembers);
	bHasReceivedMembers = true;

	OnBeaconMembersReceived().Broadcast();
}

void AILLSessionBeaconState::OnPlayerTalkingStateChanged(TSharedRef<const FUniqueNetId> TalkerId, bool bIsTalking)
{
	if (AILLSessionBeaconMemberState* MemberState = FindMember(TalkerId))
	{
		if (bIsTalking)
		{
			MemberState->LastTalkTime = FPlatformTime::Seconds();
			if (MemberState->StartTalkTime == 0.0)
				MemberState->StartTalkTime = FPlatformTime::Seconds();
			MemberState->bHasTalked = true;
		}
		else
		{
			MemberState->StartTalkTime = 0.0;
		}
	}
}
