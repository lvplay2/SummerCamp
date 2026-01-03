// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLPlayerListItemWidget.h"

#include "ILLLobbyBeaconMemberState.h"
#include "ILLLobbyBeaconState.h"
#include "ILLOnlineSessionClient.h"
#include "ILLPartyBeaconMemberState.h"
#include "ILLPartyBeaconState.h"
#include "ILLPlayerState.h"

UILLPlayerListItemWidget::UILLPlayerListItemWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void UILLPlayerListItemWidget::AttemptToSynchronize()
{
	UWorld* World = GetWorld();
	if (!World || !World->GetGameState())
		return;

	// Determine if we need anything
	UILLOnlineSessionClient* OSC = Cast<UILLOnlineSessionClient>(World->GetGameInstance()->GetOnlineSession());
	if (!OSC)
		return;
	const bool bNeedsLobbyMember = (!LobbyMemberState && OSC->GetLobbyBeaconState());
	const bool bNeedsPartyMember = (!PartyMemberState && OSC->GetPartyBeaconState());
	if (PlayerState && !bNeedsLobbyMember && !bNeedsPartyMember)
		return;

	// Attempt to identify ourself
	const FILLBackendPlayerIdentifier& KnownAccountId = GetPlayerAccountId();
	if (KnownAccountId.IsValid())
	{
		// Search for a matching lobby member
		if (bNeedsLobbyMember)
		{
			if (AILLSessionBeaconMemberState* Member = OSC->GetLobbyBeaconState()->FindMember(KnownAccountId))
				SetLobbyMemberState(CastChecked<AILLLobbyBeaconMemberState>(Member));
		}

		// Search for a matching party member
		if (bNeedsPartyMember)
		{
			if (AILLSessionBeaconMemberState* Member = OSC->GetPartyBeaconState()->FindMember(KnownAccountId))
				SetPartyMemberState(CastChecked<AILLPartyBeaconMemberState>(Member));
		}

		// Attempt to find their PlayerState in the GameState
		for (APlayerState* Player : World->GetGameState()->PlayerArray)
		{
			if (Player->bFromPreviousLevel)
				continue;

			AILLPlayerState* PS = Cast<AILLPlayerState>(Player);
			if (PS->GetAccountId().IsValid() && KnownAccountId == PS->GetAccountId())
			{
				SetPlayerState(PS);
				break;
			}
		}
	}
	else
	{
		FUniqueNetIdRepl KnownUniqueId = GetPlayerUniqueId();
		if (KnownUniqueId.IsValid())
		{
			// Search for a matching lobby member
			if (bNeedsLobbyMember)
			{
				if (AILLSessionBeaconMemberState* Member = OSC->GetLobbyBeaconState()->FindMember(KnownUniqueId))
					SetLobbyMemberState(CastChecked<AILLLobbyBeaconMemberState>(Member));
			}

			// Search for a matching party member
			if (bNeedsPartyMember)
			{
				if (AILLSessionBeaconMemberState* Member = OSC->GetPartyBeaconState()->FindMember(KnownUniqueId))
					SetPartyMemberState(CastChecked<AILLPartyBeaconMemberState>(Member));
			}

			// Attempt to find their PlayerState in the GameState
			for (APlayerState* Player : World->GetGameState()->PlayerArray)
			{
				if (Player->bFromPreviousLevel)
					continue;

				AILLPlayerState* PS = Cast<AILLPlayerState>(Player);
				if (KnownUniqueId.IsValid() && PS->UniqueId.IsValid() && *PS->UniqueId == *KnownUniqueId)
				{
					SetPlayerState(PS);
					break;
				}
			}
		}
	}
}

