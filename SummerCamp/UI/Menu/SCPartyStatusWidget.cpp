// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPartyStatusWidget.h"

#include "ILLBackendPlayer.h"

#include "SCGameInstance.h"
#include "SCLocalPlayer.h"
#include "SCOnlineSessionClient.h"
#include "SCPartyBeaconMemberState.h"
#include "SCPartyBeaconState.h"
#include "SCPlayerState.h"

void USCPartyStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	PollPartyState();
}

void USCPartyStatusWidget::NativeDestruct()
{
	if (bHasPartyState)
	{
		if (ASCPartyBeaconState* PBS = GetPartyBeaconState())
		{
			PBS->OnBeaconMemberAdded().RemoveAll(this);
			PBS->OnBeaconMemberRemoved().RemoveAll(this);
			bHasPartyState = false;
		}
	}

	Super::NativeDestruct();
}

void USCPartyStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	PollPartyState();
}

void USCPartyStatusWidget::PollPartyState()
{
	UWorld* World = GetWorld();
	if (!World)
		return;

	if (!LocalPlayer)
	{
		LocalPlayer = Cast<USCLocalPlayer>(World->GetFirstLocalPlayerFromController());
	}
	else if (!LocalPlayerState)
	{
		// Grab the LocalPlayerState from the first local player's player controller
		if (IsValid(LocalPlayer->PlayerController))
			LocalPlayerState = Cast<ASCPlayerState>(LocalPlayer->PlayerController->PlayerState);
	}

	// Wait for the local player to be setup before doing anything with members
	ASCPartyBeaconState* PBS = GetPartyBeaconState();
	if (LocalPlayerState && LocalPlayerState->GetAccountId().IsValid() && PBS)
	{
		const int32 NumMembers = PBS->GetNumMembers();
		for (int32 MemberIndex = 0; MemberIndex < NumMembers; ++MemberIndex)
		{
			if (ASCPartyBeaconMemberState* MemberState = Cast<ASCPartyBeaconMemberState>(PBS->GetMemberAtIndex(MemberIndex)))
			{
				if (!AddedMembers.Contains(MemberState) && MemberState->GetAccountId().IsValid())
					OnBeaconMemberAdded(MemberState);
			}
		}

		if (!bHasPartyState)
		{
			PBS->OnBeaconMemberAdded().AddUObject(this, &ThisClass::OnBeaconMemberAdded);
			PBS->OnBeaconMemberRemoved().AddUObject(this, &ThisClass::OnBeaconMemberRemoved);
			bHasPartyState = true;
		}
	}
	else
	{
		// Remove all previously-added members
		while (AddedMembers.Num() > 0)
		{
			OnBeaconMemberRemoved(AddedMembers[0]);
		}

		if (PBS)
		{
			PBS->OnBeaconMemberAdded().RemoveAll(this);
			PBS->OnBeaconMemberRemoved().RemoveAll(this);
		}

		bHasPartyState = false;
	}
}

USCOnlineSessionClient* USCPartyStatusWidget::GetOnlnineSessionClient() const
{
	UWorld* World = GetWorld();
	USCGameInstance* GI = World ? Cast<USCGameInstance>(World->GetGameInstance()) : nullptr;
	return GI ? Cast<USCOnlineSessionClient>(GI->GetOnlineSession()) : nullptr;
}

ASCPartyBeaconState* USCPartyStatusWidget::GetPartyBeaconState() const
{
	USCOnlineSessionClient* OSC = GetOnlnineSessionClient();
	return OSC ? Cast<ASCPartyBeaconState>(OSC->GetPartyBeaconState()) : nullptr;
}

void USCPartyStatusWidget::OnBeaconMemberAdded(AILLSessionBeaconMemberState* MemberState)
{
	check(LocalPlayer);
	check(LocalPlayerState);
	
	if (ASCPartyBeaconMemberState* PartyMemberState = Cast<ASCPartyBeaconMemberState>(MemberState))
	{
		// Pointer guard
		if (!PartyMemberState->GetAccountId().IsValid())
			return;

		// Skip adding the local player to the UI
		if (LocalPlayer->GetBackendPlayer()->GetAccountId() == PartyMemberState->GetAccountId())
			return;

		AddedMembers.Add(PartyMemberState);
		OnPartyMemberAdded(PartyMemberState);
	}
}

void USCPartyStatusWidget::OnBeaconMemberRemoved(AILLSessionBeaconMemberState* MemberState)
{
	if (ASCPartyBeaconMemberState* PartyMemberState = Cast<ASCPartyBeaconMemberState>(MemberState))
	{
		AddedMembers.Remove(MemberState);
		OnPartyMemberRemoved(PartyMemberState);
	}
	else if (!MemberState && AddedMembers.Contains(nullptr))
	{
		AddedMembers.Remove(nullptr);
		OnPartyMemberRemoved(nullptr);
	}
}
