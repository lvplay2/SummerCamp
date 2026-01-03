// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGame_Lobby.h"

#if WITH_EAC
# include "IEasyAntiCheatServer.h"
#endif

#include "SCBackendPlayer.h"
#include "SCHUD_Lobby.h"
#include "SCPlayerController_Lobby.h"
#include "SCPlayerState_Lobby.h"
#include "SCGameInstance.h"
#include "SCGameState_Lobby.h"
#include "SCGameSession.h"
#include "SCLocalPlayer.h"
#include "SCOnlineSessionClient.h"

FName ILLLobbyAutoStartReasons::PlayerReady(TEXT("PlayerReady"));

namespace ILLLobbyAutoStartReasons
{
	FName GameSecondTick(TEXT("GameSecondTick"));
}

ASCGame_Lobby::ASCGame_Lobby(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	HUDClass = ASCHUD_Lobby::StaticClass();
	PlayerControllerClass = ASCPlayerController_Lobby::StaticClass();
	PlayerStateClass = ASCPlayerState_Lobby::StaticClass();
	GameStateClass = ASCGameState_Lobby::StaticClass();

	bUseSeamlessTravel = SC_SEAMLESS_TRAVEL;

	bCanAutoStart = true;
}

TSubclassOf<AGameSession> ASCGame_Lobby::GetGameSessionClass() const
{
	return ASCGameSession::StaticClass();
}

void ASCGame_Lobby::SwapPlayerControllers(APlayerController* OldPC, APlayerController* NewPC)
{
#if WITH_EAC
	// Manually un-register the OldPC from EasyAntiCheat and register the NewPC
	// @see USCGameInstance::Init
	if (Role == ROLE_Authority && IEasyAntiCheatServer::IsAvailable())
	{
		IEasyAntiCheatServer::Get().OnClientDisconnect(OldPC);
		IEasyAntiCheatServer::Get().OnClientConnect(NewPC);
	}
#endif

	Super::SwapPlayerControllers(OldPC, NewPC);
}

void ASCGame_Lobby::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

#if WITH_EAC
	// Manually register with EasyAntiCheat, because the plugin does not and can not handle seamless travel correctly
	// @see USCGameInstance::Init
	if (Role == ROLE_Authority && IEasyAntiCheatServer::IsAvailable())
	{
		IEasyAntiCheatServer::Get().OnClientConnect(NewPlayer);
	}
#endif

	// Infraction related host tracking
	if (ASCPlayerController_Lobby* NewPC = Cast<ASCPlayerController_Lobby>(NewPlayer))
	{
		FILLBackendPlayerIdentifier QuickPlayHostAccountId;
		ASCGameSession* GS = CastChecked<ASCGameSession>(GameSession);
		if (GS->IsQuickPlayMatch())
		{
			if (USCLocalPlayer* FirstLocalPlayer = Cast<USCLocalPlayer>(GetWorld()->GetFirstLocalPlayerFromController()))
			{
				if (FirstLocalPlayer->GetBackendPlayer() && FirstLocalPlayer->PlayerController != NewPC)
					QuickPlayHostAccountId = FirstLocalPlayer->GetBackendPlayer()->GetAccountId();
			}
		}

		NewPC->ClientReceiveP2PHostAccountId(QuickPlayHostAccountId);
	}
}

void ASCGame_Lobby::Logout(AController* Exiting)
{
#if WITH_EAC
	// Manually un-register the OldPC from EasyAntiCheat
	// @see USCGameInstance::Init
	if (Role == ROLE_Authority && IEasyAntiCheatServer::IsAvailable())
	{
		if (APlayerController* PC = Cast<APlayerController>(Exiting))
		{
			IEasyAntiCheatServer::Get().OnClientDisconnect(PC);
		}
	}
#endif

	Super::Logout(Exiting);
}

void ASCGame_Lobby::RestartPlayer(class AController* NewPlayer)
{
	// Don't restart!
}

void ASCGame_Lobby::GameSecondTimer()
{
	Super::GameSecondTimer();

	// Check every tick so we can poll for EAC authentication
	CheckAutoStart(ILLLobbyAutoStartReasons::GameSecondTick);
}

void ASCGame_Lobby::CheckAutoStart(FName AutoStartReason, AController* RelevantController/* = nullptr*/)
{
	// Check auto-start when a player readies, a player logs out, and after seamless traveling
	UWorld* World = GetWorld();
	AILLGameSession* Session = Cast<AILLGameSession>(GameSession);
	ASCGameState_Lobby* State = Cast<ASCGameState_Lobby>(GameState);
	if (World && Session && State)
	{
		int32 TotalPlayers = 0;
		int32 TotalReady = 0;
#if WITH_EAC
		const bool bCheckEAC = IEasyAntiCheatServer::Get().IsEnforcementEnabled();
#endif
		for (APlayerState* Player : State->PlayerArray)
		{
			if (!IsValid(Player))
				continue;

			AILLPlayerController* PlayerOwner = Cast<AILLPlayerController>(Player->GetOwner());
			if (AutoStartReason == ILLLobbyAutoStartReasons::Logout && PlayerOwner == RelevantController)
			{
				// Skip the player logging out
				continue;
			}

			if (ASCPlayerState_Lobby* LobbyPlayer = Cast<ASCPlayerState_Lobby>(Player))
			{
				++TotalPlayers;
				if (LobbyPlayer->IsReady())
				{
#if WITH_EAC
					// Wait for them to be authenticated with EAC before considering them ready
					const bool bDifferentPlatform = (PlayerOwner->LinkedLobbyClient.IsValid() && PlayerOwner->LinkedLobbyClient->GetAuthorizationType() != TEXT("steam"));
					if (!bCheckEAC || bDifferentPlatform || IEasyAntiCheatServer::Get().IsClientAuthenticated(PlayerOwner))
#endif
					++TotalReady;
				}
			}
		}

		// Incorporate pending players into TotalPlayers
		USCGameInstance* GI = CastChecked<USCGameInstance>(World->GetGameInstance());
		USCOnlineSessionClient* OSC = CastChecked<USCOnlineSessionClient>(GI->GetOnlineSession());
		TotalPlayers = FMath::Max(TotalPlayers, OSC->GetLobbyMemberList().Num());

		if (TotalReady > 1 && TotalReady == TotalPlayers)
		{
			// Everybody is ready, start the AutoStartCountdownAllReady
			State->AutoStartTravelCountdown(AutoStartCountdownAllReady);
			return;
		}

		// Not everybody is ready, cancel the countdown for LAN/Private matches
		if (Session->IsLANMatch() || Session->IsPrivateMatch())
		{
			State->CancelAutoStartCountdown();
			return;
		}
	}

	if (AutoStartReason != ILLLobbyAutoStartReasons::GameSecondTick)
	{
		Super::CheckAutoStart(AutoStartReason, RelevantController);
	}
}
