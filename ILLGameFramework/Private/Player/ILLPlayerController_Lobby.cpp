// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLPlayerController_Lobby.h"

#include "ILLGameState_Lobby.h"
#include "ILLGame_Lobby.h"
#include "ILLHUD_Lobby.h"
#include "ILLPlayerState.h"

AILLPlayerController_Lobby::AILLPlayerController_Lobby(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void AILLPlayerController_Lobby::ClientTeamMessage_Implementation(APlayerState* SenderPlayerState, const FString& S, FName Type, float MsgLifeTime)
{
	if (Type == NAME_Say)
	{
		if (AILLHUD_Lobby* HUD = Cast<AILLHUD_Lobby>(GetHUD()))
		{
			if (AILLPlayerState* Sender = Cast<AILLPlayerState>(SenderPlayerState))
				HUD->PlayerMessage(Sender, S);
		}
	}

	Super::ClientTeamMessage_Implementation(SenderPlayerState, S, Type, MsgLifeTime);
}

void AILLPlayerController_Lobby::ServerChangeLobbySetting_Implementation(const FName& Name, const FString& Value)
{
	if (GetWorld())
	{
		if (AILLGameState_Lobby* GameState = Cast<AILLGameState_Lobby>(GetWorld()->GetGameState()))
		{
			GameState->ChangeLobbySetting(Name, Value);
		}
	}
}

bool AILLPlayerController_Lobby::ServerChangeLobbySetting_Validate(const FName& Name, const FString& Value)
{
	return true;
}

void AILLPlayerController_Lobby::ServerStartLobbyGame_Implementation(const FString& URL)
{
	GetWorld()->ServerTravel(URL, true);
}

bool AILLPlayerController_Lobby::ServerStartLobbyGame_Validate(const FString& URL)
{
	return true;
}

void AILLPlayerController_Lobby::ServerStartTravelCountdown_Implementation(const int32 Countdown)
{
	if (AILLPlayerState* PS = Cast<AILLPlayerState>(PlayerState))
	{
		if (PS->IsHost())
		{
			if (AILLGameState_Lobby* GameState = Cast<AILLGameState_Lobby>(GetWorld()->GetGameState()))
			{
				GameState->StartTravelCountdown(Countdown);
			}
		}
	}
}

bool AILLPlayerController_Lobby::ServerStartTravelCountdown_Validate(const int32 Countdown)
{
	return true;
}

void AILLPlayerController_Lobby::ServerCancelTravelCountdown_Implementation()
{
	if (AILLPlayerState* PS = Cast<AILLPlayerState>(PlayerState))
	{
		if (PS->IsHost())
		{
			if (AILLGameState_Lobby* GameState = Cast<AILLGameState_Lobby>(GetWorld()->GetGameState()))
			{
				GameState->CancelTravelCountdown();
			}
		}
	}
}

bool AILLPlayerController_Lobby::ServerCancelTravelCountdown_Validate()
{
	return true;
}

void AILLPlayerController_Lobby::ClientStartTravelCountdown_Implementation(const int32 TravelCountdown)
{
	if (AILLHUD_Lobby* LobbyHUD = Cast<AILLHUD_Lobby>(GetHUD()))
	{
		LobbyHUD->TravelCountdownStarted(TravelCountdown);
	}
}

void AILLPlayerController_Lobby::ClientCancelledTravelCountdown_Implementation()
{
	if (AILLHUD_Lobby* LobbyHUD = Cast<AILLHUD_Lobby>(GetHUD()))
	{
		LobbyHUD->TravelCountdownCancelled();
	}
}
