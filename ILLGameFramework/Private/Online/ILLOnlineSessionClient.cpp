// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLOnlineSessionClient.h"

#include "OnlineSubsystemSessionSettings.h"
#include "OnlineSubsystemUtils.h"

#if USE_GAMELIFT_SERVER_SDK
# include "GameLiftServerSDK.h"
#endif

#include "ILLBackendBlueprintLibrary.h"
#include "ILLBackendPlayer.h"
#include "ILLBackendRequestManager.h"
#include "ILLBackendServer.h"

#include "ILLBuildDefines.h"
#include "ILLGameBlueprintLibrary.h"
#include "ILLGameEngine.h"
#include "ILLGameInstance.h"
#include "ILLGameMode.h"
#include "ILLGameOnlineBlueprintLibrary.h"
#include "ILLGameSession.h"
#include "ILLLocalPlayer.h"
#include "ILLMatchmakingBlueprintLibrary.h"
#include "ILLOnlineMatchmakingClient.h"
#include "ILLPlayerController.h"

#include "ILLLobbyBeaconClient.h"
#include "ILLLobbyBeaconHost.h"
#include "ILLLobbyBeaconHostObject.h"
#include "ILLLobbyBeaconMemberState.h"
#include "ILLLobbyBeaconState.h"

#include "ILLPartyBlueprintLibrary.h"
#include "ILLPartyBeaconClient.h"
#include "ILLPartyBeaconHost.h"
#include "ILLPartyBeaconHostObject.h"
#include "ILLPartyBeaconMemberState.h"
#include "ILLPartyBeaconState.h"

#define LOCTEXT_NAMESPACE "ILLFonic.IGF.ILLOnlineSessionClient"

FText EOnJoinSessionCompleteResult::GetLocalizedDescription(FName SessionName, const EOnJoinSessionCompleteResult::Type Result)
{
	if (SessionName == NAME_PartySession)
	{
		switch (Result)
		{
		case EOnJoinSessionCompleteResult::Success: return LOCTEXT("JoinPartySession_Success", "Successfully joined party session.");
		case EOnJoinSessionCompleteResult::SessionIsFull: return LOCTEXT("JoinPartySession_SessionIsFull", "Party session is full!");
		case EOnJoinSessionCompleteResult::SessionDoesNotExist: return LOCTEXT("JoinPartySession_SessionDoesNotExist", "Party session does not exist!");
		case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress: return LOCTEXT("JoinPartySession_CouldNotRetrieveAddress", "Could not retrieve party connection details!");
		case EOnJoinSessionCompleteResult::AlreadyInSession: return LOCTEXT("JoinPartySession_AlreadyInSession", "Already in that party session!");
		case EOnJoinSessionCompleteResult::PlayOnlinePrivilege: return LOCTEXT("JoinPartySession_PlayOnlinePrivilege", "Party join failed! Your account lacks play online privileges!");
		case EOnJoinSessionCompleteResult::AuthFailed: return LOCTEXT("JoinPartySession_AuthFailed", "Party join failed! Authorization failure!");
		case EOnJoinSessionCompleteResult::AuthTimedOut: return LOCTEXT("JoinPartySession_AuthTimedOut", "Party join failed! Authorization timed out!");
		case EOnJoinSessionCompleteResult::BackendAuthFailed: return LOCTEXT("JoinPartySession_BackendAuthFailed", "Party join failed! Backend authorization failure!");
		case EOnJoinSessionCompleteResult::UnknownError: return LOCTEXT("JoinPartySession_UnknownError", "Unknown error while joining party session!");
		}

		return FText::Format(LOCTEXT("JoinPartySession_Failure", "Error {0} while joining party session!"), FText::FromString(EOnJoinSessionCompleteResult::ToString(Result)));
	}

	if (SessionName == NAME_GameSession)
	{
		switch (Result)
		{
		case EOnJoinSessionCompleteResult::Success: return LOCTEXT("JoinGameSession_Success", "Successfully joined game session.");
		case EOnJoinSessionCompleteResult::SessionIsFull: return LOCTEXT("JoinGameSession_SessionIsFull", "Game session is full!");
		case EOnJoinSessionCompleteResult::SessionDoesNotExist: return LOCTEXT("JoinGameSession_SessionDoesNotExist", "Game session does not exist!");
		case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress: return LOCTEXT("JoinGameSession_CouldNotRetrieveAddress", "Could not retrieve game connection details!");
		case EOnJoinSessionCompleteResult::AlreadyInSession: return LOCTEXT("JoinGameSession_AlreadyInSession", "Already in that game session!");
		case EOnJoinSessionCompleteResult::PlayOnlinePrivilege: return LOCTEXT("JoinGameSession_PlayOnlinePrivilege", "Game join failed! Your account lacks play online privileges!");
		case EOnJoinSessionCompleteResult::AuthFailed: return LOCTEXT("JoinGameSession_AuthFailed", "Game join failed! Authorization failure!");
		case EOnJoinSessionCompleteResult::AuthTimedOut: return LOCTEXT("JoinGameSession_AuthTimedOut", "Game join failed! Authorization timed out!");
		case EOnJoinSessionCompleteResult::BackendAuthFailed: return LOCTEXT("JoinGameSession_BackendAuthFailed", "Game join failed! Backend authorization failure!");
		case EOnJoinSessionCompleteResult::UnknownError: return LOCTEXT("JoinGameSession_UnknownError", "Unknown error while joining game session!");
		}
	}

	return FText::Format(LOCTEXT("JoinGameSession_Failure", "Error {0} while joining session!"), FText::FromString(EOnJoinSessionCompleteResult::ToString(Result)));
}

FILLGameSessionSettings::FILLGameSessionSettings(bool bIsLAN, int32 MaxNumPlayers, bool bAdvertise, bool bPassworded)
{
	NumPublicConnections = MaxNumPlayers;
	if (NumPublicConnections < 0)
	{
		NumPublicConnections = 0;
	}
	NumPrivateConnections = 0;
	bIsDedicated = IsRunningDedicatedServer();
	bIsLANMatch = bIsLAN;
	bShouldAdvertise = bAdvertise;
	bAllowJoinInProgress = true;
	bAllowInvites = true;
	bUsesPresence = !IsRunningDedicatedServer();
	bAllowJoinViaPresence = true;
	bAllowJoinViaPresenceFriendsOnly = false;
	bIsPassworded = bPassworded;
#if PLATFORM_DESKTOP
	bAntiCheatProtected = true;
#endif
}

UILLOnlineSessionClient::UILLOnlineSessionClient(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	GameJoinConnectionAttempts = 3;
	GameJoinConnectionRetryDelay = 2.f;

#if PLATFORM_XBOXONE || PLATFORM_PS4
	// XB1 will fail the request if there are no differences
	// PS4 will complain indefinitely if you dare send too many updates
	RedundantGameSessionUpdateInterval = 0.f;
#else
	// Assuming Steam, which needs to send redundant updates to update player pings/scores
	RedundantGameSessionUpdateInterval = 30.f;
#endif
	RedundantGameBackendReportInterval = 0.f;

	LobbyClientClass = AILLLobbyBeaconClient::StaticClass();
	LobbyHostObjectClass = AILLLobbyBeaconHostObject::StaticClass();
	LobbyBeaconHostClass = AILLLobbyBeaconHost::StaticClass();

	PartyMaxMembers = 8;
	PartyMessageClass = UILLPartyLocalMessage::StaticClass();
	PartyClientClass = AILLPartyBeaconClient::StaticClass();
	PartyHostObjectClass = AILLPartyBeaconHostObject::StaticClass();
	PartyBeaconHostClass = AILLPartyBeaconHost::StaticClass();
}

UWorld* UILLOnlineSessionClient::GetWorld() const
{
	return GetOuterUILLGameInstance()->GetWorld();
}

void UILLOnlineSessionClient::StartOnlineSession(FName SessionName)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *SessionName.ToString());

	// This does more or less the same thing as the Super call, but with more paranoid!
	IOnlineSessionPtr Sessions = GetSessionInt();
	if (Sessions.IsValid())
	{
		FNamedOnlineSession* Session = Sessions->GetNamedSession(SessionName);
		if (ensureAlways(Session) && ensureAlways(Session->SessionState == EOnlineSessionState::Pending || Session->SessionState == EOnlineSessionState::Ended))
		{
			StartSessionCompleteHandle = Sessions->AddOnStartSessionCompleteDelegate_Handle(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete));
			Sessions->StartSession(SessionName);
		}
	}
}

void UILLOnlineSessionClient::DestroyExistingSession_Impl(FDelegateHandle& OutResult, FName SessionName, FOnDestroySessionCompleteDelegate& Delegate)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *SessionName.ToString());

	// Ensure we have disconnected from the server
	if (SessionName == NAME_GameSession)
	{
		TeardownLobbyBeaconClient();
	}

	Super::DestroyExistingSession_Impl(OutResult, SessionName, Delegate);
}

void UILLOnlineSessionClient::RegisterOnlineDelegates()
{
	Super::RegisterOnlineDelegates();

#if !UE_BUILD_SHIPPING
	IConsoleManager& ConsoleManager = IConsoleManager::Get();
#endif

	OnPreLoadMapDelegateHandle = FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &ThisClass::OnPreLoadMap);
	OnPostLoadMapDelegateHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::OnPostLoadMap);

	OnWorldCleanupDelegateHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &ThisClass::OnWorldCleanup);

	OnDestroyGameForMainMenuCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroyGameForMainMenuComplete);
	OnCreateGameSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateGameSessionComplete);
	OnDestroyGameSessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroyGameSessionComplete);
	OnDestroyForGameSessionCreationFailureCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroyForGameSessionCreationFailure);

	OnCreatePartySessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreatePartySessionComplete);
	OnDestroyPartySessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroyPartySessionComplete);
	OnLeavePartySessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnLeavePartySessionComplete);

#if !UE_BUILD_SHIPPING
	PartyCreateCommand = ConsoleManager.RegisterConsoleCommand(TEXT("ill.PartyCreate"), TEXT(""), FConsoleCommandDelegate::CreateUObject(this, &ThisClass::CmdPartyCreate));
	PartyDestroyCommand = ConsoleManager.RegisterConsoleCommand(TEXT("ill.PartyDestroy"), TEXT(""), FConsoleCommandDelegate::CreateUObject(this, &ThisClass::CmdPartyDestroy));
	PartyInviteCommand = ConsoleManager.RegisterConsoleCommand(TEXT("ill.PartyInvite"), TEXT(""), FConsoleCommandDelegate::CreateUObject(this, &ThisClass::CmdPartyInvite));
	PartyLeaveCommand = ConsoleManager.RegisterConsoleCommand(TEXT("ill.PartyLeave"), TEXT(""), FConsoleCommandDelegate::CreateUObject(this, &ThisClass::CmdPartyLeave));
#endif

#if !UE_BUILD_SHIPPING
	RequestLobbySessionCommand = ConsoleManager.RegisterConsoleCommand(TEXT("ill.RequestLobbySession"), TEXT(""), FConsoleCommandWithArgsDelegate::CreateUObject(this, &ThisClass::CmdRequestLobbySession));
#endif
}

void UILLOnlineSessionClient::ClearOnlineDelegates()
{
	Super::ClearOnlineDelegates();
	
	FCoreUObjectDelegates::PreLoadMap.Remove(OnPreLoadMapDelegateHandle);
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(OnPostLoadMapDelegateHandle);

	FWorldDelegates::OnWorldCleanup.Remove(OnWorldCleanupDelegateHandle);

#if !UE_BUILD_SHIPPING
	IConsoleManager& ConsoleManager = IConsoleManager::Get();
	ConsoleManager.UnregisterConsoleObject(PartyCreateCommand);
	ConsoleManager.UnregisterConsoleObject(PartyDestroyCommand);
	ConsoleManager.UnregisterConsoleObject(PartyInviteCommand);
	ConsoleManager.UnregisterConsoleObject(PartyLeaveCommand);
	ConsoleManager.UnregisterConsoleObject(RequestLobbySessionCommand);
#endif
}

void UILLOnlineSessionClient::OnDestroyForJoinSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UE_LOG(LogOnlineGame, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *SessionName.ToString());
	}
	else
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("%s: %s failed!"), ANSI_TO_TCHAR(__FUNCTION__), *SessionName.ToString());
	}

	IOnlineSessionPtr Sessions = GetSessionInt();
	check(OnDestroyForJoinSessionCompleteDelegateHandle.IsValid());
	Sessions->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroyForJoinSessionCompleteDelegateHandle);

	if (bWasSuccessful)
	{
		const bool bGameSession = (SessionName == NAME_GameSession);
		check(bGameSession || SessionName == NAME_PartySession);
		if (bGameSession)
		{
			JoinSession(SessionName, EndForGameSessionResult, EndForGameSessionCallback);
		}
		else
		{
			JoinSession(SessionName, EndForPartySessionResult, EndForPartySessionCallback);
		}
	}

	bHandlingDisconnect = false;
}

void UILLOnlineSessionClient::OnDestroyForMainMenuComplete(FName SessionName, bool bWasSuccessful)
{
	Super::OnDestroyForMainMenuComplete(SessionName, bWasSuccessful);
}

void UILLOnlineSessionClient::OnSessionUserInviteAccepted(const bool bWasSuccess, const int32 ControllerId, TSharedPtr<const FUniqueNetId> UserId, const FOnlineSessionSearchResult& InviteResult)
{
	if (bWasSuccess && InviteResult.IsValid())
	{
		// Found it!
		bInviteSearchFailed = false;
		DeferredInviteResult = InviteResult;
	}
	else
	{
		// Failed to lookup/find the invite
		bInviteSearchFailed = true;
		DeferredInviteResult = FOnlineSessionSearchResult();
	}

	QueueFlushDeferredInvite();
}

void UILLOnlineSessionClient::OnPlayTogetherEventReceived(int32 UserIndex, TArray<TSharedPtr<const FUniqueNetId>> UserIdList)
{
	PlayTogetherInfo = FILLPlayTogetherInfo(UserIndex, UserIdList);

	// Disallow PlayTogether processing while loading
	if (bLoadingMap)
		return;

	// Verify we have backend authentication first
	// This assumes we perform platform authentication before backend authentication
	if (UILLBackendRequestManager* BRM = GetOuterUILLGameInstance()->GetBackendRequestManager())
	{
		if (!UILLBackendBlueprintLibrary::HasBackendLogin(UILLGameBlueprintLibrary::GetFirstLocalPlayerController(GetWorld())))
		{
			return;
		}
	}

	CheckPlayTogetherParty();
}

bool UILLOnlineSessionClient::HandleDisconnectInternal(UWorld* World, UNetDriver* NetDriver)
{
	if (Super::HandleDisconnectInternal(World, NetDriver))
	{
		// Ensure the lobby beacon host and client are torn down
		TeardownLobbyBeaconHost();
		TeardownLobbyBeaconClient();

		// Rip down the game socket now instead of waiting until we finish leaving the session and loading the main menu
		// This is primarily intended for PS4, because the server will not receive a notification for the shutdown that happens in Browse later due to already being out of the room... somehow this is breaking signaling
		GEngine->ShutdownWorldNetDriver(World);
		return true;
	}

	return false;
}

void UILLOnlineSessionClient::OnStartSessionComplete(FName InSessionName, bool bWasSuccessful)
{
	Super::OnStartSessionComplete(InSessionName, bWasSuccessful);

	if (InSessionName == NAME_GameSession)
	{
		if (!bWasSuccessful)
		{
			HandleGameSessionCreationFailure(LOCTEXT("GameSessionCreationError_StartFailed", "Failed to start game session! Verify you have an internet connection."));
			return;
		}

		// Enable requests on the host listener
		PauseLobbyHostBeaconRequests(false);

		// Update our party leader status
		if (IsInPartyAsLeader())
		{
			GetPartyBeaconState()->SetLeaderState(ILLPartyLeaderState::InMatch);

			if (IsHostingLobbyBeacon())
			{
				// Tell our party members to follow
				PartyHostBeaconObject->NotifyTravelToGameSession();
			}
		}

		// Send our first backend report so subsequent heartbeats are automatically sent
		if (UILLBackendServer* BackendServer = GetOuterUILLGameInstance()->GetBackendRequestManager()->GetBackendServer())
		{
			BackendServer->SendReport();
		}

		if (UWorld* World = GetWorld())
		{
			// Register any players that couldn't before, establishing voice
			if (AGameModeBase* GM = World->GetAuthGameMode())
			{
				if (AILLGameSession* GS = Cast<AILLGameSession>(GM->GameSession))
				{
					GS->FlushPlayersPendingRegistration();
				}
			}

			for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
			{
				AILLPlayerController* PC = Cast<AILLPlayerController>(*It);
				if (PC)
				{
					if (PC->IsLocalController())
					{
						// Update local push to talk settings
						PC->UpdatePushToTalkTransmit();
					}
					else
					{
						// Tell remote clients to start their session locally too
						PC->ClientStartOnlineSession();
					}
				}
			}
		}
	}
	else if (InSessionName == NAME_PartySession)
	{
	}
}

void UILLOnlineSessionClient::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LogOnlineGame, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *SessionName.ToString());
	}
	else
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("%s: %s failed, result: %s"), ANSI_TO_TCHAR(__FUNCTION__), *SessionName.ToString(), *GetLocalizedDescription(SessionName, Result).ToString());
	}

	UWorld* World = GetWorld();
	UILLOnlineMatchmakingClient* OMC = GetOnlineMatchmaking();
	UILLLocalPlayer* FirstLocalPlayer = Cast<UILLLocalPlayer>(World->GetFirstLocalPlayerFromController());
	IOnlineSessionPtr Sessions = GetSessionInt();

	FOnlineSessionSearchResult& CurrentSessionResult = (SessionName == NAME_GameSession) ? CachedSessionResult : CachedPartySessionResult;
	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		// PS4 work-around: At this point CachedSessionResult or CachedPartySessionResult do not have their address, we need to update them from their session
		CurrentSessionResult.Session = *Sessions->GetNamedSession(SessionName);
	}

	if (SessionName == NAME_GameSession)
	{
		// Cleanup our delegate
	//	check(OnJoinSessionCompleteDelegateHandle.IsValid()); // Can't because of FakeJoinCompletion...
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);

		// Cleanup timer(s) that may have called us
		if (TimerHandle_RetryGameOnJoinSessionComplete.IsValid())
		{
			World->GetTimerManager().ClearTimer(TimerHandle_RetryGameOnJoinSessionComplete);
			TimerHandle_RetryGameOnJoinSessionComplete.Invalidate();
		}

		if (Result == EOnJoinSessionCompleteResult::Success)
		{
			LobbyTravelURL = FString();
			int32 BeaconListenPort = 0;
			const bool bCanRetryConnectionLookup = (GameJoinConnectionAttempts > 0 && TotalGameJoinAttempts < GameJoinConnectionAttempts);
			if (!Sessions->GetResolvedConnectString(SessionName, LobbyTravelURL))
			{
				if (bCanRetryConnectionLookup)
				{
					UE_LOG(LogOnlineGame, Verbose, TEXT("%s: %s failed to get connection string, retrying..."), ANSI_TO_TCHAR(__FUNCTION__), *SessionName.ToString());
				}
				else
				{
					UE_LOG(LogOnlineGame, Warning, TEXT("%s: %s failed to get connection string!"), ANSI_TO_TCHAR(__FUNCTION__), *SessionName.ToString());
				}
				Result = EOnJoinSessionCompleteResult::CouldNotRetrieveAddress;
			}
			else if (!CurrentSessionResult.Session.SessionSettings.Get(SETTING_BEACONPORT, BeaconListenPort))
			{
				if (bCanRetryConnectionLookup)
				{
					UE_LOG(LogOnlineGame, Verbose, TEXT("%s: %s failed to get beacon port, retrying..."), ANSI_TO_TCHAR(__FUNCTION__), *SessionName.ToString());
				}
				else
				{
					UE_LOG(LogOnlineGame, Warning, TEXT("%s: %s failed to get beacon port!"), ANSI_TO_TCHAR(__FUNCTION__), *SessionName.ToString());
				}
				Result = EOnJoinSessionCompleteResult::CouldNotRetrieveAddress;
			}
			else
			{
				if (CurrentSessionResult.Session.SessionSettings.bIsDedicated)
					LobbyTravelURL += TEXT("?bIsDedicated");

				// Spawn a lobby beacon client if we do not already have one, because RequestLobbySession may have already established one
				if (!LobbyClientBeacon)
				{
					SpawnLobbyBeaconClient();
				}

				// Connect to the beacon and request a reservation
				// We do no error handling here because OnLobbyClientHostConnectionFailure should fire if this fails, triggering CachedGameSessionCallback
				AILLPartyBeaconState* HostingPartyState = IsInPartyAsLeader() ? GetPartyBeaconState() : nullptr;
				const FILLBackendPlayerIdentifier& PartyLeaderAccountId = IsInPartyAsMember() ? PartyClientBeacon->GetBeaconOwnerAccountId() : FILLBackendPlayerIdentifier::GetInvalid();
				LobbyClientBeacon->RequestLobbyReservation(CurrentSessionResult, FirstLocalPlayer, HostingPartyState, PartyLeaderAccountId, OMC->IsMatchmaking());
			}

			// Check if we should retry looking up connection details
			if (Result == EOnJoinSessionCompleteResult::CouldNotRetrieveAddress && bCanRetryConnectionLookup)
			{
				++TotalGameJoinAttempts;
				auto RetryDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete, SessionName, EOnJoinSessionCompleteResult::Success);
				World->GetTimerManager().SetTimer(TimerHandle_RetryGameOnJoinSessionComplete, RetryDelegate, GameJoinConnectionRetryDelay, false);
				return;
			}
		}

		if (Result != EOnJoinSessionCompleteResult::Success)
		{
			// Notify our callback of completion
			CachedGameSessionCallback.ExecuteIfBound(SessionName, Result);
			ClearPendingGameSession();
		}
	}
	else if (SessionName == NAME_PartySession)
	{
		// Cleanup our delegate
		check(OnJoinPartySessionCompleteDelegateHandle.IsValid());
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinPartySessionCompleteDelegateHandle);

		if (Result == EOnJoinSessionCompleteResult::Success)
		{
			checkf(!PartyHostBeaconObject, TEXT("Party merging not supported yet!"));

			// Spawn a party beacon client
			SpawnPartyBeaconClient(EILLPartyClientState::InitialConnect);

			// Connect to the beacon and request a reservation
			if (!PartyClientBeacon->RequestJoinParty(CurrentSessionResult, FirstLocalPlayer)) // TODO: pjackson: Test failure
			{
				UE_LOG(LogOnlineGame, Warning, TEXT("%s: %s RequestJoinParty failed!"), ANSI_TO_TCHAR(__FUNCTION__), *SessionName.ToString());

				// Teardown party beacon actors
				TeardownPartyBeaconClient();
				TeardownPartyBeaconHost();
				Result = EOnJoinSessionCompleteResult::CouldNotRetrieveAddress;
			}
		}

		if (Result != EOnJoinSessionCompleteResult::Success)
		{
			CachedPartySessionCallback.ExecuteIfBound(SessionName, Result);
			CachedPartySessionCallback.Unbind();
			CachedPartySessionResult = FOnlineSessionSearchResult();

			// Leave the party, OnSessionJoinFailure will display the error message
			LeavePartySession();
		}
	}
	else
	{
		check(0);
	}

	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		// Show the reserving screen on success
		GetOuterUILLGameInstance()->ShowReservingScreen(SessionName, CurrentSessionResult);
	}
	else
	{
		// Hide any previous connecting or loading screens
		GetOuterUILLGameInstance()->OnSessionJoinFailure(SessionName, Result);
	}
}

void UILLOnlineSessionClient::JoinSession(FName SessionName, const FOnlineSessionSearchResult& SearchResult)
{
	// Disabled because JoinSession now requires a delegate to operate
	check(0);
//	JoinSession(SessionName, SearchResult, nullptr);
}

UILLOnlineMatchmakingClient* UILLOnlineSessionClient::GetOnlineMatchmaking() const
{
	return GetOuterUILLGameInstance()->GetOnlineMatchmaking();
}

void UILLOnlineSessionClient::OnRegisteredStandalone()
{
	// Tear down our previous lobby beacon host/client, if any
	TeardownLobbyBeaconHost();
	TeardownLobbyBeaconClient();

	if (IsInPartyAsLeader())
	{
		// Tell our party members to follow
		PartyHostBeaconObject->NotifyTravelToMainMenu();

		// Reset our party leader state
		GetPartyBeaconState()->SetLeaderState(ILLPartyLeaderState::Idle);
	}

	// Cleanup our game session if we have one
	IOnlineSessionPtr Sessions = GetSessionInt();
	if (Sessions.IsValid())
	{
		if (Sessions->GetSessionState(NAME_GameSession) != EOnlineSessionState::NoSession)
		{
			DestroyExistingSession_Impl(OnDestroyGameSessionCompleteDelegateHandle, NAME_GameSession, OnDestroyGameSessionCompleteDelegate);
		}
	}

	// Clear the instance cycle count after recycling
	GetOuterUILLGameInstance()->OnGameRegisteredStandalone();

	// Flag that we are no longer using multiplayer features
	if (APlayerController* PC = GetOuterUILLGameInstance()->GetFirstLocalPlayerController())
	{
		if (PC->GetLocalPlayer() && PC->GetLocalPlayer()->GetPreferredUniqueNetId().IsValid())
		{
			IOnlineSubsystem::Get()->SetUsingMultiplayerFeatures(*PC->GetLocalPlayer()->GetPreferredUniqueNetId(), false);
		}
	}
}

void UILLOnlineSessionClient::OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	if (World)
	{
		// Ensure these are gone. This really shouldn't happen if they aren't though... right?
		if (World == LobbyBeaconWorld)
		{
			TeardownLobbyBeaconClient();
		}
		if (World == PartyBeaconWorld)
		{
			TeardownPartyBeaconClient();
			TeardownPartyBeaconHost();
		}
	}
}

void UILLOnlineSessionClient::OnPreLoadMap(const FString& MapName)
{
	bLoadingMap = true;
}

void UILLOnlineSessionClient::OnPostLoadMap(UWorld* World)
{
	bLoadingMap = false;

	if (World)
	{
		if (PendingSignoutDisconnectFailurePlayer)
		{
			// Sign out was deferred, consume PendingSignoutDisconnectFailurePlayer and PendingSignoutDisconnectFailureText
			World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::PerformSignoutAndDisconnect, PendingSignoutDisconnectFailurePlayer, PendingSignoutDisconnectFailureText));
			PendingSignoutDisconnectFailurePlayer = nullptr;
			PendingSignoutDisconnectFailureText = FText();
		}
		else
		{
			// No pending sign out
			if (PlayTogetherInfo.IsValid())
			{
				// Handle deferred PlayTogether info
				World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::AttemptFlushPlayTogetherParty);
			}
			else
			{
				// Handle a deferred invite
				QueueFlushDeferredInvite();
			}
		}
	}
}

void UILLOnlineSessionClient::PerformSignoutAndDisconnect(UILLLocalPlayer* Player, FText FailureText)
{
	// Defer the sign out and disconnect until the map load finishes
	UWorld* World = GetWorld();
	if (!World || bLoadingMap)
	{
		UE_LOG(LogOnlineGame, Log, TEXT("%s: %s (%s) deferred while loading"), ANSI_TO_TCHAR(__FUNCTION__), *Player->GetName(), *FailureText.ToString());
		if (ensure(Player))
		{
			// Notify the player that they are pending a sign out
			Player->PendingSignout();

			// Store the failure reason
			PendingSignoutDisconnectFailurePlayer = Player;
			if (PendingSignoutDisconnectFailureText.IsEmpty())
				PendingSignoutDisconnectFailureText = FailureText;
		}
		return;
	}

	UE_LOG(LogOnlineGame, Log, TEXT("%s: %s (%s)"), ANSI_TO_TCHAR(__FUNCTION__), *Player->GetName(), *FailureText.ToString());

	// Notify the player of sign out
	Player->PerformSignout();

	// Cancel matchmaking
	UILLOnlineMatchmakingClient* OMC = GetOnlineMatchmaking();
	OMC->CancelMatchmaking();

	// Destroy or leave our party
	// FIXME: pjackson: Wait until this completes before doing it for the game session below?
	if (IsInPartyAsLeader())
	{
		DestroyPartySession();
	}
	else if (IsJoiningParty() || IsInPartyAsMember())
	{
		LeavePartySession();
	}

	// Store off our failure reason
	GetOuterUILLGameInstance()->OnConnectionFailurePreDisconnect(FailureText);

	// Destroy our current game session and return to the main menu to display it
	DestroyExistingSession_Impl(OnDestroyForMainMenuCompleteDelegateHandle, NAME_GameSession, OnDestroyForMainMenuCompleteDelegate);
}

