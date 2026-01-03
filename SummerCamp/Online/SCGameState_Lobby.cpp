// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGameState_Lobby.h"

#include "SCGame_Lobby.h"
#include "SCGameInstance.h"
#include "SCGameSession.h"
#include "SCMapDefinition.h"
#include "SCMapRegistry.h"
#include "SCPlayerController_Lobby.h"
#include "SCPlayerState_Lobby.h"
#include "SCTypes.h"

ASCGameState_Lobby::ASCGameState_Lobby(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	const USCGameInstance* const DefaultGameInstance = USCGameInstance::StaticClass()->GetDefaultObject<USCGameInstance>();
	MapRegistryClass = DefaultGameInstance->MapRegistryClass;

	static ConstructorHelpers::FObjectFinder<USoundCue> LowTravelTickSoundObj(TEXT("/Game/UI/Menu/Sounds/ui_MatchCountdownSingle"));
	LowTravelTickSound = LowTravelTickSoundObj.Object;
}

void ASCGameState_Lobby::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCGameState_Lobby, CurrentMapClass);
	DOREPLIFETIME(ASCGameState_Lobby, bHostCanPickKiller);
	DOREPLIFETIME(ASCGameState_Lobby, RainSetting);
	DOREPLIFETIME(ASCGameState_Lobby, CurrentModeClass);
}

void ASCGameState_Lobby::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	if (Role == ROLE_Authority)
	{
		// Let members know if the host can pick a killer
		UWorld* World = GetWorld();
		ASCGameSession* GS = Cast<ASCGameSession>(World->GetAuthGameMode()->GameSession);
		bHostCanPickKiller = GS->AllowKillerPicking();
	}
}

void ASCGameState_Lobby::HandleMatchHasEnded()
{
	if (Role == ROLE_Authority)
	{
		// Store persistent player data
		USCGameInstance* GI = Cast<USCGameInstance>(GetWorld()->GetGameInstance());
		GI->StoreAllPersistentPlayerData();
	}

	Super::HandleMatchHasEnded();
}

FString ASCGameState_Lobby::GetSelectedLobbyMap() const
{
	const USCMapDefinition* DefaultMapDefinition = nullptr;
	if (CurrentMapClass)
	{
		DefaultMapDefinition = CurrentMapClass->GetDefaultObject<USCMapDefinition>();
	}
	else
	{
		TSubclassOf<USCModeDefinition> ModeClass = USCMapRegistry::FindModeByAlias(MapRegistryClass, GetSelectedGameMode());
		if (TSubclassOf<USCMapDefinition> RandomMapClass = USCMapRegistry::PickRandomMapByMode(MapRegistryClass, ModeClass, nullptr))
		{
			DefaultMapDefinition = RandomMapClass->GetDefaultObject<USCMapDefinition>();
		}
	}

	return DefaultMapDefinition ? DefaultMapDefinition->Path.FilePath : FString();
}

FString ASCGameState_Lobby::GetSelectedGameMode() const
{
	if (CurrentModeClass)
	{
		return CurrentModeClass->GetDefaultObject<USCModeDefinition>()->Alias;
	}
	else
	{
		if (USCMapRegistry* DefaultRegistry = MapRegistryClass ? MapRegistryClass->GetDefaultObject<USCMapRegistry>() : nullptr)
		{
			return DefaultRegistry->QuickPlayGameMode->GetDefaultObject<USCModeDefinition>()->Alias;
		}
	}

	return TEXT("hunt");
}

FString ASCGameState_Lobby::GetLevelLoadOptions() const
{
	FString Options = Super::GetLevelLoadOptions();

	// NOTE: If you add more custom settings, build out the system elsewhere, don't just clog up the lobby game state
	// Random is default, so add nothing if we didn't change our rain value
	if (RainSetting == ESCRainSetting::On)
		Options.Append(TEXT("?rain=on"));
	else if (RainSetting == ESCRainSetting::Off)
		Options.Append(TEXT("?rain=off"));

	return Options;
}

void ASCGameState_Lobby::OnRep_TravelCountdown()
{
	Super::OnRep_TravelCountdown();

	UWorld* World = GetWorld();
	if (World && IsTravelCountdownLow())
	{
		// Play a sound
		if (LowTravelTickSound)
			UGameplayStatics::PlaySound2D(World, LowTravelTickSound);

		// Ensure all local players are staring at the lobby when the countdown is very low
		// Do not do this when travel is impending
		if (TravelCountdown > 0 && !World->IsInSeamlessTravel() && TravelCountdown < VeryLowTravelCountdown)
		{
			for (FLocalPlayerIterator It(GEngine, World); It; ++It)
			{
				if (ASCPlayerController_Lobby* PC = Cast<ASCPlayerController_Lobby>(It->PlayerController))
				{
					PC->EnsureViewingLobby();
				}
			}
		}
	}
}

void ASCGameState_Lobby::TravelCountdownTimer()
{
	const bool bWasLow = IsTravelCountdownLow();

	Super::TravelCountdownTimer();

	const bool bIsLow = IsTravelCountdownLow();
	if (bWasLow != bIsLow)
	{
		if (bIsLow)
		{
			// We just hit the point of no return
			// Ready everybody up by force!
			for (APlayerState* Player : PlayerArray)
			{
				if (ASCPlayerState_Lobby* PS = Cast<ASCPlayerState_Lobby>(Player))
				{
					PS->ForceReady();
				}
			}
		}
	}
}

void ASCGameState_Lobby::SetCurrentMapClass(TSubclassOf<USCMapDefinition> NewMapClass)
{
	check(Role == ROLE_Authority);
	if (CurrentMapClass != NewMapClass)
	{
		CurrentMapClass = NewMapClass;
	}
}

void ASCGameState_Lobby::SetCurrentModeClass(TSubclassOf<USCModeDefinition> NewModeClass)
{
	check(Role == ROLE_Authority);
	if (CurrentModeClass != NewModeClass)
	{
		CurrentModeClass = NewModeClass;
	}
}

void ASCGameState_Lobby::OnRep_GameMode()
{
	OnGameModeUpdated.Broadcast(CurrentModeClass);
}

void ASCGameState_Lobby::SetRainSetting(const ESCRainSetting NewSetting)
{
	RainSetting = NewSetting;
}

void ASCGameState_Lobby::OnRep_RainSetting()
{
	OnRainSettingUpdated.Broadcast(RainSetting);
}

void ASCGameState_Lobby::CopyCustomMatchSettings() {

}