bool UILLPlayerListItemWidget::ExchangeWith(UILLPlayerListItemWidget* OtherWidget)
{
	bool bDiffers = false;
	if (GetPlayerAccountId().IsValid())
		bDiffers = (GetPlayerAccountId() != OtherWidget->GetPlayerAccountId());
	else
		bDiffers = (GetPlayerUniqueId().IsValid() && OtherWidget->GetPlayerUniqueId().IsValid() && *GetPlayerUniqueId() != *OtherWidget->GetPlayerUniqueId());
	if (!bDiffers)
	{
		// Bots do not have unique IDs so we still should fall back to exchanging based on PlayerState differences
		if (PlayerState && OtherWidget->PlayerState && PlayerState != OtherWidget->PlayerState)
			bDiffers = true;
	}

	if (bDiffers)
	{
		// Synchronize the other widget and copy of it's states
		OtherWidget->AttemptToSynchronize();
		AILLPlayerState* TempPlayerState = OtherWidget->PlayerState;
		AILLLobbyBeaconMemberState* TempMemberState = OtherWidget->LobbyMemberState;
		AILLPartyBeaconMemberState* TempPartyState = OtherWidget->PartyMemberState;

		// Give the other widget our states
		OtherWidget->SetItemStates(LobbyMemberState, PartyMemberState, PlayerState);
		OtherWidget->AttemptToSynchronize();

		// Take the other widget's states
		SetItemStates(TempMemberState, TempPartyState, TempPlayerState);

		return true;
	}

	return false;
}

FString UILLPlayerListItemWidget::GetDisplayName() const
{
	FString Result;
	if (IsValid(LobbyMemberState))
		Result = LobbyMemberState->GetDisplayName();
	if (Result.IsEmpty() && IsValid(PartyMemberState))
		Result = PartyMemberState->GetDisplayName();
	if (Result.IsEmpty() && IsValid(PlayerState))
		Result = PlayerState->PlayerName;
	return Result;
}

int32 UILLPlayerListItemWidget::GetRealPing() const
{
	if (PlayerState)
		return PlayerState->GetRealPing();

	return 0;
}

FText UILLPlayerListItemWidget::GetPingDisplayText() const
{
	if (PlayerState)
		return PlayerState->GetPingDisplayText();

	return FText();
}

bool UILLPlayerListItemWidget::IsLocalPlayer() const
{
	if (PlayerState)
	{
		if (AILLPlayerController* PCOwner = Cast<AILLPlayerController>(PlayerState->GetOwner()))
		{
			return PCOwner->IsLocalController();
		}
	}

	return false;
}

const FILLBackendPlayerIdentifier& UILLPlayerListItemWidget::GetPlayerAccountId() const
{
	if (IsValid(LobbyMemberState) && LobbyMemberState->GetAccountId().IsValid())
		return LobbyMemberState->GetAccountId();
	if (IsValid(PartyMemberState) && PartyMemberState->GetAccountId().IsValid())
		return PartyMemberState->GetAccountId();
	if (IsValid(PlayerState) && PlayerState->GetAccountId().IsValid())
		return PlayerState->GetAccountId();

	return FILLBackendPlayerIdentifier::GetInvalid();
}

FUniqueNetIdRepl UILLPlayerListItemWidget::GetPlayerUniqueId() const
{
	if (IsValid(LobbyMemberState) && LobbyMemberState->GetMemberUniqueId().IsValid())
		return LobbyMemberState->GetMemberUniqueId();
	if (IsValid(PartyMemberState) && PartyMemberState->GetMemberUniqueId().IsValid())
		return PartyMemberState->GetMemberUniqueId();
	if (IsValid(PlayerState) && PlayerState->UniqueId.IsValid())
		return PlayerState->UniqueId;

	return FUniqueNetIdRepl();
}

bool UILLPlayerListItemWidget::HasFullyTraveled() const
{
	if (PlayerState && PlayerState->HasFullyTraveled())
		return true;

	return false;
}

bool UILLPlayerListItemWidget::IsTalking() const
{
	if (PlayerState)
		return PlayerState->IsPlayerTalking();
	if (LobbyMemberState && LobbyMemberState->IsPlayerTalking())
		return true;
	if (PartyMemberState && PartyMemberState->IsPlayerTalking())
		return true;

	return false;
}

bool UILLPlayerListItemWidget::HasTalked() const
{
	if (PlayerState)
		return PlayerState->HasPlayerTalked();
	if (LobbyMemberState && LobbyMemberState->bHasTalked)
		return true;
	if (PartyMemberState && PartyMemberState->bHasTalked)
		return true;

	return false;
}