void UILLOnlineSessionClient::CheckDeferredInvite()
{
	if (!HasDeferredInvite())
		return;

	// Disallow invite processing while loading/joining another game
	UWorld* World = GetWorld();
	IOnlineSessionPtr Sessions = GetSessionInt();
	if (!World || World->IsInSeamlessTravel() || bLoadingMap || IsJoiningGameSession() || !Sessions.IsValid())
	{
		// Kick it down the road a tick
		QueueFlushDeferredInvite();
		return;
	}

	// Wait for our player to finish traveling
	AILLPlayerController* FirstLocalPC = UILLGameBlueprintLibrary::GetFirstLocalPlayerController(World);
	if (!FirstLocalPC || !FirstLocalPC->HasFullyTraveled())
	{
		// Kick it down the road a tick
		QueueFlushDeferredInvite();
		return;
	}

	// Wait for any pending privilege checks
	UILLLocalPlayer* LocalPlayer = Cast<UILLLocalPlayer>(FirstLocalPC->Player);
	if (!LocalPlayer || LocalPlayer->IsCheckingPrivileges())
	{
		// Kick it down the road a tick
		QueueFlushDeferredInvite();
		return;
	}

	// Verify we have backend authentication
	// This should occur after platform login + privilege collection
	if (UILLBackendRequestManager* BRM = GetOuterUILLGameInstance()->GetBackendRequestManager())
	{
		if (!UILLBackendBlueprintLibrary::HasBackendLogin(FirstLocalPC))
		{
			// Kick it down the road a tick
			QueueFlushDeferredInvite();
			return;
		}
	}

	// Wait until we are done matchmaking before processing the invite
	UILLOnlineMatchmakingClient* OMC = GetOnlineMatchmaking();
	if (OMC->IsMatchmaking())
	{
		OMC->CancelMatchmaking();
		return;
	}

	if (bInviteSearchFailed)
	{
		UE_LOG(LogOnlineGame, Verbose, TEXT("%s: Handling invite lookup failure..."), ANSI_TO_TCHAR(__FUNCTION__));
	}
	else
	{
		UE_LOG(LogOnlineGame, Verbose, TEXT("%s: Evaluating invite..."), ANSI_TO_TCHAR(__FUNCTION__));
	}

	// Determine what kind of session it is
	const FText ErrorHeaderText = LOCTEXT("InviteErrorHeader", "Invite Error!");
	FName SessionName = NAME_GameSession;
	if (!bInviteSearchFailed)
	{
		// NOTE: Search failure case assumes it was a NAME_GameSession failure
		FString SessionMap;
		DeferredInviteResult.Session.SessionSettings.Get(SETTING_MAPNAME, SessionMap);
		if (SessionMap.IsEmpty() && !DeferredInviteResult.Session.SessionSettings.bIsDedicated)
		{
			// No map, assume it's a party instead
			SessionName = NAME_PartySession;
		}
	}

	// Handle matchmaking status on public game sessions
	if (!bInviteSearchFailed && SessionName == NAME_GameSession && DeferredInviteResult.Session.SessionSettings.bShouldAdvertise)
	{
		switch (OMC->GetLastAggregateMatchmakingStatus())
		{
		case EILLBackendMatchmakingStatusType::Unknown:
			// Wait until we have a matchmaking status for game invites
			OMC->GetAggregateMatchmakingStatusDelegate().AddUObject(this, &ThisClass::OnInvitePartyStatusResponse);
			OMC->RequestPartyMatchmakingStatus();
			return;

		case EILLBackendMatchmakingStatusType::Ok:
			// We're good to join anything
			break;

		case EILLBackendMatchmakingStatusType::LowPriority:
			{
				// Make sure this is a low-priority session
				int32 SessionPriority = SESSION_PRIORITY_OK;
#if PLATFORM_XBOXONE
				FString HopperName;
				if (!DeferredInviteResult.Session.SessionSettings.Get(SEARCH_XBOX_LIVE_HOPPER_NAME, HopperName))
				{
					UE_LOG(LogOnlineGame, Error, TEXT("%s: Failed to retrieve the game session hopper name!"), ANSI_TO_TCHAR(__FUNCTION__));
				}
				else if (HopperName == XBOXONE_QUICKPLAY_HOPPER_LOWPRIORITY)
				{
					SessionPriority = SESSION_PRIORITY_LOW;
				}
#else
				int32 Priority = SESSION_PRIORITY_OK;
				if (!DeferredInviteResult.Session.SessionSettings.Get(SETTING_PRIORITY, Priority))
				{
					UE_LOG(LogOnlineGame, Error, TEXT("%s: Failed to retrieve game session priority!"), ANSI_TO_TCHAR(__FUNCTION__));
				}
				else
				{
					SessionPriority = Priority;
				}
#endif
				// Check if it's Ok, if it is we can not join
				if (SessionPriority != SESSION_PRIORITY_LOW)
				{
					GetOuterUILLGameInstance()->HandleLocalizedError(ErrorHeaderText, LOCTEXT("InviteJoinError_LowPriorityDenied", "You are in low priority matchmaking and may not join this session!"), false);
					ClearDeferredInvite();
					return;
				}
			}
			break;

		case EILLBackendMatchmakingStatusType::Banned:
			// Check if the ban has expired
			if (UILLMatchmakingBlueprintLibrary::GetMatchmakingBanRemainingSeconds(World) <= 0)
			{
				OMC->InvalidateMatchmakingStatus();
				OMC->GetAggregateMatchmakingStatusDelegate().AddUObject(this, &ThisClass::OnInvitePartyStatusResponse);
				OMC->RequestPartyMatchmakingStatus();
			}
			else
			{
				GetOuterUILLGameInstance()->HandleLocalizedError(ErrorHeaderText, LOCTEXT("InviteJoinError_BanDenied", "You can not join a public game session while banned from matchmaking!"), false);
				ClearDeferredInvite();
			}
			return;

		default: check(0); break;
		}
	}

	// Handle deferred failure case immediately
	// Handle anti-cheat blocking, lack of play online and offline mode too
	const bool bHasPlayOnline = UILLGameOnlineBlueprintLibrary::HasPlayOnlinePrivilege(FirstLocalPC);
	const bool bInOfflineMode = UILLBackendBlueprintLibrary::IsInOfflineMode(FirstLocalPC);
	if (bInviteSearchFailed || GAntiCheatBlockingOnlineMatches || !bHasPlayOnline || bInOfflineMode)
	{
		// Flush the failure!
		const bool bLastInviteSearchFailed = bInviteSearchFailed;
		ClearDeferredInvite();

		// Report the failure to the UI
		if (GAntiCheatBlockingOnlineMatches)
		{
			GetOuterUILLGameInstance()->HandleLocalizedError(ErrorHeaderText, LOCTEXT("InviteJoinError_AntiCheatDenied", "You can not join an invite while anti-cheat is disabled!"), false);
		}
		else if (bLastInviteSearchFailed || !bHasPlayOnline || bInOfflineMode)
		{
			if (UILLGameOnlineBlueprintLibrary::HasPlayOnlineFailureFlags(FirstLocalPC, IOnlineIdentity::EPrivilegeResults::AccountTypeFailure))
			{
#if PLATFORM_PS4
				// External UI displayed instead
				if (!UILLGameOnlineBlueprintLibrary::ShowAccountUpgradeUI(FirstLocalPC))
				{
					// Wait until we can show the UI
					UE_LOG(LogOnlineGame, Log, TEXT("%s: Could not display external UI! Deferring..."), ANSI_TO_TCHAR(__FUNCTION__));
					QueueFlushDeferredInvite();
				}
#elif PLATFORM_XBOXONE
				// Refresh the PlayOnline privilege, which displays the "buy XboxLive Gold!" nag
				LocalPlayer->RefreshPlayOnlinePrivilege();
#else
				check(0);
#endif
			}
#if PLATFORM_XBOXONE
			else if (UILLGameOnlineBlueprintLibrary::HasPlayOnlineFailureFlags(FirstLocalPC, IOnlineIdentity::EPrivilegeResults::OnlinePlayRestricted))
			{
				// Refresh the PlayOnline privilege, which displays the change privacy settings
				LocalPlayer->RefreshPlayOnlinePrivilege();
			}
#endif
			else if (UILLGameOnlineBlueprintLibrary::HasPlayOnlineFailureFlags(FirstLocalPC, IOnlineIdentity::EPrivilegeResults::RequiredPatchAvailable))
			{
				GetOuterUILLGameInstance()->PlayOnlineRequiredPatchAvailable(ErrorHeaderText);
			}
			else if (UILLGameOnlineBlueprintLibrary::HasPlayOnlineFailureFlags(FirstLocalPC, IOnlineIdentity::EPrivilegeResults::RequiredSystemUpdate))
			{
				GetOuterUILLGameInstance()->PlayOnlineRequiredSystemUpdate(ErrorHeaderText);
			}
			else if (UILLGameOnlineBlueprintLibrary::HasPlayOnlineFailureFlags(FirstLocalPC, IOnlineIdentity::EPrivilegeResults::AgeRestrictionFailure))
			{
				GetOuterUILLGameInstance()->PlayOnlineAgeRestrictionFailure(ErrorHeaderText);
			}
			else if (bLastInviteSearchFailed)
			{
				GetOuterUILLGameInstance()->HandleLocalizedError(ErrorHeaderText, LOCTEXT("InviteJoinError_SearchFailed", "Failed to lookup session details! Verify internet connectivity."), false);
			}
			else if (bInOfflineMode)
			{
				GetOuterUILLGameInstance()->HandleLocalizedError(ErrorHeaderText, LOCTEXT("InviteJoinError_OfflineMode", "Can not join an invite while in offline mode!"), false);
			}
			else
			{
				// Should be a play online failure
				GetOuterUILLGameInstance()->OnInviteJoinSessionFailure(SessionName, EOnJoinSessionCompleteResult::PlayOnlinePrivilege, false);
			}
		}
		return;
	}

	// Forbid session invites while leading or in a party
	if (IsInPartyAsLeader())
	{
		if (SessionName == NAME_GameSession)
		{
			// Verify we're joining a session that advertises
			if (!DeferredInviteResult.Session.SessionSettings.bShouldAdvertise)
			{
				UE_LOG(LogOnlineGame, Log, TEXT("%s: %s session: Attempted to join private match invite while leading a party!"), ANSI_TO_TCHAR(__FUNCTION__), *SessionName.ToString());

				GetOuterUILLGameInstance()->HandleLocalizedError(ErrorHeaderText, LOCTEXT("InviteJoinError_PartyLeaderToPrivateMatch", "Parties can not join Private Matches invites. Close it before joining."), false);
				ClearDeferredInvite();
				return;
			}
		}
		else if (SessionName == NAME_PartySession)
		{
			UE_LOG(LogOnlineGame, Log, TEXT("%s: %s session: Already in party as leader!"), ANSI_TO_TCHAR(__FUNCTION__), *SessionName.ToString());

			GetOuterUILLGameInstance()->HandleLocalizedError(ErrorHeaderText, LOCTEXT("InviteJoinError_PartyLeaderToParty", "You are leading a party. Close it before joining another."), false);
			ClearDeferredInvite();
			return;
		}
	}
	else if (IsJoiningParty() || IsInPartyAsMember())
	{
		UE_LOG(LogOnlineGame, Log, TEXT("%s: %s session: Already in party as member!"), ANSI_TO_TCHAR(__FUNCTION__), *SessionName.ToString());

		if (SessionName == NAME_GameSession)
			GetOuterUILLGameInstance()->HandleLocalizedError(ErrorHeaderText, LOCTEXT("InviteJoinError_PartyMemberToGame", "You are a party member. Leave that party before joining a different game."), false);
		else if (SessionName== NAME_PartySession)
			GetOuterUILLGameInstance()->HandleLocalizedError(ErrorHeaderText, LOCTEXT("InviteJoinError_PartyMemberToParty", "You are already a party member. Leave that party before joining another."), false);
		ClearDeferredInvite();
		return;
	}

#if PLATFORM_DESKTOP // FIXME: pjackson
	// Force garbage collection to run now
	// Ensures any dangling SteamP2P sockets are flushed before creating new ones, there's a bug allow ones pending kill to be used!
	GEngine->PerformGarbageCollectionAndCleanupActors();
#endif

	// See if we are already in a session for the kind we are being invited to
	if (FNamedOnlineSession* ExistingSession = Sessions->GetNamedSession(SessionName))
	{
		if (ExistingSession->OwningUserId.IsValid() && DeferredInviteResult.Session.OwningUserId.IsValid() && *ExistingSession->OwningUserId == *DeferredInviteResult.Session.OwningUserId)
		{
			UE_LOG(LogOnlineGame, Verbose, TEXT("%s: Already in %s session!"), ANSI_TO_TCHAR(__FUNCTION__), *SessionName.ToString());

			// We are already in this session, do not process!
			ClearDeferredInvite();
			return;
		}
		else
		{
			if (ExistingSession->SessionState == EOnlineSessionState::Destroying)
			{
				UE_LOG(LogOnlineGame, Verbose, TEXT("%s: Waiting for %s session to finish cleanup"), ANSI_TO_TCHAR(__FUNCTION__), *SessionName.ToString());
				QueueFlushDeferredInvite();
				return;
			}
			else if (SessionName == NAME_GameSession)
			{
				UE_LOG(LogOnlineGame, Verbose, TEXT("%s: Returning to main menu to process game session invite"), ANSI_TO_TCHAR(__FUNCTION__));

				// TODO: pjackson: Prompt to leave your current SessionName?
				// We are already in a game session, we need to leave for the main menu THEN process the invite
				DestroyExistingSession_Impl(OnDestroyForMainMenuCompleteDelegateHandle, SessionName, OnDestroyForMainMenuCompleteDelegate);
				return;
			}
			else if (ensure(SessionName == NAME_PartySession))
			{
				check(0); // pjackson: This should not be hit unless you are somehow not in a party beacon (see above) yet are in a party session, which should be forbidden
				ClearDeferredInvite();
				return;
			}
		}
	}
	
	UE_LOG(LogOnlineGame, Log, TEXT("%s: Joining %s invite..."), ANSI_TO_TCHAR(__FUNCTION__), *SessionName.ToString());

	// Attempt to join
	if (!JoinSession(SessionName, DeferredInviteResult, FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnInviteJoinSessionComplete), FString(), TEXT("?bIsFromInvite")))
	{
		OnInviteJoinSessionComplete(SessionName, EOnJoinSessionCompleteResult::UnknownError);
	}

	// Clear DeferredInviteResult
	ClearDeferredInvite();
}

void UILLOnlineSessionClient::OnInvitePartyStatusResponse(EILLBackendMatchmakingStatusType AggregateStatus, float UnBanRealTimeSeconds)
{
	// Attempt to process our pending invite now
	QueueFlushDeferredInvite();
}

void UILLOnlineSessionClient::QueueFlushDeferredInvite()
{
	GetOuterUILLGameInstance()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::CheckDeferredInvite);
}

void UILLOnlineSessionClient::OnInviteJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	// Remove previous loading screen
	GetOuterUILLGameInstance()->HandleFinishedJoinInvite(SessionName, (Result == EOnJoinSessionCompleteResult::Success));

	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LogOnlineGame, Verbose, TEXT("OnInviteJoinSessionComplete %s"), *SessionName.ToString());
	}
	else
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("OnInviteJoinSessionComplete %s failed, result: %s"), *SessionName.ToString(), *GetLocalizedDescription(SessionName, Result).ToString());

		GetOuterUILLGameInstance()->OnInviteJoinSessionFailure(SessionName, Result, false);

		if (SessionName == NAME_GameSession)
		{
			IOnlineSessionPtr Sessions = GetSessionInt();
			EOnlineSessionState::Type GameSessionState = Sessions->GetSessionState(SessionName);
			if (GameSessionState != EOnlineSessionState::NoSession && GameSessionState != EOnlineSessionState::Destroying)
			{
				UE_LOG(LogOnlineGame, Warning, TEXT("%s: Dangling Game session detected, destroying it!"), ANSI_TO_TCHAR(__FUNCTION__));
				DestroyExistingSession_Impl(OnDestroyGameSessionCompleteDelegateHandle, NAME_GameSession, OnDestroyGameSessionCompleteDelegate);
			}
		}
	}
}

bool UILLOnlineSessionClient::JoinSession(FName SessionName, const FOnlineSessionSearchResult& SearchResult, FOnJoinSessionCompleteDelegate Callback, const FString Password/* = FString()*/, const FString TravelOptions/* = FString()*/, const bool bAllowLobbyClientResume/* = false*/)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *SessionName.ToString());

	check(SessionName == NAME_GameSession || SessionName == NAME_PartySession);
	check(Callback.IsBound());

	// Cache off the password and travel options
	if (SessionName == NAME_GameSession)
	{
		CachedSessionPassword = Password;
		CachedGameTravelOptions = TravelOptions;
	}

	// Cache off session and callback information
	if (SessionName == NAME_GameSession)
	{
		check(!CachedGameSessionCallback.IsBound());

		// Clear our current lobby beacon host and client and previous pending game info
		TeardownLobbyBeaconHost();
		if (!bAllowLobbyClientResume)
		{
			TeardownLobbyBeaconClient();
		}

		TotalGameJoinAttempts = 0;
		CachedGameSessionCallback = Callback;
		CachedSessionResult = SearchResult;
	}
	else
	{
		check(!CachedPartySessionCallback.IsBound());

		// Clear our current party beacon client and previous pending party info
		TeardownPartyBeaconClient();

		CachedPartySessionCallback = Callback;
		CachedPartySessionResult = SearchResult;
	}

	// Show the connecting screen
	GetOuterUILLGameInstance()->ShowConnectingLoadScreen(SessionName, SearchResult);

	// Clean up existing sessions if applicable
	IOnlineSessionPtr Sessions = GetSessionInt();
	EOnlineSessionState::Type SessionState = Sessions->GetSessionState(SessionName);
	if (SessionState != EOnlineSessionState::NoSession)
	{
		// This was fucked!
		// End the session before destroying it
		if (SessionName == NAME_GameSession)
		{
			EndForGameSessionResult = SearchResult;
			EndForGameSessionCallback = Callback;
		}
		else if (SessionName == NAME_PartySession)
		{
			EndForPartySessionResult = SearchResult;
			EndForPartySessionCallback = Callback;
		}
		OnEndForJoinSessionCompleteDelegateHandle = Sessions->AddOnEndSessionCompleteDelegate_Handle(OnEndForJoinSessionCompleteDelegate);
		Sessions->EndSession(SessionName);
		return true;
	}

	// Check online privileges
	UWorld* World = GetWorld();
	AILLPlayerController* FirstLocalPC = Cast<AILLPlayerController>(GetOuterUILLGameInstance()->GetFirstLocalPlayerController());
	UILLLocalPlayer* FirstLocalPlayer = FirstLocalPC ? Cast<UILLLocalPlayer>(FirstLocalPC->GetLocalPlayer()) : nullptr;
	if (!FirstLocalPlayer)
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("%s: %s failed, no local player!"), ANSI_TO_TCHAR(__FUNCTION__), *SessionName.ToString());
		return false;
	}
	if (GAntiCheatBlockingOnlineMatches)
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("%s: %s failed, anti-cheat is blocking online matches!"), ANSI_TO_TCHAR(__FUNCTION__), *SessionName.ToString());
		return false;
	}
	if (!SearchResult.Session.SessionSettings.bIsLANMatch && !World->IsPlayInEditor())
	{
		if (UILLBackendBlueprintLibrary::IsInOfflineMode(FirstLocalPC))
		{
			UE_LOG(LogOnlineGame, Warning, TEXT("%s: %s failed, you are in offline mode!"), ANSI_TO_TCHAR(__FUNCTION__), *SessionName.ToString());
			return false;
		}
		if (!UILLGameOnlineBlueprintLibrary::HasPlayOnlinePrivilege(FirstLocalPC))
		{
			UE_LOG(LogOnlineGame, Warning, TEXT("%s: %s failed, you do not have play online privileges!"), ANSI_TO_TCHAR(__FUNCTION__), *SessionName.ToString());
			return false;
		}
	}

	// Join the session
	if (SessionName == NAME_GameSession)
	{
		check(!OnJoinSessionCompleteDelegateHandle.IsValid());
		OnJoinSessionCompleteDelegateHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);
	}
	else if (SessionName == NAME_PartySession)
	{
		check(!OnJoinPartySessionCompleteDelegateHandle.IsValid());
		OnJoinPartySessionCompleteDelegateHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);
	}
	TSharedPtr<const FUniqueNetId> UserId = FirstLocalPlayer->GetPreferredUniqueNetId();
	if (!Sessions->JoinSession(*UserId, SessionName, SearchResult))
	{
		// Hide any previous connecting or loading screens
		GetOuterUILLGameInstance()->OnSessionJoinFailure(SessionName, EOnJoinSessionCompleteResult::UnknownError);

		UE_LOG(LogOnlineGame, Warning, TEXT("%s: %s failed!"), ANSI_TO_TCHAR(__FUNCTION__), *SessionName.ToString());
		return false;
	}

	return true;
}

