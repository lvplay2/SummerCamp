// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLGameSession.h"

#include "Net/OnlineEngineInterface.h"
#include "OnlineSubsystem.h"
#include "OnlineIdentityInterface.h"
#include "OnlineSessionInterface.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"

#include "ILLBackendRequestManager.h"
#include "ILLBackendServer.h"
#include "ILLBuildDefines.h"
#include "ILLGameInstance.h"
#include "ILLGameMode.h"
#include "ILLOnlineSessionClient.h"
#include "ILLOnlineSessionSearch.h"
#include "ILLPlayerController.h"

AILLGameSession::AILLGameSession(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void AILLGameSession::InitOptions(const FString& Options)
{
	Super::InitOptions(Options);

	// Store off if this is a LAN match
	bIsLanMatch = UGameplayStatics::HasOption(Options, TEXT("bIsLanMatch"));
	bIsPrivateMatch = UGameplayStatics::HasOption(Options, TEXT("bIsPrivateMatch"));

	// Accept a command-line password override
	FParse::Value(FCommandLine::Get(), TEXT("ServerPassword="), ServerPassword);
}

FString AILLGameSession::ApproveLogin(const FString& Options)
{
	FString ErrorMessage = Super::ApproveLogin(Options);

	if (ErrorMessage.IsEmpty())
	{
		// Check the ServerPassword
		if (!ServerPassword.IsEmpty())
		{
			const FString UserPassword = UGameplayStatics::ParseOption(Options, TEXT("Password"));
			if (UserPassword.IsEmpty())
			{
				return TEXT("Password missing.");
			}
			else if (ServerPassword.Compare(UserPassword, ESearchCase::CaseSensitive) != 0)
			{
				return TEXT("Password mismatch.");
			}
		}
	}

	return ErrorMessage;
}

void AILLGameSession::RegisterPlayer(APlayerController* NewPlayer, const FUniqueNetIdRepl& UniqueId, bool bWasFromInvite)
{
	// Only defer game session registration when we know we are going to need one
	// We need to register them in offline modes with no game session too so that player states get their UniqueId
	bool bDeferRegistrationForGameSession = false;
	if (GetNetMode() == NM_DedicatedServer || GetNetMode() == NM_ListenServer)
	{
		IOnlineSessionPtr Sessions = Online::GetSessionInterface();
		if (Sessions.IsValid())
			bDeferRegistrationForGameSession = (Sessions->GetSessionState(NAME_GameSession) != EOnlineSessionState::InProgress);
	}

	if (bDeferRegistrationForGameSession)
	{
		FILLPlayerSessionRegistration Entry;
		Entry.Player = NewPlayer;
		Entry.UniqueId = UniqueId;
		Entry.bFromInvite = bWasFromInvite;
		PlayersPendingRegistration.Add(Entry);
	}
	else
	{
		Super::RegisterPlayer(NewPlayer, UniqueId, bWasFromInvite);
	}
}

void AILLGameSession::UnregisterPlayer(FName InSessionName, const FUniqueNetIdRepl& UniqueId)
{
	// NOTE: Intentionally does not make the Super call. UE4 does this in a very odd way. @see AILLPlayerState::UnregisterPlayerWithSession
}

bool AILLGameSession::AtCapacity(bool bSpectator)
{
	if (GetNetMode() == NM_Standalone)
	{
		return false;
	}

	UWorld* World = GetWorld();
	AILLGameMode* GameMode = World->GetAuthGameMode<AILLGameMode>();
	if (bSpectator)
	{
		// Check the spectator limit
		// When a listen server, wait for the host to be loaded first
		return (GameMode->NumSpectators >= MaxSpectators && (GetNetMode() != NM_ListenServer || GameMode->GetNumPlayers() > 0));
	}

	// Determine the player limit
	int32 MaxPlayersToUse = GetMaxPlayers();
#if !UE_BUILD_SHIPPING
	const auto CVarMaxPlayersOverride = IConsoleManager::Get().FindConsoleVariable(TEXT("net.MaxPlayersOverride"));
	if (CVarMaxPlayersOverride->GetInt() > 0)
		MaxPlayersToUse = CVarMaxPlayersOverride->GetInt();
#endif
	if (MaxPlayersToUse <= 0)
	{
		return false;
	}

	// Check for enough room, including the NumPendingConnections (main reason for override)
	const int32 NumPlayers = GameMode->GetNumPlayers();
	const int32 NumPendingConnections = FMath::Max(GameMode->GetNumPendingConnections(false)-1, 0); // At this point the user making the query will count, so account for that...
	return (NumPlayers+NumPendingConnections >= MaxPlayersToUse);
}

bool AILLGameSession::KickPlayer(APlayerController* KickedPlayer, const FText& KickReason)
{
	// Do not kick ourselves, check for a UNetConnection
	if (KickedPlayer && Cast<UNetConnection>(KickedPlayer->Player))
	{
		// Notify them they were kicked
		KickedPlayer->ClientWasKicked(KickReason);

		// Cleanup the PlayerController
		KickedPlayer->PawnLeavingGame();
		KickedPlayer->Destroy();
		KickedPlayer->ForceNetUpdate();
		return true;
	}

	return false;
}

void AILLGameSession::HandleMatchIsWaitingToStart()
{
	Super::HandleMatchIsWaitingToStart();

#if STATS && !UE_BUILD_SHIPPING
	if (GetNetMode() == NM_DedicatedServer || GetNetMode() == NM_ListenServer)
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("MatchAutoNetProfile")))
		{
			UE_LOG(LogOnlineGame, Log, TEXT("Match has started - begin automatic network profiling"));
			GEngine->Exec(GetWorld(), TEXT("netprofile enable"));
		}
	}
