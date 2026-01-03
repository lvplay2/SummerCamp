// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLGameState_Lobby.h"

#include "ILLGameBlueprintLibrary.h"
#include "ILLHUD_Lobby.h"
#include "ILLGame_Lobby.h"
#include "ILLGameInstance.h"
#include "ILLGameSession.h"
#include "ILLPlayerController_Lobby.h"
#include "ILLPlayerState.h"

AILLGameState_Lobby::AILLGameState_Lobby(const FObjectInitializer& ObjectInitializer) 
: Super(ObjectInitializer)
{
	NetUpdateFrequency = 5.f;
}

void AILLGameState_Lobby::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AILLGameState_Lobby, LobbySettings);
	DOREPLIFETIME(AILLGameState_Lobby, TravelCountdown);
	DOREPLIFETIME(AILLGameState_Lobby, bAutoTravelCountdown);
	DOREPLIFETIME(AILLGameState_Lobby, bTravelCountingDown);
}

void AILLGameState_Lobby::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();

	if (Role != ROLE_Authority || bPendingInstanceRecycle)
		return;

	LobbySettingsUpdated();

	// Make sure to clear the timer
	GetWorldTimerManager().ClearTimer(TimerHandle_TravelCountdown);

	FString MapPath = GetSelectedLobbyMap();
	FString GameModeName = GetSelectedGameMode();
	FString LevelLoadOptions = GetLevelLoadOptions();

	UILLGameBlueprintLibrary::ShowLoadingAndTravel(this, MapPath, true, *FString::Printf(TEXT("game=%s%s"), *GameModeName, *LevelLoadOptions));
}

void AILLGameState_Lobby::ChangeLobbySetting(const FName& Name, const FString& Value)
{
	bool bFound = false;
	bool bChanged = false;
	for (FILLLobbySetting& Setting : LobbySettings)
	{
		if (Setting.Name == Name)
		{
			if (Setting.Value != Value)
			{
				Setting.Value = Value;
				bChanged = true;
			}

			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		FILLLobbySetting NewSetting;
		NewSetting.Name = Name;
		NewSetting.Value = Value;
		LobbySettings.Push(NewSetting);

		bChanged = true;
	}

	if (bChanged)
	{
		// Simulate the modification notification
		LobbySettingsUpdated();
	}
}

FString AILLGameState_Lobby::GetLobbySetting(const FName& Name) const
{
	for (const FILLLobbySetting& Setting : LobbySettings)
	{
		if (Setting.Name == Name)
		{
			return Setting.Value;
		}
	}

	return FString();
}

void AILLGameState_Lobby::LobbySettingsUpdated()
{
	if (Role == ROLE_Authority)
	{
		// Force a replication update immediately
		ForceNetUpdate();
	}

	// Update any local player HUDs
	for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
	{
		if (APlayerController* PC = It->PlayerController)
		{
			if (AILLHUD_Lobby* LobbyHUD = Cast<AILLHUD_Lobby>(PC->GetHUD()))
			{
				LobbyHUD->LobbySettingsUpdated();
			}
		}
	}
}

void AILLGameState_Lobby::OnRep_LobbySettings()
{
	LobbySettingsUpdated();
}

void AILLGameState_Lobby::AutoStartTravelCountdown(const int Countdown, const bool bAllowAutoTimerIncrease/* = false*/)
{
	// Only accept new Countdown updates when they are shorter and we are already in an automatic countdown
	// This allows a shorter countdown to kick in as lobbies grow
	if (!bTravelCountingDown || (bAutoTravelCountdown && (bAllowAutoTimerIncrease || Countdown < TravelCountdown)))
	{
		StartTravelCountdown(Countdown);
		bAutoTravelCountdown = true;
	}
}

void AILLGameState_Lobby::CancelAutoStartCountdown()
{
	CancelTravelCountdown();
}

void AILLGameState_Lobby::StartTravelCountdown(const int Countdown)
{
	check(Role == ROLE_Authority);

	if (bTravelCountingDown)
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_TravelCountdown);
	}

	if (Countdown < 0)
	{
		GetWorld()->GetAuthGameMode<AILLGameMode>()->EndMatch();
	}
	else
	{
		bAutoTravelCountdown = false;
		bTravelCountingDown = true;
		TravelCountdown = FMath::Max(Countdown, 1);
		GetWorldTimerManager().SetTimer(TimerHandle_TravelCountdown, this, &AILLGameState_Lobby::TravelCountdownTimer, GetWorldSettings()->GetEffectiveTimeDilation(), true);

		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (AILLPlayerController_Lobby* PC = Cast<AILLPlayerController_Lobby>(*It))
			{
				PC->ClientStartTravelCountdown(TravelCountdown);
			}
		}
	}

	LobbySettingsUpdated();
}

void AILLGameState_Lobby::CancelTravelCountdown()
{
	check(Role == ROLE_Authority);

	if (bTravelCountingDown)
	{
		bAutoTravelCountdown = false;
		bTravelCountingDown = false;
		GetWorldTimerManager().ClearTimer(TimerHandle_TravelCountdown);
		
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (AILLPlayerController_Lobby* PC = Cast<AILLPlayerController_Lobby>(*It))
			{
				PC->ClientCancelledTravelCountdown();
			}
		}

		LobbySettingsUpdated();
	}
}

void AILLGameState_Lobby::OnRep_TravelCountdown()
{
	LobbySettingsUpdated();
}

void AILLGameState_Lobby::TravelCountdownTimer()
{
	check(Role == ROLE_Authority);
	check(bTravelCountingDown);

	if (TravelCountdown > 0)
	{
		--TravelCountdown;
		OnRep_TravelCountdown();
	}

	if (TravelCountdown == 0)
	{
		GetWorld()->GetAuthGameMode<AILLGameMode>()->EndMatch();
	}
}

FORCEINLINE bool IsMapOrModeStringRandom(const FString& String)
{
	return (String.IsEmpty() || String == TEXT("Rndm") || String == TEXT("Random"));
}

FString AILLGameState_Lobby::GetLevelLoadOptions() const
{
	return Cast<AILLGameSession>(GetWorld()->GetAuthGameMode()->GameSession)->GetPersistentTravelOptions();
}