void UILLOnlineSessionClient::FakeJoinCompletion(FOnJoinSessionCompleteDelegate Callback, EOnJoinSessionCompleteResult::Type Result)
{
	TotalGameJoinAttempts = 0;
	CachedGameSessionCallback = Callback;
	OnJoinSessionComplete(NAME_GameSession, Result);
}

void UILLOnlineSessionClient::OnDestroyGameForMainMenuComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionName != NAME_GameSession)
		return;

	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	IOnlineSessionPtr Sessions = GetSessionInt();
	check(OnDestroyGameForMainMenuCompleteDelegateHandle.IsValid());
	Sessions->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroyGameForMainMenuCompleteDelegateHandle);

	// FIXME: pjackson: This seems like the wrong spot to call to CachedGameSessionCallback?
	CachedGameSessionCallback.ExecuteIfBound(SessionName, EOnJoinSessionCompleteResult::CouldNotRetrieveAddress);

	// Ensure our lobby beacon client and pending game session info are cleaned up
	TeardownLobbyBeaconClient();

	// Return to the main menu
	OnDestroyForMainMenuComplete(SessionName, bWasSuccessful);
}

UWorld* UILLOnlineSessionClient::SpawnBeaconWorld(const FName& WorldName)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *WorldName.ToString());

	// Spawn a non-game world, force it past BeginPlay and actor initialization so RPCs work
	UWorld* BeaconWorld = UWorld::CreateWorld(EWorldType::None, true, WorldName, nullptr, true);
	BeaconWorld->bBegunPlay = true;
	BeaconWorld->bActorsInitialized = true;
	BeaconWorld->SetGameInstance(GetOuterUILLGameInstance());

	// Create a blank world context, to prevent crashes
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::None);
	WorldContext.SetCurrentWorld(BeaconWorld);

	return BeaconWorld;
}

bool UILLOnlineSessionClient::HostGameSession(TSharedPtr<FILLGameSessionSettings> SessionSettings)
{
	UWorld* World = GetWorld();

#if WITH_EDITOR
	if (World && World->IsPlayInEditor())
		return false;
#endif

	check(SessionSettings->NumPublicConnections > 0);

	// Check anti-cheat
	if (GAntiCheatBlockingOnlineMatches)
	{
		HandleGameSessionCreationFailure(LOCTEXT("GameSessionCreationError_AntiCheatDenied", "Anti-cheat is preventing hosting a game session!"));
		return false;
	}

	// Online match checks
	if (!SessionSettings->bIsLANMatch)
	{
		if (AILLPlayerController* PC = Cast<AILLPlayerController>(GetOuterUILLGameInstance()->GetFirstLocalPlayerController()))
		{
			// Disallow game session creation while in offline mode or when there are no play online privileges
			if (UILLBackendBlueprintLibrary::IsInOfflineMode(PC))
			{
				HandleGameSessionCreationFailure(LOCTEXT("GameSessionCreationError_OfflineModeBlocked", "Can not host game session while in offline mode!"));
				return false;
			}
			else if (!UILLGameOnlineBlueprintLibrary::HasPlayOnlinePrivilege(PC))
			{
				HandleGameSessionCreationFailure(LOCTEXT("GameSessionCreationError_PlayOnlineBlocked", "Can not host a game session without play online privileges! Verify you have an internet connection."));
				return false;
			}

			// Verify the matchmaking status for public games
			if (SessionSettings->bShouldAdvertise)
			{
				UILLOnlineMatchmakingClient* OMC = GetOnlineMatchmaking();
				const EILLBackendMatchmakingStatusType MatchmakingStatus = OMC->GetLastAggregateMatchmakingStatus();
				if (MatchmakingStatus == EILLBackendMatchmakingStatusType::Unknown)
				{
					HandleGameSessionCreationFailure(LOCTEXT("GameSessionCreationError_MatchmakingStatusUnknown", "Can not host public game session without a matchmaking status!"));
					return false;
				}
				else if (MatchmakingStatus == EILLBackendMatchmakingStatusType::Banned)
				{
					HandleGameSessionCreationFailure(LOCTEXT("GameSessionCreationError_MatchmakingStatusBanned", "Can not host public game session while banned from matchmaking!"));
					return false;
				}
				else
				{
					checkf(MatchmakingStatus == EILLBackendMatchmakingStatusType::Ok || MatchmakingStatus == EILLBackendMatchmakingStatusType::LowPriority, TEXT("%s: Unexpected matchmaking status %s"), ANSI_TO_TCHAR(__FUNCTION__), ToString(MatchmakingStatus));
				}
			}
		}
	}

	// Check session interface
	IOnlineSessionPtr Sessions = GetSessionInt();
	if (!Sessions.IsValid())
	{
		HandleGameSessionCreationFailure(LOCTEXT("GameSessionCreationError_NoSessionInterface", "No session interface to start game session! Verify you have an internet connection."));
		return false;
	}

#if PLATFORM_DESKTOP
	// Verify Steam session interface presence
	IOnlineSessionPtr ExpectedSessionsPtr = Online::GetSessionInterface(STEAM_SUBSYSTEM);
	if (!ExpectedSessionsPtr.IsValid() || Sessions != ExpectedSessionsPtr)
	{
		HandleGameSessionCreationFailure(LOCTEXT("GameSessionCreationError_WrongSessionInterface", "Incorrect session interface to start game session! Verify you have an internet connection."));
		return false;
	}
#endif

	// Spawn lobby host beacon
	if (!SpawnLobbyBeaconHost(*SessionSettings.Get()))
	{
		HandleGameSessionCreationFailure(LOCTEXT("GameSessionCreationError_LobbyBeaconHostFailed", "Failed to create lobby host connection! Verify you have an internet connection."));
		return false;
	}

	// Apply general game settings
	UpdateGameSessionSettings(SessionSettings.ToSharedRef().Get());
	UE_LOG(LogOnlineGame, Log, TEXT("%s: NumPublicConnections: %i (LAN? %s, Private? %s, Dedicated? %s, Presence? %s)"),
		ANSI_TO_TCHAR(__FUNCTION__),
		SessionSettings->NumPublicConnections,
		SessionSettings->bIsLANMatch ? TEXT("true") : TEXT("false"),
		SessionSettings->bShouldAdvertise ? TEXT("false") : TEXT("true"),
		SessionSettings->bIsDedicated ? TEXT("true") : TEXT("false"), SessionSettings->bUsesPresence ? TEXT("true") : TEXT("false"));

	// Listen for completion then call to CreateSession
	UILLLocalPlayer* FirstLocalPlayer = World ? Cast<UILLLocalPlayer>(World->GetFirstLocalPlayerFromController()) : nullptr;
	OnCreateGameSessionCompleteDelegateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(OnCreateGameSessionCompleteDelegate);
	if (!Sessions->CreateSession(FirstLocalPlayer ? FirstLocalPlayer->GetControllerId() : 0, NAME_GameSession, *SessionSettings))
	{
		// NOTE: No need to call HandleGameSessionCreationFailure here, because the delegate should fire when CreateSession returns false
		return false;
	}

	return true;
}

void UILLOnlineSessionClient::ResumeGameSession()
{
	UE_LOG(LogOnlineGame, Log, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	IOnlineSessionPtr Sessions = GetSessionInt();
	FNamedOnlineSession* Session = Sessions->GetNamedSession(NAME_GameSession);
	if (Session->SessionState == EOnlineSessionState::Pending)
	{
		// Start the game session
		// This only really hits on XB1 when you SmartMatch into a session together
		StartOnlineSession(NAME_GameSession);
	}
	else if (Session->SessionState != EOnlineSessionState::Creating && Session->SessionState != EOnlineSessionState::Starting)
	{
		checkf(Session->SessionState == EOnlineSessionState::InProgress, TEXT("%s: Unexpected session state '%s'"), ANSI_TO_TCHAR(__FUNCTION__), EOnlineSessionState::ToString(Session->SessionState));

		// Update the session to ensure the SETTING_BEACONPORT is updated
		SendGameSessionUpdate();
	}
}

bool operator !=(const FOnlineSessionSettings& LHS, const FOnlineSessionSettings& RHS)
{
	if (LHS.Settings.Num() != RHS.Settings.Num())
		return true;

	for (auto LeftIt(LHS.Settings.CreateConstIterator()); LeftIt; ++LeftIt)
	{
		const FName LeftKey = LeftIt.Key();
		const FOnlineSessionSetting& LeftSetting = LeftIt.Value();

		// Look for the same key in the right side
		if (const FOnlineSessionSetting* RightSetting = RHS.Settings.Find(LeftKey))
		{
			// Compare setting values
			if (!(*RightSetting == LeftSetting))
			{
				return true;
			}
		}
		else
		{
			// Not found!
			return true;
		}
	}

	return false;
}

void UILLOnlineSessionClient::SendGameSessionUpdate()
{
	// NOTE: May be called when there is no world, while a dedicated server is deferred!

	IOnlineSessionPtr Sessions = GetSessionInt();
	if (!Sessions.IsValid())
		return;

	FNamedOnlineSession* Session = Sessions->GetNamedSession(NAME_GameSession);
	if (!Session || Session->SessionState != EOnlineSessionState::InProgress)
		return;

	// Sometimes this can fire while we are joining a game session, so avoid that
	if (!IsHostingLobbyBeacon())
		return;
	if (IsJoiningGameSession())
		return;

	// Update the OSS
	// Apply general game session settings
	const FOnlineSessionSettings OldSettings = Session->SessionSettings;
	FOnlineSessionSettings NewSettings = OldSettings;
	UpdateGameSessionSettings(NewSettings);

	// Check if we should send them to the OSS, throttling is necessary on some
	const bool bSettingsChanged = (NewSettings != OldSettings);
	const float CurrentTime = FPlatformTime::Seconds();
	UILLBackendServer* BackendServer = GetOuterUILLGameInstance()->GetBackendRequestManager()->GetBackendServer();

#if USE_GAMELIFT_SERVER_SDK
	if (IsRunningDedicatedServer())
	{
		// Notify GameLift of open for matchmaking status changes
		const bool bOpenToMatchmaking = IsHostedGameSessionOpenToMatchmaking();
		if (bLastGameSessionOpenForMatchmaking != bOpenToMatchmaking)
		{
			UILLGameEngine* GameEngine = CastChecked<UILLGameEngine>(GEngine);
			FGameLiftServerSDKModule& GameLiftModule = GameEngine->GetGameLiftModule();
			GameLiftModule.UpdatePlayerSessionCreationPolicy(bOpenToMatchmaking ? EPlayerSessionCreationPolicy::ACCEPT_ALL : EPlayerSessionCreationPolicy::DENY_ALL);
			bLastGameSessionOpenForMatchmaking = bOpenToMatchmaking;
		}
	}
#endif // USE_GAMELIFT_SERVER_SDK

	// Send a backend report whenever settings change or we have gone LastGameSessionReportSeconds seconds without doing so
	const bool bSendRedundantBackendReport = (RedundantGameBackendReportInterval > 0.f && CurrentTime-LastGameSessionReportSeconds >= RedundantGameBackendReportInterval);
	const bool bSendBackendReport = BackendServer && (bSettingsChanged || bSendRedundantBackendReport);

	// Send a game session update to the platform when we have waited at least LastGameSessionUpdateSeconds since the last update and the settings are different or we have gone longer than RedundantGameSessionUpdateInterval seconds without doing so
	const bool bSendRedudantGameSessionUpdate = (RedundantGameSessionUpdateInterval > 0.f && CurrentTime-LastGameSessionUpdateSeconds >= RedundantGameSessionUpdateInterval);
	const bool bSendGameSessionUpdate = (bSettingsChanged || bSendRedudantGameSessionUpdate);
	if (!bSendBackendReport && !bSendGameSessionUpdate)
	{
		UE_LOG(LogOnlineGame, VeryVerbose, TEXT("%s: Suppressed"), ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}

	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: bSendBackendReport: %s, bSendGameSessionUpdate: %s"), ANSI_TO_TCHAR(__FUNCTION__), bSendBackendReport ? TEXT("true") : TEXT("false"), bSendGameSessionUpdate ? TEXT("true") : TEXT("false"));

	// Update the backend
	if (bSendBackendReport)
	{
		BackendServer->SendReport();
		LastGameSessionReportSeconds = CurrentTime;
	}

	// Update the online subsystem
	// NOTE: This also updates player pings/scores with Steam
	if (bSendGameSessionUpdate)
	{
		if (!Sessions->UpdateSession(NAME_GameSession, NewSettings, true))
		{
			UE_LOG(LogOnlineGame, Warning, TEXT("... failed to send game session update!"));
		}
		LastGameSessionUpdateSeconds = CurrentTime;
	}
}

bool UILLOnlineSessionClient::IsHostedGameSessionOpenToMatchmaking() const
{
	UWorld* World = GetWorld();
	AILLGameMode* GameMode = World ? World->GetAuthGameMode<AILLGameMode>() : nullptr;
	UILLGameEngine* GameEngine = Cast<UILLGameEngine>(GEngine);
	return ((GameMode && GameMode->IsOpenToMatchmaking()) || (GameEngine && !GameEngine->bHasStarted));
}

void UILLOnlineSessionClient::UpdateGameSessionSettings(FOnlineSessionSettings& InOutSessionSettings)
{
	UWorld* World = GetWorld();
	AILLGameMode* GameMode = World->GetAuthGameMode<AILLGameMode>();
	UILLGameEngine* GameEngine = Cast<UILLGameEngine>(GEngine);
	UILLOnlineMatchmakingClient* OMC = GetOnlineMatchmaking();

	// Arbitration port
	const int32 LobbyBeaconHostPort = GetLobbyBeaconHostListenPort();
	if (IsHostingLobbyBeacon())
		InOutSessionSettings.Set(SETTING_BEACONPORT, LobbyBeaconHostPort, EOnlineDataAdvertisementType::ViaOnlineService);

	const int32 OpenForMatchmaking = IsHostedGameSessionOpenToMatchmaking() ? 1 : 0;
	const int32 NumPlayers = FMath::Max(GameMode ? GameMode->GetNumPlayers() : 0, GetLobbyMemberList().Num());

#if PLATFORM_XBOXONE
	InOutSessionSettings.Set(SEARCH_KEYWORDS, FString::Printf(XBOXONE_QUICKPLAY_KEYWORDS_FORMAT, ILLBUILD_BUILD_NUMBER), EOnlineDataAdvertisementType::ViaOnlineService);
	InOutSessionSettings.Set(SETTING_MATCHING_TIMEOUT, XBOXONE_QUICKPLAY_MATCHING_TIMEOUT, EOnlineDataAdvertisementType::ViaOnlineService);
	if (InOutSessionSettings.bShouldAdvertise)
	{
		InOutSessionSettings.bAllowJoinInProgress = (OpenForMatchmaking != 0); // Update JIP for QuickPlay so matching tickets can cleanup
		InOutSessionSettings.Set(SETTING_SESSION_TEMPLATE_NAME, XBOXONE_QUICKPLAY_SESSIONTEMPLATE, EOnlineDataAdvertisementType::DontAdvertise);
		if (OMC->GetLastAggregateMatchmakingStatus() == EILLBackendMatchmakingStatusType::Ok)
			InOutSessionSettings.Set(SEARCH_XBOX_LIVE_HOPPER_NAME, XBOXONE_QUICKPLAY_HOPPER_STANDARD, EOnlineDataAdvertisementType::DontAdvertise);
		else
			InOutSessionSettings.Set(SEARCH_XBOX_LIVE_HOPPER_NAME, XBOXONE_QUICKPLAY_HOPPER_LOWPRIORITY, EOnlineDataAdvertisementType::DontAdvertise);
		InOutSessionSettings.Set(SETTING_MATCHING_ATTRIBUTES, FString::Printf(TEXT("{ \"BuildNumber\": %i }"), ILLBUILD_BUILD_NUMBER), EOnlineDataAdvertisementType::DontAdvertise);
	}
	else
	{
		InOutSessionSettings.Set(SETTING_SESSION_TEMPLATE_NAME, XBOXONE_PRIVATE_SESSIONTEMPLATE, EOnlineDataAdvertisementType::DontAdvertise);
	}
#else
	InOutSessionSettings.Set(SETTING_BUILDNUMBER, ILLBUILD_BUILD_NUMBER, EOnlineDataAdvertisementType::ViaOnlineService);
	InOutSessionSettings.Set(SETTING_OPENFORMM, OpenForMatchmaking, EOnlineDataAdvertisementType::ViaOnlineService);
	InOutSessionSettings.Set(SETTING_PLAYERCOUNT, NumPlayers, EOnlineDataAdvertisementType::ViaOnlineService);
	if (InOutSessionSettings.bShouldAdvertise)
	{
		InOutSessionSettings.Set(SETTING_PRIORITY, (OMC->GetLastAggregateMatchmakingStatus() == EILLBackendMatchmakingStatusType::Ok) ? SESSION_PRIORITY_OK : SESSION_PRIORITY_LOW, EOnlineDataAdvertisementType::ViaOnlineService);
	}
#endif

	// Report a game mode if present, otherwise assume "Lobby"
	if (GameMode)
	{
		check(!GameMode->ModeName.IsEmpty());
	}
	InOutSessionSettings.Set(SETTING_GAMEMODE, GameMode ? GameMode->ModeName : FString(TEXT("Lobby")), EOnlineDataAdvertisementType::ViaOnlineService);

	// Support a region on desktop platforms
#if PLATFORM_DESKTOP
	if (IsRunningDedicatedServer())
	{
		FString ServerRegion;
		FParse::Value(FCommandLine::Get(), TEXT("ServerRegion="), ServerRegion);
		if (!ServerRegion.IsEmpty())
			InOutSessionSettings.Set(SETTING_REGION, ServerRegion, EOnlineDataAdvertisementType::DontAdvertise); // Do not take up tag space with this
	}
#endif

	// Report the current map, otherwise assume server default map
	FString MapName;
	if (GameEngine && GameEngine->bHasStarted)
	{
		MapName = World->GetMapName();
	}
	else
	{
		MapName = UGameMapsSettings::GetServerDefaultMap();
	}
	InOutSessionSettings.Set(SETTING_MAPNAME, FPackageName::GetShortName(MapName), EOnlineDataAdvertisementType::ViaOnlineService);
}

void UILLOnlineSessionClient::OnCreateGameSessionComplete(FName InSessionName, bool bWasSuccessful)
{
	if (InSessionName != NAME_GameSession)
		return;

	UE_LOG(LogOnlineGame, Log, TEXT("%s: %s bSuccess: %d"), ANSI_TO_TCHAR(__FUNCTION__), *InSessionName.ToString(), bWasSuccessful);

	IOnlineSessionPtr Sessions = GetSessionInt();
	check(OnCreateGameSessionCompleteDelegateHandle.IsValid());
	Sessions->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateGameSessionCompleteDelegateHandle);

	if (bWasSuccessful)
	{
		// For some reason this is in Super::HandleMatchHasStarted, so if you don't want your "match" to start until the session starts you're fucked, brilliant
		FNamedOnlineSession* Session = Sessions->GetNamedSession(InSessionName);
		if (Session->SessionState == EOnlineSessionState::Pending)
		{
			// Start the game session
			StartOnlineSession(NAME_GameSession);
		}
		else
		{
			UE_LOG(LogOnlineGame, Warning, TEXT("%s: Skipping session start for %s session in state %s"), ANSI_TO_TCHAR(__FUNCTION__), *InSessionName.ToString(), EOnlineSessionState::ToString(Session->SessionState));
		}
	}
	else
	{
		// Error fall-through, cleanup!
		HandleGameSessionCreationFailure(LOCTEXT("GameSessionCreationError_CreateFailed", "Failed to create game session! Verify you have an internet connection."));
	}
}

void UILLOnlineSessionClient::OnDestroyGameSessionComplete(FName InSessionName, bool bWasSuccessful)
{
	if (InSessionName != NAME_GameSession)
		return;

	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: %s bSuccess: %d"), ANSI_TO_TCHAR(__FUNCTION__), *InSessionName.ToString(), bWasSuccessful);

	IOnlineSessionPtr Sessions = GetSessionInt();
	check(OnDestroyGameSessionCompleteDelegateHandle.IsValid());
	Sessions->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroyGameSessionCompleteDelegateHandle);

	// Ensure this is cleaned up
	ClearPendingGameSession();
}

