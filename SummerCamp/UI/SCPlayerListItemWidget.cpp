// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPlayerListItemWidget.h"

#include "SCGameState_Hunt.h"
#include "SCLobbyBeaconMemberState.h"
#include "SCPlayerState.h"

#define LOCTEXT_NAMESPACE "ILLFonic.F13.SCPlayerListItemWidget"

USCPlayerListItemWidget::USCPlayerListItemWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

ESlateVisibility USCPlayerListItemWidget::GetPickedCharacterPanelVisibility() const
{
	ASCGameState* GS = GetWorld() ? GetWorld()->GetGameState<ASCGameState>() : nullptr;
	if (PlayerState && GS)
	{
		if (GS->HasMatchStarted() && PlayerState->HasFullyTraveled())
			return ESlateVisibility::Visible;
	}

	return ESlateVisibility::Collapsed;
}

ESlateVisibility USCPlayerListItemWidget::GetReadBoxPanelVisibility() const
{
	ASCGameState* GS = GetWorld() ? GetWorld()->GetGameState<ASCGameState>() : nullptr;
	if(GS)
	{
		if (GS->IsMatchWaitingToStart() || GS->IsMatchWaitingPreMatch())
			return ESlateVisibility::Visible;
	}

	return ESlateVisibility::Collapsed;
}

ESlateVisibility USCPlayerListItemWidget::GetHostArrowImageVisibility() const
{
	if (PlayerState && PlayerState->IsHost())
			return ESlateVisibility::Visible;

	return ESlateVisibility::Collapsed;
}

ESlateVisibility USCPlayerListItemWidget::GetCharacterSelectPanelVisibility() const
{
	if (PlayerState && !PlayerState->HasFullyTraveled())
		return ESlateVisibility::Collapsed;

	ASCGameState* GS = GetWorld() ? GetWorld()->GetGameState<ASCGameState>() : nullptr;
	if (GS)
	{
		return GS->HasMatchStarted() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible;
	}

	return ESlateVisibility::Collapsed;
}

ESlateVisibility USCPlayerListItemWidget::GetPlayerStatusPanelVisibility() const
{
	return GetStatusDisplayText().IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible;
}

ESlateVisibility USCPlayerListItemWidget::GetSpectatingPanelVisibility() const
{
	return bSpectating ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
}

ESlateVisibility USCPlayerListItemWidget::GetAliveDeadPanelVisiblity() const
{
	return bSpectating ? ESlateVisibility::Collapsed : ESlateVisibility::Visible;
}

ESlateVisibility USCPlayerListItemWidget::GetKillerPickerButtonVisibility() const
{
	ASCGameState* GS = GetWorld() ? GetWorld()->GetGameState<ASCGameState>() : nullptr;
	if (PlayerState && GS)
	{
		if (PlayerState->HasFullyTraveled())
			return ESlateVisibility::Visible;
	}

	return ESlateVisibility::Collapsed;
}

ESlateVisibility USCPlayerListItemWidget::GetRemoteInRangeOrWalkieSpeakerVisibility() const
{
	ASCPlayerController* OwningPlayer = Cast<ASCPlayerController>(GetOwningPlayer());
	ASCPlayerState* OwningPS = OwningPlayer ? Cast<ASCPlayerState>(OwningPlayer->PlayerState) : nullptr;

	// Hide when no player, no local player or this is the local player
	if (!LobbyMemberState || !OwningPS || OwningPS == PlayerState)
		return ESlateVisibility::Collapsed;

	if (ASCPlayerState* OurPS = Cast<ASCPlayerState>(PlayerState))
	{
		// Check the LobbyMemberState for speaking because the PlayerState will check party voice
		if (LobbyMemberState->IsPlayerTalking())
		{
			if (OwningPS->IsPlayerInAttenuationRange(OurPS) || (OwningPS->HasWalkieTalkie() && OurPS->HasWalkieTalkie()))
				return ESlateVisibility::Visible;
		}
	}

	return ESlateVisibility::Collapsed;
}

ESlateVisibility USCPlayerListItemWidget::GetMutualWalkieVisibility() const
{
	ASCPlayerController* OwningPlayer = Cast<ASCPlayerController>(GetOwningPlayer());
	ASCPlayerState* OwningPS = OwningPlayer ? Cast<ASCPlayerState>(OwningPlayer->PlayerState) : nullptr;
	ASCPlayerState* OurPS = Cast<ASCPlayerState>(PlayerState);

	if (OwningPS && OurPS)
	{
		// Never show this for the killer (even though he should never have a walkie..? brought over from Blueprint)
		if (!OurPS->IsActiveCharacterKiller())
		{
			if (OwningPS->HasWalkieTalkie() && OurPS->HasWalkieTalkie())
				return ESlateVisibility::Visible;
		}
	}

	return ESlateVisibility::Collapsed;
}

FText USCPlayerListItemWidget::GetStatusDisplayText() const
{
	UWorld* World = GetWorld();
	ASCGameState* GameState = World ? World->GetGameState<ASCGameState>() : nullptr;
	ASCPlayerState* SCPlayerState = Cast<ASCPlayerState>(PlayerState);
	if (!GameState || !SCPlayerState || !SCPlayerState->HasFullyTraveled())
		return LOCTEXT("PlayerStatusText_Loading", "Loading...");

	if (GameState->HasMatchStarted())
	{
		if (SCPlayerState->IsActiveCharacterKiller())
		{
			// Killer status
			int32 TotalCounselors = GameState->SpawnedCounselorCount;
			//if (GameState->bIsHunterSpawned) // 02/03/2021 - re-enable Hunter Selection
				++TotalCounselors;

			return FText::Format(LOCTEXT("KillerStatusText", "Killed {0}/{1}"), SCPlayerState->GetKills(), TotalCounselors);
		}
		else
		{
			// Counselor status in Hunt game mode
			if (SCPlayerState->GetEscaped())
				return LOCTEXT("CounselorStatusText_Escaped", "Escaped");

			if (SCPlayerState->GetIsDead())
				return SCPlayerState->GetDeathDisplayText();

			return LOCTEXT("CounselorStatusText_Alive", "Alive");
		}
	}

	return FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE
