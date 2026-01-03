// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLGame_Lobby.h"

#include "ILLPlayerState.h"
#include "ILLGameSession.h"
#include "ILLGameState_Lobby.h"
#include "ILLHUD_Lobby.h"
#include "ILLPlayerController_Lobby.h"

FName ILLLobbyAutoStartReasons::PostBeaconLogin(TEXT("PostBeaconLogin"));
FName ILLLobbyAutoStartReasons::PostLogin(TEXT("PostLogin"));
FName ILLLobbyAutoStartReasons::Logout(TEXT("Logout"));
FName ILLLobbyAutoStartReasons::PostSeamlessTravel(TEXT("PostSeamlessTravel"));

AILLGame_Lobby::AILLGame_Lobby(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	HUDClass = AILLHUD_Lobby::StaticClass();
	PlayerControllerClass = AILLPlayerController_Lobby::StaticClass();
	GameStateClass = AILLGameState_Lobby::StaticClass();

	bStartPlayersAsSpectators = true;

	AutoStartCountdown = 10;
	MinAutoStartCountdownOnBeaconJoin = 10;
	MinAutoStartCountdownOnJoin = 10;

	ModeName = TEXT("Lobby");
}

void AILLGame_Lobby::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// Check if we have enough players to auto-start
	CheckAutoStart(ILLLobbyAutoStartReasons::PostLogin, NewPlayer);
}

void AILLGame_Lobby::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	// Check if we should cancel auto-start
	CheckAutoStart(ILLLobbyAutoStartReasons::Logout, Exiting);
}

void AILLGame_Lobby::PostSeamlessTravel()
{
	Super::PostSeamlessTravel();

	// Check if we should trigger an auto-start
	CheckAutoStart(ILLLobbyAutoStartReasons::PostSeamlessTravel);
}

void AILLGame_Lobby::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();

	// Automatically start the recycling process
	AILLGameState_Lobby* State = Cast<AILLGameState_Lobby>(GameState);
	if (State && State->IsPendingInstanceRecycle())
	{
		RestartGame();
	}
}

void AILLGame_Lobby::AddLobbyMemberState(AILLLobbyBeaconMemberState* MemberState)
{
	Super::AddLobbyMemberState(MemberState);

	CheckAutoStart(ILLLobbyAutoStartReasons::PostBeaconLogin);
}

void AILLGame_Lobby::RemoveLobbyMemberState(AILLLobbyBeaconMemberState* MemberState)
{
	Super::RemoveLobbyMemberState(MemberState);
}

void AILLGame_Lobby::CheckAutoStart(FName AutoStartReason, AController* RelevantController/* = nullptr*/)
{
	AILLGameState_Lobby* State = Cast<AILLGameState_Lobby>(GameState);

	int32 MinimumTime = -1;
	if (AutoStartReason == ILLLobbyAutoStartReasons::PostBeaconLogin)
		MinimumTime = MinAutoStartCountdownOnBeaconJoin;
	else if (AutoStartReason == ILLLobbyAutoStartReasons::PostLogin)
		MinimumTime = MinAutoStartCountdownOnJoin;

	if (MinimumTime > 0 && State && State->IsAutoTravelCountdownActive() && State->GetTravelCountdown() < MinimumTime)
	{
		// Bump the previous auto-travel countdown timer up to MinimumTime when someone joins and it is lower than that
		State->AutoStartTravelCountdown(MinimumTime, true);

		// Update the game session immediately
		QueueGameSessionUpdate();
	}

	if (!bCanAutoStart)
	{
		return;
	}

	AILLGameSession* Session = Cast<AILLGameSession>(GameSession);
	if (Session && State)
	{
		const int32 TotalPlayers = GetNumPlayers();
		if (TotalPlayers <= FMath::Max(0, CancelAutoStartMinPlayers))
		{
			// Cancel a previous auto-start if started
			State->CancelAutoStartCountdown();

			// Update the game session immediately
			QueueGameSessionUpdate();
		}
		else
		{
			const bool bIsLANPrivate = (Session->IsLANMatch() || Session->IsPrivateMatch());
			auto PickCountdown = [&](const float NormalDuration, const float LANPrivateDurationOverride) -> float
			{
				if (bIsLANPrivate && LANPrivateDurationOverride != 0)
				{
					return LANPrivateDurationOverride;
				}

				return NormalDuration;
			};

			// Determine the CountdownDuration
			int32 CountdownDuration = 0;
			if (TotalPlayers == 1)
			{
				// Solo auto-start
				CountdownDuration = PickCountdown(AutoStartCountdownSolo, AutoStartCountdownLANPrivateSolo);
			}
			else if (TotalPlayers == Session->GetMaxPlayers())
			{
				// Full lobby auto-start
				CountdownDuration = PickCountdown(AutoStartCountdownFull, AutoStartCountdownLANPrivateFull);
			}
			else
			{
				// Check for a minimum player auto-start
				int32 MinPlayers = AutoStartNumPlayers;
				if (AutoStartNumLANPrivatePlayers != 0 && bIsLANPrivate)
				{
					MinPlayers = AutoStartNumLANPrivatePlayers;
				}

				// MinPlayers <=0 means no small-smarts
				if (MinPlayers > 0 && TotalPlayers >= MinPlayers)
				{
					CountdownDuration = PickCountdown(AutoStartCountdown, AutoStartCountdownLANPrivate);
				}
			}

			if (CountdownDuration > 0)
			{
				// Automatically start the game timer countdown once we hit capacity
				State->AutoStartTravelCountdown(CountdownDuration);

				// Update the game session immediately
				QueueGameSessionUpdate();
			}
		}
	}
}