void UILLOnlineSessionClient::OnDestroyForGameSessionCreationFailure(FName SessionName, bool bWasSuccessful)
{
	if (SessionName != NAME_GameSession)
		return;

	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));
	check(!IsInLobbyBeacon());
	
	IOnlineSessionPtr Sessions = GetSessionInt();
	check(OnDestroyForGameSessionCreationFailureCompleteDelegateHandle.IsValid());
	Sessions->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroyForGameSessionCreationFailureCompleteDelegateHandle);

	// Return to the main menu
	OnDestroyForMainMenuComplete(SessionName, bWasSuccessful);
}

void UILLOnlineSessionClient::HandleGameSessionCreationFailure(const FText& FailureReason)
{
	UE_LOG(LogOnlineGame, Error, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *FailureReason.ToString());

	// Cleanup our beacon host, if any
	TeardownLobbyBeaconHost();

	if (IsRunningDedicatedServer())
	{
		// Dedicated servers just exit immediately
		GIsRequestingExit = true;
	}
	else
	{
		// Queue an error prompt
		GetOuterUILLGameInstance()->OnGameSessionCreateFailurePreTravel(FailureReason);

		// Destroy the game session, if present, then return to the main menu
		DestroyExistingSession_Impl(OnDestroyForGameSessionCreationFailureCompleteDelegateHandle, NAME_GameSession, OnDestroyForGameSessionCreationFailureCompleteDelegate);
	}
}

void UILLOnlineSessionClient::FinishLobbyTravel()
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Check for a backend login
	UWorld* World = GetWorld();
	UILLLocalPlayer* FirstLocalPlayer = Cast<UILLLocalPlayer>(World->GetFirstLocalPlayerFromController());
	if (!FirstLocalPlayer || !FirstLocalPlayer->GetBackendPlayer() || !FirstLocalPlayer->GetBackendPlayer()->IsLoggedIn())
	{
		UE_LOG(LogOnlineGame, Error, TEXT("%s: Local player not signed into backend!"), ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}

	// Cleanup our travel listener
	LobbyClientBeacon->OnBeaconTravelAcked().Unbind();

	// Modify LobbyTravelURL first
	FString PendingTravelString = LobbyTravelURL;

	// Tell them if this was from an invite
	if (bIsFromInvite)
	{
		PendingTravelString += TEXT("?bIsFromInvite");
		bIsFromInvite = false;
	}

	// Pass along the password so we can get in
	if (!CachedSessionPassword.IsEmpty())
	{
		PendingTravelString += TEXT("?Password=");
		PendingTravelString += CachedSessionPassword;
		CachedSessionPassword = FString();
	}

	// Tack on additional options
	if (!CachedGameTravelOptions.IsEmpty())
	{
		if (!CachedGameTravelOptions.StartsWith(TEXT("?")))
			PendingTravelString += TEXT("?");
		PendingTravelString += CachedGameTravelOptions;
		CachedGameTravelOptions = FString();
	}

	// Pass along an encryption token
	const FString& EncryptionToken = LobbyClientBeacon->GetSessionKeySet().EncryptionToken;
	if (!EncryptionToken.IsEmpty())
	{
		PendingTravelString += TEXT("?EncryptionToken=");
		PendingTravelString += EncryptionToken;
	}

	// Trigger the lobby join session callback
	CachedGameSessionCallback.ExecuteIfBound(NAME_GameSession, EOnJoinSessionCompleteResult::Success);
	ClearPendingGameSession();

	// Now begin the actual travel
	FirstLocalPlayer->GetPlayerController(World)->ClientTravel(PendingTravelString, TRAVEL_Absolute);
}

AILLLobbyBeaconState* UILLOnlineSessionClient::GetLobbyBeaconState() const
{
	if (IsHostingLobbyBeacon())
	{
		return Cast<AILLLobbyBeaconState>(LobbyHostBeaconObject->GetSessionBeaconState());
	}
	else if (IsInLobbyBeacon())
	{
		return Cast<AILLLobbyBeaconState>(LobbyClientBeacon->GetSessionBeaconState());
	}

	return nullptr;
}

int32 UILLOnlineSessionClient::GetNumAcceptedLobbyMembers() const
{
	if (GetLobbyBeaconState())
	{
		return GetLobbyBeaconState()->GetNumAcceptedMembers();
	}

	return 0;
}

bool UILLOnlineSessionClient::IsHostingLobbyBeacon() const
{
	return (LobbyBeaconHost && LobbyHostBeaconObject && LobbyHostBeaconObject->IsInitialized());
}

bool UILLOnlineSessionClient::IsInLobbyBeacon() const
{
	return (LobbyClientBeacon && LobbyClientBeacon->IsReservationAccepted());
}

void UILLOnlineSessionClient::OnLobbyBeaconEstablished()
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	AILLLobbyBeaconState* LobbyBeaconState = GetLobbyBeaconState();
	check(LobbyBeaconState);

	// Listen for new member additions/removals
	LobbyBeaconState->OnBeaconMemberAdded().AddUObject(this, &ThisClass::OnLobbyBeaconMemberAdded);
	LobbyBeaconState->OnBeaconMemberRemoved().AddUObject(this, &ThisClass::OnLobbyBeaconMemberRemoved);

	// Add our current members
	for (int32 MemberIndex = 0; MemberIndex < LobbyBeaconState->GetNumMembers(); ++MemberIndex)
	{
		// LobbyMember may be NULL on clients (not replicated yet) but OnBeaconMemberAdded should catch that later
		if (AILLLobbyBeaconMemberState* LobbyMember = Cast<AILLLobbyBeaconMemberState>(LobbyBeaconState->GetMemberAtIndex(MemberIndex)))
		{
			OnLobbyBeaconMemberAdded(LobbyMember);
		}
	}
}

void UILLOnlineSessionClient::OnLobbyBeaconMemberAdded(AILLSessionBeaconMemberState* MemberState)
{
	AILLLobbyBeaconMemberState* LBS = Cast<AILLLobbyBeaconMemberState>(MemberState);
	check(LBS);

	LobbyMemberList.Add(LBS);
	LobbyMemberAdded.Broadcast(LBS);

	// "Start" the server
	if (IsRunningDedicatedServer())
	{
		UILLGameEngine* GameEngine = Cast<UILLGameEngine>(GEngine);
		if (!GameEngine->bHasStarted)
		{
			GameEngine->ForceStart();
		}
	}
}

void UILLOnlineSessionClient::OnLobbyBeaconMemberRemoved(AILLSessionBeaconMemberState* MemberState)
{
	AILLLobbyBeaconMemberState* LBS = Cast<AILLLobbyBeaconMemberState>(MemberState);
	check(LBS);

	LobbyMemberList.Remove(LBS);
	LobbyMemberRemoved.Broadcast(LBS);
}

void UILLOnlineSessionClient::FullApproveLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
#if WITH_EDITOR
	// Don't need to worry about logging in when playing in Editor
	if (GetWorld() && GetWorld()->IsPlayInEditor())
		return;
#endif

	if (!IsHostingLobbyBeacon())
	{
		ErrorMessage = TEXT("Beacon not initialized!");
		return;
	}

	// Check for a reservation for this player
	const FILLBackendPlayerIdentifier AccountId = FILLBackendPlayerIdentifier(UGameplayStatics::ParseOption(Options, TEXT("AccountId")));
	AILLLobbyBeaconMemberState* MemberState = Cast<AILLLobbyBeaconMemberState>(LobbyHostBeaconObject->GetSessionBeaconState()->FindMember(AccountId));
	if (!MemberState)
		MemberState = Cast<AILLLobbyBeaconMemberState>(LobbyHostBeaconObject->GetSessionBeaconState()->FindMember(UniqueId));
	if (!MemberState)
	{
		ErrorMessage = TEXT("Lobby beacon reservation not found!");
		return;
	}

	// Verify they have fully authorized before joining
	if (!MemberState->GetBeaconClientActor())
	{
		ErrorMessage = TEXT("Authorization failed!");
		return;
	}
}

int32 UILLOnlineSessionClient::GetLobbyBeaconHostListenPort() const
{
	return IsHostingLobbyBeacon() ? LobbyBeaconHost->GetListenPort() : 0;
}

void UILLOnlineSessionClient::PauseLobbyHostBeaconRequests(const bool bPause)
{
	if (IsHostingLobbyBeacon())
	{
		UE_LOG(LogOnlineGame, Log, TEXT("%s: bPause: %s"), ANSI_TO_TCHAR(__FUNCTION__), bPause ? TEXT("true") : TEXT("false"));
		LobbyBeaconHost->PauseBeaconRequests(bPause);
	}
}

bool UILLOnlineSessionClient::SpawnLobbyBeaconHost(const FOnlineSessionSettings& SessionSettings)
{
	if (LobbyBeaconHost && LobbyHostBeaconObject)
	{
		return true;
	}

	const int32 MaxPlayers = SessionSettings.NumPublicConnections + SessionSettings.NumPrivateConnections;
	UE_LOG(LogOnlineGame, Log, TEXT("%s: MaxPlayers: %i"), ANSI_TO_TCHAR(__FUNCTION__), MaxPlayers);

	FActorSpawnParameters LobbyActorSpawnParams;
	LobbyActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	LobbyActorSpawnParams.ObjectFlags |= EObjectFlags::RF_Transient;

	// Spawn the lobby host listener net driver
	LobbyBeaconHost = GetOrCreateLobbyBeaconWorld()->SpawnActor<AILLLobbyBeaconHost>(LobbyBeaconHostClass, LobbyActorSpawnParams);
	if (!LobbyBeaconHost->InitHost(SessionSettings))
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("%s: Could not initialize the host listener!"), ANSI_TO_TCHAR(__FUNCTION__));
		return false;
	}

	// Spawn the lobby host beacon
	LobbyHostBeaconObject = GetOrCreateLobbyBeaconWorld()->SpawnActor<AILLLobbyBeaconHostObject>(LobbyHostObjectClass, LobbyActorSpawnParams);

	// Register it so that it can network
	LobbyBeaconHost->RegisterHost(LobbyHostBeaconObject);

	// Initialize the host beacon
	UWorld* World = GetWorld();
	AILLPartyBeaconState* HostingPartyState = IsInPartyAsLeader() ? GetPartyBeaconState() : nullptr;
	UILLLocalPlayer* FirstLocalPlayer = Cast<UILLLocalPlayer>(World->GetFirstLocalPlayerFromController());
	if (!LobbyHostBeaconObject->InitLobbyBeacon(MaxPlayers, HostingPartyState, FirstLocalPlayer))
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("%s: Failed to initialize the LobbyHostBeaconObject!"), ANSI_TO_TCHAR(__FUNCTION__));
		TeardownLobbyBeaconHost();
		return false;
	}

	// Handle the beacon state
	OnLobbyBeaconEstablished();

	return true;
}

void UILLOnlineSessionClient::TeardownLobbyBeaconHost()
{
	if (LobbyBeaconHost || LobbyHostBeaconObject)
	{
		UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));
	}

	if (LobbyBeaconHost)
	{
		if (LobbyHostBeaconObject)
		{
			// Unregister the LobbyHostBeaconObject, disconnecting all clients
			LobbyBeaconHost->UnregisterHost(LobbyHostBeaconObject->GetBeaconType());
		}
		LobbyBeaconHost->DestroyBeacon();
		LobbyBeaconHost = nullptr;
	}

	if (LobbyHostBeaconObject)
	{
		LobbyHostBeaconObject->Destroy();
		LobbyHostBeaconObject = nullptr;
	}

	LobbyMemberList.Empty();
}

