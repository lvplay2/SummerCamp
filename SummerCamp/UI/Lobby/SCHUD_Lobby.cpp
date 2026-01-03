// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCHUD_Lobby.h"

#include "SCGameInstance.h"
#include "SCMatchEndLobbyWidget.h"
#include "SCLobbyWidget.h"
#include "SCPlayerController_Lobby.h"
#include "SCPlayerState_Lobby.h"
#include "SCMenuStackHUDComponent.h"

ASCHUD_Lobby::ASCHUD_Lobby(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	SoftLobbyWidgetClass = FSoftObjectPath(TEXT("SCLobbyWidget'/Game/UI/Lobby/LobbyWidget.LobbyWidget_C'"));

	MenuStackComponent = CreateDefaultSubobject<USCMenuStackHUDComponent>(MenuStackCompName);
}

void ASCHUD_Lobby::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		// Hide the loading screen
		if (USCGameInstance* GameInstance = Cast<USCGameInstance>(World->GetGameInstance()))
		{
			GameInstance->HideLoadingScreen();
		}

		// Load the lobby widget
		// Super call must be before this!
		GetLobbyWidget();
	}
}

void ASCHUD_Lobby::HideLobbyUI()
{
	// We call ChangeRootMenu instead of PopMenu in case other menus were pushed over the lobby
	MenuStackComponent->ChangeRootMenu(nullptr, true);
}

void ASCHUD_Lobby::SpawnLobbyWidget()
{
	if (!HardLobbyWidgetClass)
		HardLobbyWidgetClass = SoftLobbyWidgetClass.LoadSynchronous();
	MenuStackComponent->ChangeRootMenu(HardLobbyWidgetClass);
}

UILLLobbyWidget* ASCHUD_Lobby::GetLobbyWidget() const
{
	// Do not create the HUD until we BeginPlay, so that we wait for the loading screen to go away
	if (HasActorBegunPlay() && HardLobbyWidgetClass)
	{
		return Cast<UILLLobbyWidget>(MenuStackComponent->GetOpenMenuByClass(HardLobbyWidgetClass));
	}

	return nullptr;
}

void ASCHUD_Lobby::ForceLobbyUI()
{
	// When the Lobby is not open, force it up
	UILLUserWidget* CurrentMenu = MenuStackComponent->GetCurrentMenu();
	if (!Cast<USCLobbyWidget>(CurrentMenu))
	{
		if (!HardLobbyWidgetClass)
			HardLobbyWidgetClass = SoftLobbyWidgetClass.LoadSynchronous();
		MenuStackComponent->ChangeRootMenu(HardLobbyWidgetClass);
	}
}
