// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLHUD_Lobby.h"

#include "ILLGameState_Lobby.h"
#include "ILLLobbyWidget.h"
#include "ILLPlayerState.h"

AILLHUD_Lobby::AILLHUD_Lobby(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void AILLHUD_Lobby::Destroyed()
{
	Super::Destroyed();

	HideLobbyUI();
}

void AILLHUD_Lobby::BeginPlay()
{
	Super::BeginPlay();

	// Spawn the UI
	if (UWorld* World = GetWorld())
	{
		SpawnLobbyWidget();
	}
}

void AILLHUD_Lobby::HideLobbyUI()
{
	if (UILLLobbyWidget* LobbyWidgetInstance = GetLobbyWidget())
	{
		LobbyWidgetInstance->RemoveFromParent();
		LobbyWidgetInstance->MarkPendingKill();
		LobbyWidgetInstance = nullptr;
	}
}

void AILLHUD_Lobby::LobbySettingsUpdated()
{
	if (UILLLobbyWidget* LobbyWidgetInstance = GetLobbyWidget())
	{
		LobbyWidgetInstance->LobbySettingsUpdated();
	}
}

void AILLHUD_Lobby::PlayerMessage(AILLPlayerState* Sender, const FString& Message)
{
	if (UILLLobbyWidget* LobbyWidgetInstance = GetLobbyWidget())
	{
		LobbyWidgetInstance->LobbyPlayerMessage(Sender, Message);
	}
}

void AILLHUD_Lobby::TravelCountdownStarted(const int32 TravelCountdown)
{
	if (UILLLobbyWidget* LobbyWidgetInstance = GetLobbyWidget())
	{
		LobbyWidgetInstance->LobbyTravelCountdownStarted(TravelCountdown);
	}
}

void AILLHUD_Lobby::TravelCountdownCancelled()
{
	if (UILLLobbyWidget* LobbyWidgetInstance = GetLobbyWidget())
	{
		LobbyWidgetInstance->LobbyTravelCountdownCancelled();
	}
}