void UILLOnlineSessionClient::BeginRequestLobbySession(const FString& ConnectionString, const FString& PlayerSessionId, FILLOnLobbyBeaconSessionReceived ReceivedDelegate, FILLOnLobbyBeaconSessionRequestComplete CompletionDelegate)
{
	UE_LOG(LogOnlineGame, Log, TEXT("%s: ConnectionString: %s, PlayerSessionId: %s"), ANSI_TO_TCHAR(__FUNCTION__), *ConnectionString, *PlayerSessionId);

	// Cache these off in case we have to wait for game session deletion below
	LobbyBeaconSessionReceivedDelegate = ReceivedDelegate;
	LobbyBeaconSessionRequestCompleteDelegate = CompletionDelegate;

	IOnlineSessionPtr Sessions = GetSessionInt();
	check(Sessions->GetSessionState(NAME_GameSession) == EOnlineSessionState::NoSession);

	// Destroy and create a new lobby beacon client
	TeardownLobbyBeaconClient();
	SpawnLobbyBeaconClient();

	// Connect and perform a lobby session request
	UWorld* World = GetWorld();
	UILLLocalPlayer* FirstLocalPlayer = Cast<UILLLocalPlayer>(World->GetFirstLocalPlayerFromController());
	LobbyClientBeacon->OnLobbyBeaconSessionReceived().BindUObject(this, &ThisClass::OnLobbyClientSessionReceived);
	LobbyClientBeacon->OnLobbyBeaconSessionRequestComplete().BindUObject(this, &ThisClass::OnLobbyClientSessionRequestComplete);
	LobbyClientBeacon->RequestLobbySession(ConnectionString, FirstLocalPlayer, PlayerSessionId);
}

bool UILLOnlineSessionClient::BeginRequestPartyLobbySession(const TArray<FILLMatchmadePlayerSession>& PlayerSessions, FILLOnLobbyBeaconSessionReceived ReceivedDelegate, FILLOnLobbyBeaconSessionRequestComplete CompletionDelegate)
{
	UE_LOG(LogOnlineGame, Log, TEXT("%s: PlayerSessions: %i"), ANSI_TO_TCHAR(__FUNCTION__), PlayerSessions.Num());

	// TODO: pjackson: Verify PlayerIds match up with our party and skip if not?

	UWorld* World = GetWorld();
	UILLLocalPlayer* FirstLocalPlayer = Cast<UILLLocalPlayer>(World->GetFirstLocalPlayerFromController());
	if (!FirstLocalPlayer || !FirstLocalPlayer->GetBackendPlayer() || !FirstLocalPlayer->GetBackendPlayer()->IsLoggedIn())
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("%s: Local player is not logged into the backend!"), ANSI_TO_TCHAR(__FUNCTION__));
		CompletionDelegate.ExecuteIfBound(EILLLobbyBeaconSessionResult::NotSignedIn);
		return false;
	}

	// Find ours and join on it
	int32 LocalPlayerIndex = INDEX_NONE;
	for (int32 Index = 0; Index < PlayerSessions.Num(); ++Index)
	{
		const FILLMatchmadePlayerSession& PlayerSession = PlayerSessions[Index];
		if (PlayerSession.AccountId == FirstLocalPlayer->GetBackendPlayer()->GetAccountId())
		{
			LocalPlayerIndex = Index;
			break;
		}
	}
	if (LocalPlayerIndex == INDEX_NONE)
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("%s: Failed to find LocalPlayerIndex!"), ANSI_TO_TCHAR(__FUNCTION__));
		CompletionDelegate.ExecuteIfBound(EILLLobbyBeaconSessionResult::FailedToFindLocalPlayer);
		return false;
	}

	// Store off the PlayerSessions for use in OnLobbyClientSessionReceived
	PartyPlayerSessions = PlayerSessions;

	// Begin local travel, party members will be brought along in OnLobbyClientSessionReceived
	const FILLMatchmadePlayerSession& LocalPlayerSession = PartyPlayerSessions[LocalPlayerIndex];
	BeginRequestLobbySession(LocalPlayerSession.ConnectionString, LocalPlayerSession.PlayerSessionId, ReceivedDelegate, CompletionDelegate);
	return true;
}

void UILLOnlineSessionClient::OnLobbyClientSessionReceived(const FString& SessionId)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	if (!IsInPartyAsLeader())
		return;

	auto GetPlayerSessionFor = [this](const FILLBackendPlayerIdentifier& AccountId) -> FILLMatchmadePlayerSession*
	{
		for (int32 Index = 0; Index < PartyPlayerSessions.Num(); ++Index)
		{
			if (PartyPlayerSessions[Index].AccountId == AccountId)
			{
				return &PartyPlayerSessions[Index];
			}
		}

		return nullptr;
	};

	// Notify all party members of our progress joining this session so they can start
	if (AILLPartyBeaconState* PartyBeaconState = GetPartyBeaconState())
	{
		for (int32 MemberIndex = 0; MemberIndex < PartyBeaconState->GetNumMembers(); ++MemberIndex)
		{
			if (AILLPartyBeaconMemberState* PartyMember = Cast<AILLPartyBeaconMemberState>(PartyBeaconState->GetMemberAtIndex(MemberIndex)))
			{
				// We only care about members (clients), aka not ourself
				if (AILLPartyBeaconClient* PartyClient = Cast<AILLPartyBeaconClient>(PartyMember->GetBeaconClientActor()))
				{
					if (FILLMatchmadePlayerSession* PlayerSession = GetPlayerSessionFor(PartyMember->GetAccountId()))
					{
						// Tell them to start their own BeginRequestLobbySession sequence
						PartyClient->ClientMatchmakingFollow(SessionId, PlayerSession->ConnectionString, PlayerSession->PlayerSessionId);
					}
					else
					{
						UE_LOG(LogOnlineGame, Warning, TEXT("%s: Could not find a player session entry for member %s!"), ANSI_TO_TCHAR(__FUNCTION__), *PartyMember->ToDebugString());

						// TODO: pjackson: Kick them? Right now it will just attempt to find the game session when they receive JoinLeadersGameSession later, which will fall back no PlayerSessionId reservations to the bypass endpoint
					}
				}
			}
		}
	}

	// Cleanup the list so it's not used again incorrectly
	PartyPlayerSessions.Empty();

	// Notify listeners
	LobbyBeaconSessionReceivedDelegate.ExecuteIfBound(SessionId);
}

void UILLOnlineSessionClient::OnLobbyClientSessionRequestComplete(const EILLLobbyBeaconSessionResult Response)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), ToString(Response));

	// No longer need to list for this
	LobbyClientBeacon->OnLobbyBeaconSessionRequestComplete().Unbind();

	if (Response != EILLLobbyBeaconSessionResult::Success)
	{
		// Make sure we completely leave the lobby and cleanup our pending game session info (including CachedGameSessionCallback)
		TeardownLobbyBeaconClient();
	}

	// Notify listeners
	LobbyBeaconSessionRequestCompleteDelegate.ExecuteIfBound(Response);
}

void UILLOnlineSessionClient::OnLobbyClientHostConnectionFailure()
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	const EOnJoinSessionCompleteResult::Type Result = EOnJoinSessionCompleteResult::CouldNotRetrieveAddress;

	// Check if we are in the process of joining
	if (CachedGameSessionCallback.IsBound())
	{
		// Happened during the process of joining the game
		// Trigger the lobby join session callback
		CachedGameSessionCallback.ExecuteIfBound(NAME_GameSession, Result);

		// Make sure we completely leave the lobby and cleanup our pending game session info (including CachedGameSessionCallback)
		TeardownLobbyBeaconClient();
	}
	else if (LobbyClientBeacon && LobbyClientBeacon->OnLobbyBeaconSessionRequestComplete().IsBoundToObject(this))
	{
		OnLobbyClientSessionRequestComplete(EILLLobbyBeaconSessionResult::FailedToBind);
	}
	else
	{
		// Leave NAME_GameSession
		// OnDestroyGameForMainMenuComplete will take care of TeardownLobbyBeaconClient
		DestroyExistingSession_Impl(OnDestroyGameForMainMenuCompleteDelegateHandle, NAME_GameSession, OnDestroyGameForMainMenuCompleteDelegate);
	}
}

void UILLOnlineSessionClient::OnLobbyClientKicked(const FText& KickReason)
{
	UE_LOG(LogOnlineGame, Log, TEXT("%s: \"%s\""), ANSI_TO_TCHAR(__FUNCTION__), *KickReason.ToString());

	// Store our kick reason off
	if (KickReason.IsEmpty())
	{
		GetOuterUILLGameInstance()->SetLastPlayerKickReason(LOCTEXT("LobbyKickedWithoutReason", "You were kicked!"));
	}
	else
	{
		GetOuterUILLGameInstance()->SetLastPlayerKickReason(FText::Format(LOCTEXT("LobbyKickedWithReason", "You were kicked: {0}"), KickReason));
	}

	// Leave our party if we are only a member
	if (IsJoiningParty() || IsInPartyAsMember())
	{
		LeavePartySession();
	}

	// Tear down our client now, because we will be kicked immediately after this message anyways, no sense in telling the server we are leaving and waiting on the ack
	TeardownLobbyBeaconClient();

	// Destroy the game session and return to the main menu
	DestroyExistingSession_Impl(OnDestroyGameForMainMenuCompleteDelegateHandle, NAME_GameSession, OnDestroyGameForMainMenuCompleteDelegate);
}

void UILLOnlineSessionClient::OnLobbyClientReservationAccepted()
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));
	
	// This may be hit as a fall-through option when there is no lobby beacon
	if (LobbyClientBeacon)
	{
		// Handle the beacon state
		OnLobbyBeaconEstablished();
	}

	if (IsInPartyAsLeader())
	{
		// Begin party travel coordination
		PartyHostBeaconObject->OnPartyTravelCoordinationComplete().BindUObject(this, &ThisClass::OnPartyHostTravelCoordinationComplete);
		ensureAlways(PartyHostBeaconObject->NotifyLobbyReservationAccepted());
	}
	else
	{
		// Fake a coordination completion to kick off local travel
		OnPartyHostTravelCoordinationComplete();
	}
}

void UILLOnlineSessionClient::OnLobbyClientReservationRequestFailed(const EILLSessionBeaconReservationResult::Type Response)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: Response: %s"), ANSI_TO_TCHAR(__FUNCTION__), *EILLSessionBeaconReservationResult::GetLocalizedFailureText(Response).ToString());

	// Hide any previous loading screen
	GetOuterUILLGameInstance()->OnSessionReservationFailed(NAME_GameSession, Response);

	// Trigger the lobby join session callback
	EOnJoinSessionCompleteResult::Type TranslatedError = EOnJoinSessionCompleteResult::UnknownError;
	switch (Response)
	{
	case EILLSessionBeaconReservationResult::Success: TranslatedError = EOnJoinSessionCompleteResult::Success; break;
	case EILLSessionBeaconReservationResult::NotInitialized: TranslatedError = EOnJoinSessionCompleteResult::SessionDoesNotExist; break;
	case EILLSessionBeaconReservationResult::NoSession: TranslatedError = EOnJoinSessionCompleteResult::SessionDoesNotExist; break;
	case EILLSessionBeaconReservationResult::InvalidReservation: TranslatedError = EOnJoinSessionCompleteResult::CouldNotRetrieveAddress; break; // FIXME: pjackson: It's a stretch...
	case EILLSessionBeaconReservationResult::Denied: TranslatedError = EOnJoinSessionCompleteResult::SessionDoesNotExist; break;
	case EILLSessionBeaconReservationResult::GameFull: TranslatedError = EOnJoinSessionCompleteResult::SessionIsFull; break;
	case EILLSessionBeaconReservationResult::PartyFull: break; // Shouldn't happen
	case EILLSessionBeaconReservationResult::AuthFailed: TranslatedError = EOnJoinSessionCompleteResult::AuthFailed; break;
	case EILLSessionBeaconReservationResult::AuthTimedOut: TranslatedError = EOnJoinSessionCompleteResult::AuthTimedOut; break;
	case EILLSessionBeaconReservationResult::BypassFailed: TranslatedError = EOnJoinSessionCompleteResult::BackendAuthFailed; break;
	case EILLSessionBeaconReservationResult::MatchmakingClosed: break; // Shouldn't happen
	}
	CachedGameSessionCallback.ExecuteIfBound(NAME_GameSession, TranslatedError);

	// Make sure we completely leave the lobby and cleanup CachedGameSessionCallback
	TeardownLobbyBeaconClient();
}

void UILLOnlineSessionClient::SpawnLobbyBeaconClient()
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));
	check(!LobbyClientBeacon);

	FActorSpawnParameters ClientBeaconSpawnParams;
	ClientBeaconSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ClientBeaconSpawnParams.ObjectFlags |= EObjectFlags::RF_Transient;
	LobbyClientBeacon = GetOrCreateLobbyBeaconWorld()->SpawnActor<AILLLobbyBeaconClient>(LobbyClientClass, ClientBeaconSpawnParams);
	LobbyClientBeacon->OnHostConnectionFailure().BindUObject(this, &ThisClass::OnLobbyClientHostConnectionFailure);
	LobbyClientBeacon->OnBeaconClientKicked().BindUObject(this, &ThisClass::OnLobbyClientKicked);
	LobbyClientBeacon->OnBeaconReservationAccepted().BindUObject(this, &ThisClass::OnLobbyClientReservationAccepted);
	LobbyClientBeacon->OnBeaconReservationRequestFailed().BindUObject(this, &ThisClass::OnLobbyClientReservationRequestFailed);
}

void UILLOnlineSessionClient::TeardownLobbyBeaconClient()
{
	if (LobbyClientBeacon || !LobbyTravelURL.IsEmpty() || LobbyMemberList.Num() > 0)
	{
		UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));
	}

	if (LobbyClientBeacon)
	{
		// Cleanup the client beacon
		LobbyClientBeacon->DestroyBeacon();
		LobbyClientBeacon = nullptr;
	}

	LobbyTravelURL = FString();
	LobbyMemberList.Empty();

	// Cleanup the CachedSessionResult so IsJoiningGameSession no longer returns true
	ClearPendingGameSession();
}

bool UILLOnlineSessionClient::CanCreateParty()
{
	UWorld* World = GetWorld();
	if (!World)
		return false;

	// Disallow parties while offline
	if (AILLPlayerController* PC = Cast<AILLPlayerController>(GetOuterUILLGameInstance()->GetFirstLocalPlayerController()))
	{
		if (UILLBackendBlueprintLibrary::IsInOfflineMode(PC))
			return false;
		if (!UILLGameOnlineBlueprintLibrary::HasPlayOnlinePrivilege(PC))
			return false;
	}
	if (GAntiCheatBlockingOnlineMatches)
		return false;

	// Disallow party creation while matchmaking
	UILLOnlineMatchmakingClient* OMC = GetOnlineMatchmaking();
	if (OMC->IsMatchmaking())
		return false;

	// Disallow party creation while creating/leading one
	if (IsCreatingParty() || IsInPartyAsLeader())
		return false;

	// ... while joining/in one
	if (IsJoiningParty() || IsInPartyAsMember())
		return false;

	// ... or while joining a game session
	if (IsJoiningGameSession())
		return false;

	// ... or while a party session still exists
	IOnlineSessionPtr Sessions = GetSessionInt();
	const EOnlineSessionState::Type CurrentSessionState = Sessions->GetSessionState(NAME_PartySession);
	if (CurrentSessionState != EOnlineSessionState::NoSession)
		return false;

	return true;
}

bool UILLOnlineSessionClient::CanInviteToParty()
{
	// Only allow leaders and members obviously
	if (!IsInPartyAsLeader() && !IsInPartyAsMember())
		return false;

	// Wait until they are settled into the game session first
	if (IsJoiningGameSession())
		return false;

	return true;
}

bool UILLOnlineSessionClient::CreateParty()
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Check IsJoiningParty because that checks if ANY of the party actors are present
	// This function attempts to create the session and/or create the party beacon actors and set them up for it
	if (!CanCreateParty())
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("%s: Can not create a party at this time!"), ANSI_TO_TCHAR(__FUNCTION__));
		OnCreatePartyComplete().ExecuteIfBound(false);
		return false;
	}

	// Ensure this is reset
	bPartyBeaconEstablished = false;
	
	// Send an arbiter heartbeat (or wait for an existing pending one to finish)
	UWorld* World = GetWorld();
	UILLLocalPlayer* FirstLocalPlayer = Cast<UILLLocalPlayer>(World->GetFirstLocalPlayerFromController());
	if (FirstLocalPlayer->GetBackendPlayer() && FirstLocalPlayer->GetBackendPlayer()->IsLoggedIn())
	{
		// Wait for the heartbeat to complete before continuing party creation
		FirstLocalPlayer->GetBackendPlayer()->OnBackendPlayerArbiterHeartbeatComplete().AddUObject(this, &ThisClass::OnCreatePartyBackendPlayerHeartbeat);
		FirstLocalPlayer->GetBackendPlayer()->SendArbiterHeartbeat();
	}
	else
	{
		// Failed to send, cleanup!
		TeardownPartyBeaconHost();

		// Notify our listeners
		OnCreatePartyComplete().ExecuteIfBound(false);
		return false;
	}

	return true;
}

