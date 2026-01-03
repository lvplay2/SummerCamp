// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGameMode_Offline.h"

#include "ILLGameBlueprintLibrary.h"

ASCGameMode_Offline::ASCGameMode_Offline(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bPauseable = true;
}

void ASCGameMode_Offline::RestartGame()
{
	// Do not call the Super!
	UWorld* World = GetWorld();
	if (World->IsPlayInEditor())
	{
		return;
	}
	if (GetMatchState() == MatchState::LeavingMap)
	{
		return;
	}

	// Return to the main menu
	UILLGameBlueprintLibrary::ShowLoadingAndReturnToMainMenu(UILLGameBlueprintLibrary::GetFirstLocalPlayerController(GetWorld()), FText());
}

bool ASCGameMode_Offline::CanCompleteWaitingPreMatch() const
{
	// we're offline, just doo eet.
	return true;
}
