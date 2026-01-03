// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLLobbyWidget.h"

#include "Online.h"
#include "OnlineSubsystemUtils.h"

#include "ILLGameState.h"
#include "ILLGameState_Lobby.h"
#include "ILLGame_Lobby.h"
#include "ILLHUD_Lobby.h"
#include "ILLGameInstance.h"
#include "ILLPlayerController_Lobby.h"
#include "ILLPlayerState.h"

UILLLobbyWidget::UILLLobbyWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void UILLLobbyWidget::SendLobbyMessage(const FString& Message)
{
	if (AILLPlayerController_Lobby* PC = Cast<AILLPlayerController_Lobby>(GetOwningPlayer()))
	{
		PC->Say(Message);
	}
}

AILLPlayerState* UILLLobbyWidget::GetLobbyHost() const
{
	UWorld* World = GetWorld();
	if (AILLGameState_Lobby* GameState = World ? Cast<AILLGameState_Lobby>(World->GetGameState()) : nullptr)
	{
		return GameState->GetCurrentHost();
	}

	return nullptr;
}

bool UILLLobbyWidget::IsLobbyHost() const
{
	if (AILLPlayerController_Lobby* PC = Cast<AILLPlayerController_Lobby>(GetOwningPlayer()))
	{
		if (AILLPlayerState* PS = Cast<AILLPlayerState>(PC->PlayerState))
		{
			return PS->IsHost();
		}
	}

	return false;
}

void UILLLobbyWidget::ChangeLobbySetting(const FName& Name, const FString& Value)
{
	if (IsLobbyHost())
	{
		if (AILLPlayerController_Lobby* PC = Cast<AILLPlayerController_Lobby>(GetOwningPlayer()))
		{
			PC->ServerChangeLobbySetting(Name, Value);
		}
	}
}

FString UILLLobbyWidget::GetLobbySetting(const FName& Name) const
{
	UWorld* World = GetWorld();
	if (AILLGameState_Lobby* GameState = Cast<AILLGameState_Lobby>(World->GetGameState()))
	{
		return GameState->GetLobbySetting(Name);
	}

	return FString();
}

void UILLLobbyWidget::LobbyTravel(const FString& URL)
{
	if (IsLobbyHost())
	{
		if (AILLPlayerController_Lobby* PC = Cast<AILLPlayerController_Lobby>(GetOwningPlayer()))
		{
			PC->ServerStartLobbyGame(URL);
		}
	}
}

void UILLLobbyWidget::StartTravelCountdown(const int32 Countdown)
{
	if (IsLobbyHost())
	{
		if (AILLPlayerController_Lobby* PC = Cast<AILLPlayerController_Lobby>(GetOwningPlayer()))
		{
			PC->ServerStartTravelCountdown(Countdown);
		}
	}
}

void UILLLobbyWidget::CancelTravelCountdown()
{
	if (IsLobbyHost())
	{
		if (AILLPlayerController_Lobby* PC = Cast<AILLPlayerController_Lobby>(GetOwningPlayer()))
		{
			PC->ServerCancelTravelCountdown();
		}
	}
}
