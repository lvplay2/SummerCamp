// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCInGameHUD_Online.h"

#include "SCGameState.h"
#include "SCWaitingForPlayersWidget.h"

ASCInGameHUD_Online::ASCInGameHUD_Online(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	WaitingForPlayersScreenClass = StaticLoadClass(USCWaitingForPlayersWidget::StaticClass(), nullptr, TEXT("/Game/UI/Loading/WaitingForPlayersMenuWidget.WaitingForPlayersMenuWidget_C"));
}

void ASCInGameHUD_Online::PostInitializeComponents()
{
	// Clean up the loading screen
	Super::PostInitializeComponents();

	// Create the waiting for players screen right away
	UWorld* World = GetWorld();
	if (!World->bBegunPlay)
	{
		WaitingForPlayersScreenInstance = Cast<USCWaitingForPlayersWidget>(GetMenuStackComp()->PushMenu(WaitingForPlayersScreenClass));
		WaitingForPlayersScreenInstance->OnFirstDisplayed();
	}
}

void ASCInGameHUD_Online::Destroyed()
{
	Super::Destroyed();

	// Cleanup WaitingForPlayersScreenInstance
	RemoveWaitingForPlayers();
}

void ASCInGameHUD_Online::OnLevelIntroSequenceStarted()
{
	// Cleanup WaitingForPlayersScreenInstance
	RemoveWaitingForPlayers();

	// Hide the mouse cursor
	Super::OnLevelIntroSequenceStarted();
}

void ASCInGameHUD_Online::OnSpawnIntroSequenceStarted()
{
	// Cleanup WaitingForPlayersScreenInstance
	RemoveWaitingForPlayers();

	// Update the cursor and HUD
	Super::OnSpawnIntroSequenceStarted();
}

void ASCInGameHUD_Online::OnSpawnIntroSequenceCompleted()
{
	// Cleanup WaitingForPlayersScreenInstance
	// Only hit when no level or spawn intro plays for this player
	RemoveWaitingForPlayers();

	// Update the HUD
	Super::OnSpawnIntroSequenceCompleted();
}

void ASCInGameHUD_Online::UpdateCharacterHUD(const bool bForceSpectatorHUD/* = false*/)
{
#if WITH_EDITOR
	if (GEditor && GEditor->bIsSimulatingInEditor)
	{
		// Clean up WaitingForPlayersScreenInstance in SIE
		RemoveWaitingForPlayers();
	}
#endif

	if (ASCGameState* GameState = Cast<ASCGameState>(GetWorld()->GetGameState()))
	{
		// Ensure waiting for players cleans up after the match starts
		if (GameState->IsInPreMatchIntro() || GameState->HasMatchStarted() || GameState->HasMatchEnded())
		{
			RemoveWaitingForPlayers();
		}
	}

	Super::UpdateCharacterHUD(bForceSpectatorHUD);
}

void ASCInGameHUD_Online::RemoveWaitingForPlayers()
{
	if (IsValid(WaitingForPlayersScreenInstance))
	{
		GetMenuStackComp()->RemoveMenuByClass(WaitingForPlayersScreenInstance->GetClass());
		WaitingForPlayersScreenInstance = nullptr;
	}
}