bool UILLOnlineSessionClient::DestroyPartySession()
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	if (!IsInPartyAsLeader())
		return false;

	// Stop matchmaking if in-progress
	UILLOnlineMatchmakingClient* OMC = GetOnlineMatchmaking();
	OMC->CancelMatchmaking();

	// Notify all members about the shutdown
	PartyHostBeaconObject->OnBeaconShutdownAcked().BindUObject(this, &ThisClass::OnPartyHostShutdownAcked);
	PartyHostBeaconObject->StartShutdown();
	return true;
}

bool UILLOnlineSessionClient::DestroyPartyForMainMenu(FText NetworkFailureReason)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *NetworkFailureReason.ToString());

	if (!IsInPartyAsLeader())
		return false;

	// Stop matchmaking if in-progress
	UILLOnlineMatchmakingClient* OMC = GetOnlineMatchmaking();
	OMC->CancelMatchmaking();

	// TODO: pjackson: This path is untested!
	// Destroy NAME_PartySession then return to the main menu
	GetOuterUILLGameInstance()->OnPartyNetworkFailurePreTravel(NetworkFailureReason);
	DestroyExistingSession_Impl(OnDestroyForMainMenuCompleteDelegateHandle, NAME_PartySession, OnDestroyForMainMenuCompleteDelegate);

	return true;
}

bool UILLOnlineSessionClient::LeavePartySession()
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));
	
	// TEMP CALLSTACK! Print callstack so we can see what's requesting us to leave the party session
	{
		const SIZE_T StackTraceSize = 65535;
		ANSICHAR* StackTrace = (ANSICHAR*)FMemory::Malloc(StackTraceSize);
		StackTrace[0] = 0;

		// Walk the stack and dump it to the allocated memory.
		FPlatformStackWalk::StackWalkAndDump(StackTrace, StackTraceSize, 0);
		UE_LOG(LogOnlineGame, Log, TEXT("%s"), ANSI_TO_TCHAR(StackTrace));
		FMemory::Free(StackTrace); 
	}

	// Throttle repeated requests
	if (IsValid(PartyClientBeacon) && PartyClientBeacon->IsPendingLeave())
	{
		return true;
	}

	// Stop matchmaking if in-progress
	UILLOnlineMatchmakingClient* OMC = GetOnlineMatchmaking();
	OMC->CancelMatchmaking();

	if (!IsInPartyAsMember())
	{
		// We are not fully in the party
		// Skip straight to FinishLeavingPartySession, which cleans up the session
		FinishLeavingPartySession();
		return false;
	}

	// NOTE: We do not care about what StartLeaving returns, because it only fails when we are already attempting to leave
	PartyClientBeacon->OnBeaconLeaveAcked().BindUObject(this, &ThisClass::OnPartyClientLeaveAcked);
	PartyClientBeacon->StartLeaving();
	return true;
}

bool UILLOnlineSessionClient::LeavePartyForMainMenu(FText NetworkFailureReason)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *NetworkFailureReason.ToString());

	// Throttle repeated requests
	if (IsValid(PartyClientBeacon) && PartyClientBeacon->IsPendingLeave())
	{
		return true;
	}

	// Stop matchmaking if in-progress
	UILLOnlineMatchmakingClient* OMC = GetOnlineMatchmaking();
	OMC->CancelMatchmaking();

	// Store the failure reason
	if (!NetworkFailureReason.IsEmpty())
	{
		GetOuterUILLGameInstance()->OnPartyNetworkFailurePreTravel(NetworkFailureReason);
	}

	if (!IsInPartyAsMember())
	{
		// We are not fully in the party
		// Skip straight to FinishLeavingPartySession, which cleans up the session
		FinishLeavingPartyForMainMenu();
		return false;
	}

	// NOTE: We do not care about what StartLeaving returns, because it only fails when we are already attempting to leave
	PartyClientBeacon->OnBeaconLeaveAcked().BindUObject(this, &ThisClass::FinishLeavingPartyForMainMenu);
	PartyClientBeacon->StartLeaving();
	return true;
}

int32 UILLOnlineSessionClient::GetPartySize(const bool bEvenWhenNotInOne/* = true*/) const
{
	if (AILLPartyBeaconState* BeaconState = GetPartyBeaconState())
	{
		return BeaconState->GetNumMembers();
	}

	if (bEvenWhenNotInOne)
	{
		return 1; // FIXME: pjackson: Assume we have one local player
	}

	return 0;
}

AILLPartyBeaconState* UILLOnlineSessionClient::GetPartyBeaconState() const
{
	if (IsInPartyAsLeader())
	{
		return Cast<AILLPartyBeaconState>(PartyHostBeaconObject->GetSessionBeaconState());
	}
	else if (IsInPartyAsMember())
	{
		return Cast<AILLPartyBeaconState>(PartyClientBeacon->GetSessionBeaconState());
	}

	return nullptr;
}

bool UILLOnlineSessionClient::HasPendingPartyMembers() const
{
	if (IsInPartyAsLeader())
	{
		return GetPartyBeaconState()->HasAnyPendingMembers();
	}

	return false;
}

bool UILLOnlineSessionClient::IsCreatingParty() const
{
	return (PartyBeaconHost || PartyHostBeaconObject);
}

bool UILLOnlineSessionClient::IsInPartyAsLeader() const
{
	return (bPartyBeaconEstablished && PartyBeaconHost && PartyHostBeaconObject && PartyHostBeaconObject->IsInitialized());
}

bool UILLOnlineSessionClient::IsInPartyAsMember() const
{
	return (bPartyBeaconEstablished && PartyClientBeacon && PartyClientBeacon->IsReservationAccepted() && PartyClientState == EILLPartyClientState::Connected);
}

bool UILLOnlineSessionClient::IsJoiningParty() const
{
	return (PartyClientState == EILLPartyClientState::InitialConnect);
}

void UILLOnlineSessionClient::LocalizedPartyNotification(UILLPartyLocalMessage::EMessage Message, AILLPartyBeaconMemberState* PartyMemberState/* = nullptr*/)
{
	if (PartyMessageClass)
	{
		if (AILLPlayerController* PC = Cast<AILLPlayerController>(GetOuterUILLGameInstance()->GetFirstLocalPlayerController()))
		{
			PC->ClientReceiveLocalizedMessage(PartyMessageClass, static_cast<int32>(Message), nullptr, nullptr, PartyMemberState);
		}
	}
}

void UILLOnlineSessionClient::FinishLeavingPartySession()
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Ensure simulated matchmaking stops
	GetOnlineMatchmaking()->OnFinishedLeavingPartySession();

	// Tear down the client and don't persist it
	TeardownPartyBeaconClient();

	// Leave NAME_PartySession and stay where we are
	DestroyExistingSession_Impl(OnLeavePartySessionCompleteDelegateHandle, NAME_PartySession, OnLeavePartySessionCompleteDelegate);

#if PLATFORM_DESKTOP // FIXME: pjackson
	// Force garbage collection to run now
	// Ensures any dangling SteamP2P sockets are flushed before creating new ones, there's a bug allow ones pending kill to be used!
	GEngine->PerformGarbageCollectionAndCleanupActors();
#endif

	// Show a localized notification
	LocalizedPartyNotification(UILLPartyLocalMessage::EMessage::Left);
}

void UILLOnlineSessionClient::FinishLeavingPartyForMainMenu()
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Ensure simulated matchmaking stops
	GetOnlineMatchmaking()->OnFinishedLeavingPartyForMainMenu();

	// Tear down the client and don't persist it
	TeardownPartyBeaconClient();

	// Leave NAME_PartySession and disconnect, returning to the main menu
	DestroyExistingSession_Impl(OnDestroyForMainMenuCompleteDelegateHandle, NAME_PartySession, OnDestroyForMainMenuCompleteDelegate);
}

void UILLOnlineSessionClient::OnCreatePartyBackendPlayerHeartbeat(UILLBackendPlayer* BackendPlayer, EILLBackendArbiterHeartbeatResult HeartbeatResult)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: HeartbeatResult: %i"), ANSI_TO_TCHAR(__FUNCTION__), static_cast<uint8>(HeartbeatResult));

	// Discard the arbitration handshake callback
	UWorld* World = GetWorld();
	if (UILLLocalPlayer* FirstLocalPlayer = Cast<UILLLocalPlayer>(World->GetFirstLocalPlayerFromController()))
	{
		if (FirstLocalPlayer->GetBackendPlayer())
			FirstLocalPlayer->GetBackendPlayer()->OnBackendPlayerArbiterHeartbeatComplete().RemoveAll(this);
	}

	if (HeartbeatResult == EILLBackendArbiterHeartbeatResult::Success)
	{
#if PLATFORM_DESKTOP // FIXME: pjackson
		// Force garbage collection to run now
		// Ensures any dangling SteamP2P sockets are flushed before creating new ones, there's a bug allow ones pending kill to be used!
		GEngine->PerformGarbageCollectionAndCleanupActors();
#endif

		// Spawn the party host listener net driver
		FActorSpawnParameters PartyActorSpawnParams;
		PartyActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		PartyActorSpawnParams.ObjectFlags |= EObjectFlags::RF_Transient;
		PartyBeaconHost = GetOrCreatePartyBeaconWorld()->SpawnActor<AILLPartyBeaconHost>(PartyBeaconHostClass, PartyActorSpawnParams);
		if (!PartyBeaconHost->InitHost())
		{
			UE_LOG(LogOnlineGame, Warning, TEXT("%s: Could not initialize the host listener!"), ANSI_TO_TCHAR(__FUNCTION__));
		}
		else
		{
			// Spawn the party host beacon
			PartyHostBeaconObject = GetOrCreatePartyBeaconWorld()->SpawnActor<AILLPartyBeaconHostObject>(PartyHostObjectClass, PartyActorSpawnParams);

			// Now initialize it
			PartyBeaconHost->RegisterHost(PartyHostBeaconObject);

			// Initialize with the first player as arbiter
			UILLLocalPlayer* FirstLocalPlayer = Cast<UILLLocalPlayer>(World->GetFirstLocalPlayerFromController());
			if (!PartyHostBeaconObject->InitPartyBeacon(PartyMaxMembers, FirstLocalPlayer))
			{
				UE_LOG(LogOnlineGame, Warning, TEXT("%s: Failed to initialize the PartyHostBeaconObject!"), ANSI_TO_TCHAR(__FUNCTION__));
			}
			else
			{
				// Create the party session host settings
				TSharedRef<FILLPartySessionSettings> HostSettings = MakeShareable(new FILLPartySessionSettings());
				HostSettings->NumPublicConnections = PartyMaxMembers;
				HostSettings->Set(SETTING_BEACONPORT, PartyBeaconHost->GetListenPort(), EOnlineDataAdvertisementType::ViaOnlineService);
#if PLATFORM_XBOXONE
				HostSettings->Set(SETTING_SESSION_TEMPLATE_NAME, XBOXONE_PARTY_SESSIONTEMPLATE, EOnlineDataAdvertisementType::DontAdvertise);
				HostSettings->Set(SEARCH_KEYWORDS, XBOXONE_PARTY_KEYWORDS, EOnlineDataAdvertisementType::ViaOnlineService);
#endif

				// Listen for completion then call to CreateSession
				IOnlineSessionPtr Sessions = GetSessionInt();
				OnCreatePartySessionCompleteDelegateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(OnCreatePartySessionCompleteDelegate);
				if (!Sessions->CreateSession(FirstLocalPlayer->GetControllerId(), NAME_PartySession, *HostSettings))
				{
					UE_LOG(LogOnlineGame, Error, TEXT("%s: CreateSession failed!"), ANSI_TO_TCHAR(__FUNCTION__));
				}
				else
				{
					return;
				}
			}
		}
	}

	// Error fall-through, cleanup!
	TeardownPartyBeaconHost();

	// Notify our listeners
	OnCreatePartyComplete().ExecuteIfBound(false);
}

void UILLOnlineSessionClient::OnCreatePartySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionName != NAME_PartySession)
		return;
	
	if (bWasSuccessful)
	{
		UE_LOG(LogOnlineGame, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *SessionName.ToString());
	}
	else
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("%s: %s failed!"), ANSI_TO_TCHAR(__FUNCTION__), *SessionName.ToString());
	}
	
	IOnlineSessionPtr Sessions = GetSessionInt();
	check(OnCreatePartySessionCompleteDelegateHandle.IsValid());
	Sessions->ClearOnCreateSessionCompleteDelegate_Handle(OnCreatePartySessionCompleteDelegateHandle);

	if (bWasSuccessful)
	{
		// Immediately start the session
		// This will register players which potentially enables voice
		StartOnlineSession(NAME_PartySession);

		// Finish party beacon host startup
		PartyHostBeaconObject->NotifyPartySessionStarted();

		// Fully established
		OnPartyBeaconEstablished();

		// Show a localized notification
		LocalizedPartyNotification(UILLPartyLocalMessage::EMessage::Created);

		// Send PlayTogether invites
		CheckPlayTogetherParty();
	}
	else
	{
		UE_LOG(LogOnlineGame, Error, TEXT("%s: Failed to create the party session!"), ANSI_TO_TCHAR(__FUNCTION__));

		// Error fall-through, cleanup!
		TeardownPartyBeaconHost();
	}

	// Notify our listeners
	OnCreatePartyComplete().ExecuteIfBound(bWasSuccessful);
}

void UILLOnlineSessionClient::OnDestroyPartySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionName != NAME_PartySession)
		return;

#if PLATFORM_DESKTOP // FIXME: pjackson
	// Force garbage collection to run now
	// Ensures any dangling SteamP2P sockets are flushed before creating new ones, there's a bug allow ones pending kill to be used!
	GEngine->PerformGarbageCollectionAndCleanupActors();
#endif

	if (bWasSuccessful)
	{
		UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

		// Show a localized notification
		LocalizedPartyNotification(UILLPartyLocalMessage::EMessage::Closed);
	}
	else
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("%s: failed!"), ANSI_TO_TCHAR(__FUNCTION__));
	}

	IOnlineSessionPtr Sessions = GetSessionInt();
	check(OnDestroyPartySessionCompleteDelegateHandle.IsValid());
	Sessions->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroyPartySessionCompleteDelegateHandle);

	// Notify our listener
	OnDestroyPartySessionComplete().ExecuteIfBound(bWasSuccessful);
}

void UILLOnlineSessionClient::OnLeavePartySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionName != NAME_PartySession)
		return;

	if (bWasSuccessful)
	{
		UE_LOG(LogOnlineGame, Verbose, TEXT("%s:"), ANSI_TO_TCHAR(__FUNCTION__));
	}
	else
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("%s: failed!"), ANSI_TO_TCHAR(__FUNCTION__));
	}

	IOnlineSessionPtr Sessions = GetSessionInt();
	check(OnLeavePartySessionCompleteDelegateHandle.IsValid());
	Sessions->ClearOnDestroySessionCompleteDelegate_Handle(OnLeavePartySessionCompleteDelegateHandle);

	// Notify our listener
	OnLeavePartyComplete().ExecuteIfBound(bWasSuccessful);
}

void UILLOnlineSessionClient::OnPartyClientHostConnectionFailure()
{
	UE_LOG(LogOnlineGame, Warning, TEXT("%s: PartyClientState: %i"), ANSI_TO_TCHAR(__FUNCTION__), static_cast<uint8>(PartyClientState));

	if (PartyClientState == EILLPartyClientState::InitialConnect)
	{
		const EOnJoinSessionCompleteResult::Type Result = EOnJoinSessionCompleteResult::CouldNotRetrieveAddress;

		if (PartyClientState == EILLPartyClientState::InitialConnect)
		{
			// Trigger the party join session callback
			CachedPartySessionCallback.ExecuteIfBound(NAME_PartySession, Result);
			CachedPartySessionCallback.Unbind();
		}

		CachedPartySessionResult = FOnlineSessionSearchResult();

		if (PartyClientState == EILLPartyClientState::InitialConnect)
		{
			// Hide any previous connecting or loading screens and log the failure
			GetOuterUILLGameInstance()->OnSessionJoinFailure(NAME_PartySession, Result);

			// Cleanup the party session
			LeavePartySession();
		}
	}
	else if (PartyClientState == EILLPartyClientState::Connected)
	{
		CachedPartySessionResult = FOnlineSessionSearchResult();
		PartyClientState = EILLPartyClientState::Disconnected;
		FinishLeavingPartySession();
		GetOuterUILLGameInstance()->OnPartyClientHostShutdown(LOCTEXT("PartyConnectionFailure_ConnectionLost", "Party connection lost!"));
	}
	else
	{
		check(0); // Is this ever hit?

		// Fallback to full teardown
		TeardownPartyBeaconClient();
		LeavePartyForMainMenu(FText());
	}

	bPartyBeaconEstablished = false;
}