#endif
}

void AILLGameSession::HandleMatchHasStarted()
{
	// SKIP THE SUPER CALL: Starts our game session even if it is already started, causing unnecessary errors.
	// Also starts the game session, which is a dumb spot. @see AILLGameSession::OnCreateGameSessionComplete for that.

	// This is copied from the Super function
	if (STATS && !UE_BUILD_SHIPPING)
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("MatchAutoStatCapture")))
		{
			UE_LOG(LogOnlineGame, Log, TEXT("Match has started - begin automatic stat capture"));
			GEngine->Exec(GetWorld(), TEXT("stat startfile"));
		}
	}
}

void AILLGameSession::HandleMatchHasEnded()
{
	// SKIP THE SUPER CALL: Ends our game session, which we do not want
	// This is copied from the Super function
	if (STATS && !UE_BUILD_SHIPPING)
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("MatchAutoStatCapture")))
		{
			UE_LOG(LogOnlineGame, Log, TEXT("Match has ended - end automatic stat capture"));
			GEngine->Exec(GetWorld(), TEXT("stat stopfile"));
		}
	}
}

void AILLGameSession::FlushPlayersPendingRegistration()
{
	for (const FILLPlayerSessionRegistration& Entry : PlayersPendingRegistration)
	{
		RegisterPlayer(Entry.Player, Entry.UniqueId, Entry.bFromInvite);
	}
	PlayersPendingRegistration.Empty();
}

FString AILLGameSession::GetPersistentTravelOptions() const
{
	FString LevelOptions;

	if (IsListenSession())
		LevelOptions += TEXT("?listen");

	if (IsLANMatch())
		LevelOptions += TEXT("?bIsLanMatch");

	if (IsPrivateMatch())
		LevelOptions += TEXT("?bIsPrivateMatch");

	return LevelOptions;
}

bool AILLGameSession::IsReadyToStartMatch()
{
	UWorld* World = GetWorld();
#if WITH_EDITOR
	if (World && World->IsPlayInEditor())
	{
		return true;
	}
#endif

	IOnlineSessionPtr Sessions = Online::GetSessionInterface();
	if (Sessions.IsValid())
	{
		const EOnlineSessionState::Type CurrentSessionState = Sessions->GetSessionState(NAME_GameSession);

		// When standalone we expect no session
		if (World->GetNetMode() == NM_Standalone)
		{
			return (CurrentSessionState == EOnlineSessionState::NoSession);
		}

		return (CurrentSessionState == EOnlineSessionState::InProgress || CurrentSessionState == EOnlineSessionState::Pending);
	}

	return false;
}

void AILLGameSession::HandleLeavingMap()
{
#if STATS && !UE_BUILD_SHIPPING
	if (GetNetMode() == NM_DedicatedServer || GetNetMode() == NM_ListenServer)
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("MatchAutoNetProfile")))
		{
			UE_LOG(LogOnlineGame, Log, TEXT("Match has started - begin automatic network profiling"));
			GEngine->Exec(GetWorld(), TEXT("netprofile disable"));
		}
	}
#endif
}

TSharedPtr<FILLGameSessionSettings> AILLGameSession::GenerateHostSettings() const
{
	return MakeShareable(new FILLGameSessionSettings(IsLANMatch(), GetMaxPlayers(), !IsPrivateMatch(), IsPassworded()));
}
