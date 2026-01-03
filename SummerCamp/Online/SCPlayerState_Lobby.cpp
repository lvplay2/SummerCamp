// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPlayerState_Lobby.h"

#include "ILLGameOnlineBlueprintLibrary.h"

#include "SCGame_Lobby.h"
#include "SCGameInstance.h"
#include "SCGameMode.h"
#include "SCGameSession.h"
#include "SCGameState_Lobby.h"

ASCPlayerState_Lobby::ASCPlayerState_Lobby(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NetUpdateFrequency = 5.f;

	static ConstructorHelpers::FObjectFinder<USoundCue> RemoteReadiedObj(TEXT("/Game/UI/Menu/Sounds/LobbyPlayerReadiedCue"));
	RemoteReadiedSound = RemoteReadiedObj.Object;
	static ConstructorHelpers::FObjectFinder<USoundCue> RemoteUnReadiedObj(TEXT("/Game/UI/Menu/Sounds/LobbyPlayerReadiedCue"));
	RemoteUnReadiedSound = RemoteUnReadiedObj.Object;
}

void ASCPlayerState_Lobby::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCPlayerState_Lobby, bIsReady);
}

bool ASCPlayerState_Lobby::CanChangePickedCharacters() const
{
	if (Super::CanChangePickedCharacters())
	{
		// Lock it out when readied too
		return !bIsReady;
	}

	return false;
}

bool ASCPlayerState_Lobby::CanReady() const
{
	return true;
}

bool ASCPlayerState_Lobby::CanUnReady() const
{
	UWorld* World = GetWorld();
	if (ASCGameState_Lobby* GameState = World->GetGameState<ASCGameState_Lobby>())
	{
		// Allow it when the countdown is low even in LAN/Private sessions
		if (UILLGameOnlineBlueprintLibrary::IsCurrentGameSessionLAN(World) || UILLGameOnlineBlueprintLibrary::IsCurrentGameSessionPrivate(World))
			return true;

		// Otherwise, only allow it when the travel countdown is not low
		if (!GameState->IsTravelCountdownLow())
			return true;
	}

	return false;
}

void ASCPlayerState_Lobby::ForceReady()
{
	check(Role == ROLE_Authority);
	if (!bIsReady)
	{
		bIsReady = true;
		OnRep_bIsReady();
	}
}

bool ASCPlayerState_Lobby::ServerSetReady_Validate(bool bNewReady)
{
	return true;
}

void ASCPlayerState_Lobby::ServerSetReady_Implementation(bool bNewReady)
{
	if (bIsReady)
	{
		if (!CanReady())
			return;
	}
	else if (!CanUnReady())
	{
		return;
	}

	if (bIsReady != bNewReady)
	{
		bIsReady = bNewReady;
		OnRep_bIsReady();

		// Check if enough people have readied to autostart
		if (ASCGame_Lobby* GameMode = GetWorld()->GetAuthGameMode<ASCGame_Lobby>())
		{
			GameMode->CheckAutoStart(ILLLobbyAutoStartReasons::PlayerReady, Cast<AController>(GetOwner()));
		}
	}
}

void ASCPlayerState_Lobby::OnRep_bIsReady()
{
	// Play a sound for remote players
	if (UWorld* World = GetWorld())
	{
		AILLPlayerController* PC = Cast<AILLPlayerController>(GetOwner());
		if (!PC || !PC->IsLocalController())
		{
			if (bIsReady)
			{
				if (RemoteReadiedSound)
					UGameplayStatics::PlaySound2D(World, RemoteReadiedSound);
			}
			else
			{
				if (RemoteUnReadiedSound)
					UGameplayStatics::PlaySound2D(World, RemoteUnReadiedSound);
			}
		}
	}

	PlayerSettingsUpdated();
}

bool ASCPlayerState_Lobby::ServerRequestMapClass_Validate(TSubclassOf<USCMapDefinition> NewMapClass)
{
	return true;
}

void ASCPlayerState_Lobby::ServerRequestMapClass_Implementation(TSubclassOf<USCMapDefinition> NewMapClass)
{
	if (!IsHost())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (ASCGameState_Lobby* GS = World->GetGameState<ASCGameState_Lobby>())
		{
			GS->SetCurrentMapClass(NewMapClass);
		}
	}
}

bool ASCPlayerState_Lobby::ServerRequestModeClass_Validate(TSubclassOf<USCModeDefinition> NewModeClass)
{
	return true;
}

void ASCPlayerState_Lobby::ServerRequestModeClass_Implementation(TSubclassOf<USCModeDefinition> NewModeClass)
{
	if (!IsHost())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (ASCGameState_Lobby* GS = World->GetGameState<ASCGameState_Lobby>())
		{
			GS->SetCurrentModeClass(NewModeClass);
		}
	}
}