void UILLOnlineSessionClient::OnPartyClientKicked(const FText& KickReason)
{
	UE_LOG(LogOnlineGame, Log, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));
	
	// Store our kick reason off
	if (KickReason.IsEmpty())
	{
		GetOuterUILLGameInstance()->SetLastPlayerKickReason(LOCTEXT("PartyKickedWithoutReason", "You were kicked from the party!"), false);
	}
	else
	{
		GetOuterUILLGameInstance()->SetLastPlayerKickReason(FText::Format(LOCTEXT("PartyKickedWithReason", "You were kicked from the party: {0}"), KickReason), false);
	}

	// Tear down our client now, because we will be kicked immediately after this message anyways, no sense in telling the server we are leaving and waiting on the ack
	TeardownPartyBeaconClient();

	// Leave the party
	LeavePartySession();
}

void UILLOnlineSessionClient::OnPartyClientHostShutdown()
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Leave the party session
	FinishLeavingPartySession();

	// Notify the UI
	GetOuterUILLGameInstance()->OnPartyClientHostShutdown(LOCTEXT("PartySessionClosed", "Party leader closed group"));
}

void UILLOnlineSessionClient::OnPartyClientLeaveAcked()
{
	FinishLeavingPartySession();
}

void UILLOnlineSessionClient::OnPartyClientReservationAccepted()
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Trigger the party join session callback
	CachedPartySessionCallback.ExecuteIfBound(NAME_PartySession, EOnJoinSessionCompleteResult::Success);

	// Flag that we're in
	PartyClientState = EILLPartyClientState::Connected;
	CachedPartySessionCallback.Unbind();
	CachedPartySessionResult = FOnlineSessionSearchResult();

	// Fully established
	OnPartyBeaconEstablished();

	// Show a localized notification
	LocalizedPartyNotification(UILLPartyLocalMessage::EMessage::Entered);
}

void UILLOnlineSessionClient::OnPartyClientReservationRequestFailed(const EILLSessionBeaconReservationResult::Type Response)
{
	UE_LOG(LogOnlineGame, Warning, TEXT("%s: Response: %s"), ANSI_TO_TCHAR(__FUNCTION__), *EILLSessionBeaconReservationResult::GetLocalizedFailureText(Response).ToString());

	if (ensure(PartyClientState == EILLPartyClientState::InitialConnect))
	{
		// Map the failure if possible
		EOnJoinSessionCompleteResult::Type JoinResult = EOnJoinSessionCompleteResult::UnknownError;
		switch (Response)
		{
		case EILLSessionBeaconReservationResult::Success:
			check(0);
			break;

		case EILLSessionBeaconReservationResult::GameFull:
		case EILLSessionBeaconReservationResult::PartyFull:
			JoinResult = EOnJoinSessionCompleteResult::SessionIsFull;
			break;

		case EILLSessionBeaconReservationResult::NotInitialized:
		case EILLSessionBeaconReservationResult::NoSession:
		case EILLSessionBeaconReservationResult::InvalidReservation:
		case EILLSessionBeaconReservationResult::Denied:
		case EILLSessionBeaconReservationResult::AuthFailed:
		case EILLSessionBeaconReservationResult::AuthTimedOut:
		case EILLSessionBeaconReservationResult::MatchmakingClosed:
			break;
		}

		// Notify the UI
		if (PartyClientState == EILLPartyClientState::InitialConnect)
		{
			// Trigger the party join session callback
			CachedPartySessionCallback.ExecuteIfBound(NAME_PartySession, JoinResult);
			CachedPartySessionCallback.Unbind();

			// Hide any previous loading screen
			GetOuterUILLGameInstance()->OnSessionReservationFailed(NAME_PartySession, Response);

			// Leave the party
			LeavePartySession();
		}
	}
}

void UILLOnlineSessionClient::OnPartyClientFindingLeaderGameSession()
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	UWorld* World = GetWorld();
	if (UILLGameInstance* GI = Cast<UILLGameInstance>(World->GetGameInstance()))
	{
		GI->OnPartyFindingLeaderGameSession();
	}
}

void UILLOnlineSessionClient::OnPartyClientJoinLeaderGameSessionFailed()
{
	UE_LOG(LogOnlineGame, Warning, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	LeavePartyForMainMenu(LOCTEXT("PartyFollowFailure_JoinSession", "Failed to join party leader's game session!"));
}

bool UILLOnlineSessionClient::OnPartyClientTravelRequested(const FOnlineSessionSearchResult& Session, FOnJoinSessionCompleteDelegate Delegate)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	if (Session.IsValid())
	{
#if PLATFORM_DESKTOP // FIXME: pjackson
		// Force garbage collection to run now
		// Ensures any dangling SteamP2P sockets are flushed before creating new ones, there's a bug allow ones pending kill to be used!
		GEngine->PerformGarbageCollectionAndCleanupActors();
#endif

		// Valid session, try to join it
		return JoinSession(NAME_GameSession, Session, Delegate);
	}

	// No session to join, return to the main menu
	UWorld* World = GetWorld();
	if (UILLGameInstance* GI = Cast<UILLGameInstance>(World->GetGameInstance()))
	{
		if (APlayerController* PC = GI->GetFirstLocalPlayerController())
		{
			PC->ClientReturnToMainMenu(FText());
			return true;
		}
	}

	return false;
}

void UILLOnlineSessionClient::OnPartyBeaconEstablished()
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// CLEANUP: pjackson: Ew.
	bPartyBeaconEstablished = true;
	AILLPartyBeaconState* PartyBeaconState = GetPartyBeaconState();
	bPartyBeaconEstablished = false;

	// Update push to talk
	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			AILLPlayerController* PC = Cast<AILLPlayerController>(*It);
			if (PC->IsLocalController())
			{
				PC->UpdatePushToTalkTransmit();
			}
		}
	}

	// Listen for new member additions/removals
	PartyBeaconState->OnBeaconMemberAdded().AddUObject(this, &ThisClass::OnPartyBeaconMemberAdded);
	PartyBeaconState->OnBeaconMemberRemoved().AddUObject(this, &ThisClass::OnPartyBeaconMemberRemoved);

	// Add our current members
	for (int32 MemberIndex = 0; MemberIndex < PartyBeaconState->GetNumMembers(); ++MemberIndex)
	{
		// PartyMember may be NULL on clients (not replicated yet) but OnBeaconMemberAdded should catch that later
		if (AILLPartyBeaconMemberState* PartyMember = Cast<AILLPartyBeaconMemberState>(PartyBeaconState->GetMemberAtIndex(MemberIndex)))
		{
			OnPartyBeaconMemberAdded(PartyMember);
		}
	}
	bPartyBeaconEstablished = true;
}

void UILLOnlineSessionClient::OnPartyBeaconMemberAdded(AILLSessionBeaconMemberState* MemberState)
{
	// Wait for our connection to fully establish before displaying these
	if (bPartyBeaconEstablished)
	{
		// Show a localized notification
		LocalizedPartyNotification(UILLPartyLocalMessage::EMessage::NewMember, Cast<AILLPartyBeaconMemberState>(MemberState));
	}

	if (IsInPartyAsLeader())
	{
		UILLOnlineMatchmakingClient* OMC = GetOnlineMatchmaking();

		// Cancel matchmaking when someone joins the party
		OMC->CancelMatchmaking();

		// Also invalidate our aggregate matchmaking status
		OMC->InvalidateMatchmakingStatus();
	}
}

void UILLOnlineSessionClient::OnPartyBeaconMemberRemoved(AILLSessionBeaconMemberState* MemberState)
{
	// Show a localized notification
	LocalizedPartyNotification(UILLPartyLocalMessage::EMessage::MemberLeft, Cast<AILLPartyBeaconMemberState>(MemberState));

	// Invalidate our aggregate matchmaking status
	if (IsInPartyAsLeader())
	{
		GetOnlineMatchmaking()->InvalidateMatchmakingStatus();
	}
}

void UILLOnlineSessionClient::OnPartyHostShutdownAcked()
{
	// Tear down the party beacon host
	TeardownPartyBeaconHost();

	// Destroy the NAME_PartySession and stay where we are
	DestroyExistingSession_Impl(OnDestroyPartySessionCompleteDelegateHandle, NAME_PartySession, OnDestroyPartySessionCompleteDelegate);
}

void UILLOnlineSessionClient::OnPartyHostTravelCoordinationComplete()
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: Succeeded, traveling..."), ANSI_TO_TCHAR(__FUNCTION__));

	// Show the loading screen
	GetOuterUILLGameInstance()->ShowTravelingScreen(CachedSessionResult);

	// Notify the server of impending travel
	if (IsInLobbyBeacon())
	{
		LobbyClientBeacon->OnBeaconTravelAcked().BindUObject(this, &ThisClass::FinishLobbyTravel);
		if (!LobbyClientBeacon->StartTraveling())
		{
			UE_LOG(LogOnlineGame, Warning, TEXT("%s: Failed to StartTraveling!"), ANSI_TO_TCHAR(__FUNCTION__));
		}
	}
	else
	{
		// No beacon, skip to travel
		FinishLobbyTravel();
	}
}

void UILLOnlineSessionClient::SpawnPartyBeaconClient(const EILLPartyClientState ConnectingState)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: ConnectingState: %d"), ANSI_TO_TCHAR(__FUNCTION__), static_cast<uint8>(ConnectingState));
	check(!PartyClientBeacon);

	FActorSpawnParameters ClientBeaconSpawnParams;
	ClientBeaconSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ClientBeaconSpawnParams.ObjectFlags |= EObjectFlags::RF_Transient;
	PartyClientBeacon = GetOrCreatePartyBeaconWorld()->SpawnActor<AILLPartyBeaconClient>(PartyClientClass, ClientBeaconSpawnParams);
	PartyClientBeacon->OnHostConnectionFailure().BindUObject(this, &ThisClass::OnPartyClientHostConnectionFailure);
	PartyClientBeacon->OnBeaconClientKicked().BindUObject(this, &ThisClass::OnPartyClientKicked);
	PartyClientBeacon->OnBeaconHostShutdown().BindUObject(this, &ThisClass::OnPartyClientHostShutdown);
	PartyClientBeacon->OnBeaconReservationAccepted().BindUObject(this, &ThisClass::OnPartyClientReservationAccepted);
	PartyClientBeacon->OnBeaconReservationRequestFailed().BindUObject(this, &ThisClass::OnPartyClientReservationRequestFailed);
	PartyClientBeacon->OnPartyFindingLeaderGameSession().BindUObject(this, &ThisClass::OnPartyClientFindingLeaderGameSession);
	PartyClientBeacon->OnPartyJoinLeaderGameSessionFailed().BindUObject(this, &ThisClass::OnPartyClientJoinLeaderGameSessionFailed);
	PartyClientBeacon->OnPartyTravelRequested().BindUObject(this, &ThisClass::OnPartyClientTravelRequested);

	UILLOnlineMatchmakingClient* OMC = GetOnlineMatchmaking();
	PartyClientBeacon->OnPartyMatchmakingStarted().BindUObject(OMC, &UILLOnlineMatchmakingClient::OnPartyMatchmakingStarted);
	PartyClientBeacon->OnPartyMatchmakingUpdated().BindUObject(OMC, &UILLOnlineMatchmakingClient::OnPartyMatchmakingUpdated);
	PartyClientBeacon->OnPartyMatchmakingStopped().BindUObject(OMC, &UILLOnlineMatchmakingClient::OnPartyMatchmakingStopped);

	PartyClientState = ConnectingState;
	bPartyBeaconEstablished = false;
}

void UILLOnlineSessionClient::TeardownPartyBeaconClient()
{
	if (PartyClientBeacon || PartyClientState != EILLPartyClientState::Disconnected)
	{
		UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));
	}

	if (PartyClientBeacon)
	{
		// Cleanup the client beacon
		PartyClientBeacon->DestroyBeacon();
		PartyClientBeacon = nullptr;
	}

	// Cleanup any state used during connection
	CachedPartySessionResult = FOnlineSessionSearchResult();
	PartyClientState = EILLPartyClientState::Disconnected;
	bPartyBeaconEstablished = false;
}

void UILLOnlineSessionClient::TeardownPartyBeaconHost()
{
	if (PartyBeaconHost || PartyHostBeaconObject)
	{
		UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));
	}

	if (PartyBeaconHost)
	{
		if (PartyHostBeaconObject)
		{
			// Unregister the PartyHostBeaconObject, disconnecting all clients
			PartyBeaconHost->UnregisterHost(PartyHostBeaconObject->GetBeaconType());
		}
		PartyBeaconHost->DestroyBeacon();
		PartyBeaconHost = nullptr;
	}

	if (PartyHostBeaconObject)
	{
		PartyHostBeaconObject->Destroy();
		PartyHostBeaconObject = nullptr;
	}

	bPartyBeaconEstablished = false;
}

#if !UE_BUILD_SHIPPING
void UILLOnlineSessionClient::CmdPartyCreate()
{
	CreateParty();
}

void UILLOnlineSessionClient::CmdPartyDestroy()
{
	DestroyPartySession();
}

void UILLOnlineSessionClient::CmdPartyInvite()
{
	UILLPartyBlueprintLibrary::InviteFriendToParty(GetWorld());
}

void UILLOnlineSessionClient::CmdPartyLeave()
{
	LeavePartySession();
}
#endif

bool UILLOnlineSessionClient::CheckPlayTogetherParty()
{
	if (!PlayTogetherInfo.IsValid())
		return false;

	if (IsInPartyAsLeader())
	{
		// Leading a party, send invites
		SendPlayTogetherInvites(NAME_PartySession);
	}
	else if (!IsCreatingParty()) // Wait for party creation completion
	{
		// Create a party 
		ensureAlways(CreateParty());
	}

	return true;
}

void UILLOnlineSessionClient::SendPlayTogetherInvites(FName InSessionName)
{
	IOnlineSessionPtr Sessions = GetSessionInt();
	check(Sessions.IsValid());

	if (PlayTogetherInfo.IsValid())
	{
		// Blast out invites to the entire Play Together group
		for (const ULocalPlayer* LocalPlayer : GetOuterUILLGameInstance()->GetLocalPlayers())
		{
			if (LocalPlayer->GetControllerId() == PlayTogetherInfo.UserIndex)
			{
				TSharedPtr<const FUniqueNetId> PlayerId = LocalPlayer->GetPreferredUniqueNetId();
				if (PlayerId.IsValid())
				{
					// Automatically send invites to friends in the player's PS4 party to conform with Play Together requirements
					for (const TSharedPtr<const FUniqueNetId>& FriendId : PlayTogetherInfo.UserIdList)
					{
						Sessions->SendSessionInviteToFriend(*PlayerId.ToSharedRef(), InSessionName, *FriendId.ToSharedRef());
					}
				}
			}
		}

		// Handle automatically progressing into a hosted game
		if (bPlayTogetherAutoHostQuickPlay)
		{
			UILLGameBlueprintLibrary::ShowLoadingAndTravel(this, GetOuterUILLGameInstance()->PickRandomQuickPlayMap(), true, TEXT("listen?ForPlayTogether"));
		}

		PlayTogetherInfo = FILLPlayTogetherInfo();
	}
}

#if !UE_BUILD_SHIPPING
void UILLOnlineSessionClient::CmdRequestLobbySession(const TArray<FString>& Args)
{
	if (Args.Num() != 2)
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("Usage: RequestLobbySession <ConnectionString> <PlayerSessionId>"));
		return;
	}
	FILLOnLobbyBeaconSessionReceived Dumb;
	FILLOnLobbyBeaconSessionRequestComplete Dumber;
	BeginRequestLobbySession(Args[0], Args[1], Dumb, Dumber);
}
#endif

#undef LOCTEXT_NAMESPACE
