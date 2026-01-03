// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLLobbyBeaconClient.h"

#include "OnlineIdentityInterface.h"
#include "OnlineSessionInterface.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

#if USE_GAMELIFT_SERVER_SDK
# include "GameLiftServerSDK.h"
#endif

#include "ILLBackendPlayer.h"
#include "ILLBackendRequestManager.h"
#include "ILLGameEngine.h"
#include "ILLGameInstance.h"
#include "ILLLobbyBeaconHostObject.h"
#include "ILLLobbyBeaconMemberState.h"
#include "ILLLobbyBeaconState.h"
#include "ILLLocalPlayer.h"
#include "ILLOnlineMatchmakingClient.h"
#include "ILLOnlineSessionClient.h"
#include "ILLPartyBeaconState.h"
#include "ILLPartyBeaconMemberState.h"
#include "ILLPSNRequestManager.h"

#if PLATFORM_XBOXONE
using namespace Windows::Xbox::Networking;
#endif

#define AUTH_PARAMS_SEPARATOR TEXT("--")

/** @return Unique character for the OnlineEnvironment. MUST MATCH FromUniqueCharacter! */
FORCEINLINE TCHAR ToUniqueCharacter(const EOnlineEnvironment::Type OnlineEnvironment)
{
	switch (OnlineEnvironment)
	{
	case EOnlineEnvironment::Development: return TEXT('D');
	case EOnlineEnvironment::Certification: return TEXT('C');
	case EOnlineEnvironment::Production: return TEXT('P');
	}

	check(OnlineEnvironment == EOnlineEnvironment::Unknown);
	return TEXT('U');
}

/** @return Online environment for UniqueChar. MUST MATCH ToUniqueCharacter! */
FORCEINLINE EOnlineEnvironment::Type FromUniqueCharacter(const TCHAR UniqueChar)
{
	switch (UniqueChar)
	{
	case TEXT('D'): return EOnlineEnvironment::Development;
	case TEXT('C'): return EOnlineEnvironment::Certification;
	case TEXT('P'): return EOnlineEnvironment::Production;
	}

	return EOnlineEnvironment::Unknown;
}

AILLLobbyBeaconClient::AILLLobbyBeaconClient(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	AuthorizationTimeout = 40.f;
	ClientControllerId = -1;

#if PLATFORM_DESKTOP
	LobbySearchDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnLobbySearchCompleted);
#else
	LobbySearchDelegate = FOnSingleSessionResultCompleteDelegate::CreateUObject(this, &ThisClass::OnLobbySearchCompleted);
#endif
	LobbyJoinDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnLobbyJoinCompleted);
}

void AILLLobbyBeaconClient::OnNetCleanup(UNetConnection* Connection)
{
	// Flag us for destruction
	Super::OnNetCleanup(Connection);

	// Cleanup our XB1 SDAs
#if PLATFORM_XBOXONE
	if (LobbyPeerTemplate)
	{
		LobbyPeerTemplate->AssociationIncoming -= LobbyAssociationIncoming;
	}
	if (GamePeerTemplate)
	{
		GamePeerTemplate->AssociationIncoming -= GameAssociationIncoming;
	}
#endif
}

bool AILLLobbyBeaconClient::ShouldReceiveVoiceFrom(const FUniqueNetIdRepl& Sender) const
{
	if (AILLPlayerController* GamePC = FindGamePlayerController())
	{
		// Check that we have finished loading and synchronizing
		if (!GamePC->HasFullyTraveled())
			return false;

		// Check if the Sender is muted
		if (GamePC->IsPlayerMuted(Sender))
			return false;

		// Wait for a game state, but disallow it once we start leaving the current map
		AGameState* GameState = GamePC->GetWorld()->GetGameState<AGameState>();
		if (!GameState || GameState->GetMatchState() == MatchState::LeavingMap)
			return false;

		return true;
	}

	return false;
}

bool AILLLobbyBeaconClient::ShouldSendVoice() const
{
	if (AILLPlayerController* GamePC = FindGamePlayerController())
	{
		// Wait to finish loading
		if (!GamePC->HasFullyTraveled())
			return false;

		// Wait for a game state, but disallow it once we start leaving the current map
		AGameState* GameState = GamePC->GetWorld()->GetGameState<AGameState>();
		if (!GameState || GameState->GetMatchState() == MatchState::LeavingMap)
			return false;

		return true;
	}

	return false;
}

void AILLLobbyBeaconClient::HandleNetworkFailure(UWorld* World, UNetDriver* InNetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	if (InNetDriver && InNetDriver->NetDriverName == NetDriverName)
	{
		// HACK: pjackson: Forge a game world network failure first, so we get correct error messaging
		UILLGameInstance* GI = CastChecked<UILLGameInstance>(GetGameInstance());
		GI->OnNetworkFailure(nullptr, NetDriver, FailureType, ErrorString);
	}

	Super::HandleNetworkFailure(World, NetDriver, FailureType, ErrorString);
}

void AILLLobbyBeaconClient::OnConnected()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	if (bPendingLobbySession)
	{
		// Request the lobby session immediately
		ServerRequestLobbySession(ClientAccountId, ClientUniqueId, AuthorizationParams, ClientPlayerSessionId);
	}
	else
	{
		// Request membership immediately
		ServerRequestLobbyReservation(SessionOwnerUniqueId, PendingReservation, ClientAccountId, ClientUniqueId, AuthorizationParams, bMatchmakingReservation);
	}
}

#if PLATFORM_XBOXONE
const TCHAR* AssociationStateToString(SecureDeviceAssociationState State)
{
	switch (State)
	{
	case SecureDeviceAssociationState::CreatingInbound: return TEXT("CreatingInbound");
	case SecureDeviceAssociationState::CreatingOutbound: return TEXT("CreatingOutbound");
	case SecureDeviceAssociationState::Destroyed: return TEXT("Destroyed");
	case SecureDeviceAssociationState::DestroyingLocal: return TEXT("DestroyingLocal");
	case SecureDeviceAssociationState::DestroyingRemote: return TEXT("DestroyingRemote");
	case SecureDeviceAssociationState::Invalid: return TEXT("Invalid");
	case SecureDeviceAssociationState::Ready: return TEXT("Ready");
	}

	return TEXT("Unknown");
}
void LogAssociationStateChange(SecureDeviceAssociation^ Association, SecureDeviceAssociationStateChangedEventArgs^ EventArgs)
{
	if (EventArgs == nullptr)
	{
		return;
	}

	UE_LOG(LogBeacon, Log, TEXT("SecureDeviceAssociation with remote host %s changed from state %s to %s"),
		Association->RemoteHostName->DisplayName->Data(),
		AssociationStateToString(EventArgs->OldState),
		AssociationStateToString(EventArgs->NewState));
}
#endif

bool AILLLobbyBeaconClient::InitClientBeacon(const FString& ConnectionString, const FILLBackendPlayerIdentifier& AccountId, const FUniqueNetIdRepl& UniqueId)
{
	// Use network encryption
	SetEncryptionToken(SessionKeySet.EncryptionToken);

	// Grab our XB1 SDAs template to open the ports
#if PLATFORM_XBOXONE
	if (ConnectionString.Contains(TEXT("?bIsDedicated")))
	{
		if (!LobbyTemplateName.IsEmpty())
		{
			try
			{
				LobbyPeerTemplate = SecureDeviceAssociationTemplate::GetTemplateByName(ref new Platform::String(*LobbyTemplateName));

				// Listen for Secure Association incoming
				auto AssociationIncomingEvent = ref new TypedEventHandler<SecureDeviceAssociationTemplate^, SecureDeviceAssociationIncomingEventArgs^>(
					[] (Platform::Object^, SecureDeviceAssociationIncomingEventArgs^ EventArgs)
				{
					if (EventArgs->Association)
					{
						UE_LOG(LogBeacon, Log, TEXT("Received association, state is %d."), EventArgs->Association->State);

						auto StateChangedEvent = ref new TypedEventHandler<SecureDeviceAssociation^, SecureDeviceAssociationStateChangedEventArgs^>(&LogAssociationStateChange);
						EventArgs->Association->StateChanged += StateChangedEvent;
					}
				});

				LobbyAssociationIncoming = LobbyPeerTemplate->AssociationIncoming += AssociationIncomingEvent;
			}
			catch(Platform::COMException^ Ex)
			{
				UE_LOG(LogBeacon, Error, TEXT("Couldn't find secure device association template named %s. Check the app manifest."), *LobbyTemplateName);
			}
		}

		if (!GameTemplateName.IsEmpty())
		{
			try
			{
				GamePeerTemplate = SecureDeviceAssociationTemplate::GetTemplateByName(ref new Platform::String(*GameTemplateName));

				// Listen for Secure Association incoming
				auto AssociationIncomingEvent = ref new TypedEventHandler<SecureDeviceAssociationTemplate^, SecureDeviceAssociationIncomingEventArgs^>(
					[] (Platform::Object^, SecureDeviceAssociationIncomingEventArgs^ EventArgs)
				{
					if (EventArgs->Association)
					{
						UE_LOG(LogBeacon, Log, TEXT("Received association, state is %d."), EventArgs->Association->State);

						auto StateChangedEvent = ref new TypedEventHandler<SecureDeviceAssociation^, SecureDeviceAssociationStateChangedEventArgs^>(&LogAssociationStateChange);
						EventArgs->Association->StateChanged += StateChangedEvent;
					}
				});

				GameAssociationIncoming = GamePeerTemplate->AssociationIncoming += AssociationIncomingEvent;
			}
			catch(Platform::COMException^ Ex)
			{
				UE_LOG(LogBeacon, Error, TEXT("Couldn't find secure device association template named %s. Check the app manifest."), *GameTemplateName);
			}
		}
	}
#endif

	return Super::InitClientBeacon(ConnectionString, AccountId, UniqueId);
}

bool AILLLobbyBeaconClient::IsPendingAuthorization() const
{
	// Wait for authorization
	if (!bAuthorized)
		return true;

#if USE_GAMELIFT_SERVER_SDK
	if (IsRunningDedicatedServer())
	{
		// Wait for matchmaking bypass
		if (ClientPlayerSessionId.IsEmpty() || MatchmakingBypassDelegate.IsBound())
			return true;
	}
#endif

	return Super::IsPendingAuthorization();
}

AILLPlayerController* AILLLobbyBeaconClient::FindGamePlayerController() const
{
	AILLPlayerController* Result = nullptr;
	if (UWorld* GameWorld = GetGameInstance()->GetWorld())
	{
		// Look for the corresponding PlayerController in the game world
		for (FConstPlayerControllerIterator It = GameWorld->GetPlayerControllerIterator(); It; ++It)
		{
			AILLPlayerController* PC = Cast<AILLPlayerController>(*It);
			if (PC && PC->LinkedLobbyClient.Get() == this)
			{
				Result = PC;
				break;
			}
		}
	}

	return Result;
}

void AILLLobbyBeaconClient::OnLoginToConnectComplete(bool bWasSuccessful, const FString& Error, UILLLocalPlayer* FirstLocalPlayer)
{
	if (bWasSuccessful)
	{
		ConditionalRequestKeysAndConnect();
	}
	else if (bPendingLobbySession)
	{
		LobbyBeaconSessionRequestComplete.ExecuteIfBound(EILLLobbyBeaconSessionResult::FailedToLogin);
	}
	else
	{
		OnFailure();
	}
}

void AILLLobbyBeaconClient::ConditionalRequestKeysAndConnect()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Only desktop/dedicated sessions will perform lobby network encryption
	UILLGameInstance* GI = CastChecked<UILLGameInstance>(GetGameInstance());
	UILLBackendRequestManager* BRM = GI->GetBackendRequestManager();
	if (BRM->KeyDistributionService_Create && BRM->KeyDistributionService_Retrieve && ILLNETENCRYPTION_DESKTOP_REQUIRED && (PLATFORM_DESKTOP || PendingConnectionString.Contains(TEXT("?bIsDedicated"))))
	{
		// Make a client key request
		UWorld* GameWorld = GI->GetWorld();
		UILLLocalPlayer* FirstLocalPlayer = Cast<UILLLocalPlayer>(GameWorld->GetFirstLocalPlayerFromController());
		FOnILLBackendSimpleRequestDelegate Delegate = FOnILLBackendSimpleRequestDelegate::CreateUObject(this, &ThisClass::KeySessionResponse);
		UILLBackendSimpleHTTPRequestTask* RequestTask = BRM->CreateSimpleRequest(BRM->KeyDistributionService_Create, Delegate, FString(), FirstLocalPlayer->GetBackendPlayer());
		if (RequestTask)
		{
			// Queue request
			RequestTask->QueueRequest();
		}
		else
		{
			// Failed to build a request
			if (bPendingLobbySession)
			{
				LobbyBeaconSessionRequestComplete.ExecuteIfBound(EILLLobbyBeaconSessionResult::FailedToRequestKeys);
			}
			else
			{
				OnFailure();
			}
		}
	}
	else
	{
		// Clear our key set so we don't accidentally request encryption
		SessionKeySet.Reset();

		// Start the connection process
		if (!InitClientBeacon(PendingConnectionString, ClientAccountId, ClientUniqueId))
		{
			// InitClientBeacon will call OnFailure when it returns false
			if (bPendingLobbySession)
			{
				LobbyBeaconSessionRequestComplete.ExecuteIfBound(EILLLobbyBeaconSessionResult::FailedToBind);
			}
		}
	}
}

void AILLLobbyBeaconClient::KeySessionResponse(int32 ResponseCode, const FString& ResponseContent)
{
	if (ResponseCode == 200)
	{
		UE_LOG(LogBeacon, Verbose, TEXT("%s: %i"), ANSI_TO_TCHAR(__FUNCTION__), ResponseCode);
	}
	else
	{
		UE_LOG(LogBeacon, Warning, TEXT("%s: %i"), ANSI_TO_TCHAR(__FUNCTION__), ResponseCode);
	}

	// Attempt to parse the response
	UILLGameInstance* GI = CastChecked<UILLGameInstance>(GetGameInstance());
	if (FILLBackendKeySession::ParseFromJSON(GI, ResponseCode, ResponseContent, SessionKeySet))
	{
		// Start the connection process
		if (!InitClientBeacon(PendingConnectionString, ClientAccountId, ClientUniqueId))
		{
			// InitClientBeacon will call OnFailure when it returns false
			if (bPendingLobbySession)
			{
				LobbyBeaconSessionRequestComplete.ExecuteIfBound(EILLLobbyBeaconSessionResult::FailedToBind);
			}
		}
	}
	else
	{
		if (bPendingLobbySession)
		{
			LobbyBeaconSessionRequestComplete.ExecuteIfBound(EILLLobbyBeaconSessionResult::FailedToRequestKeys);
		}
		else
		{
			OnFailure();
		}
	}
}

void AILLLobbyBeaconClient::MigrateToReplacement(const TArray<FILLMatchmadePlayerSession>& PlayerSessions)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Tell the client to migrate
	ClientMigrateToReplacement(PlayerSessions);
}

void AILLLobbyBeaconClient::ClientMigrateToReplacement_Implementation(const TArray<FILLMatchmadePlayerSession>& PlayerSessions)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Return to the main menu and join the replacement
	UILLGameInstance* GameInstance = CastChecked<UILLGameInstance>(GetGameInstance());
	UWorld* GameWorld = GameInstance->GetWorld();
	UILLOnlineMatchmakingClient* OMC = GameInstance->GetOnlineMatchmaking();
	OMC->HandleDisconnectForMatchmaking(GameWorld, GameWorld->GetNetDriver(), PlayerSessions);
}

void AILLLobbyBeaconClient::RequestLobbySession(const FString& ConnectionString, UILLLocalPlayer* FirstLocalPlayer, const FString& PlayerSessionId)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *ConnectionString);

	// Flag as pending a lobby session, store off information to be sent up once connected
	bPendingLobbySession = true;
	ClientPlayerSessionId = PlayerSessionId;
	ClientAccountId = FirstLocalPlayer->GetBackendPlayer()->GetAccountId();
	ClientUniqueId = FirstLocalPlayer->GetCachedUniqueNetId();
	ClientControllerId = FirstLocalPlayer->GetControllerId();

	// Lobby session requests are always sent for dedicated server connection requests
	PendingConnectionString = ConnectionString;
	PendingConnectionString += TEXT("?bIsDedicated");

	// Build our authorization token
	BeginLoginForAuthorization(FirstLocalPlayer, FOnILLLobbyBeaconLoginComplete::CreateUObject(this, &ThisClass::OnLoginToConnectComplete, FirstLocalPlayer));
}

void AILLLobbyBeaconClient::OnLobbySessionClientAuthorizationComplete(AILLLobbyBeaconClient* LobbyClient, EILLLobbyBeaconClientAuthorizationResult Result)
{
	check(IsRunningDedicatedServer());

	AILLLobbyBeaconHostObject* BeaconHost = CastChecked<AILLLobbyBeaconHostObject>(GetBeaconOwner());

	// Check authorization result
	if (Result != EILLLobbyBeaconClientAuthorizationResult::Success)
	{
		ClientLobbySessionAuthorizationFailure();
		BeaconHost->DisconnectClient(this);
		return;
	}

#if USE_GAMELIFT_SERVER_SDK
	// Authorize the user with GameLift first
	UILLGameEngine* GameEngine = CastChecked<UILLGameEngine>(GEngine);
	FGameLiftServerSDKModule& GameLiftModule = GameEngine->GetGameLiftModule();
	if (GameLiftModule.AcceptPlayerSession(ClientPlayerSessionId).success)
	{
		UE_LOG(LogBeacon, Verbose, TEXT("... authorized ClientPlayerSessionId"));
	}
	else
	{
		UE_LOG(LogBeacon, Warning, TEXT("... ClientPlayerSessionId not accepted!"));
		ClientLobbySessionPlayerSessionInvalid();
		BeaconHost->DisconnectClient(this);
		return;
	}
#endif // USE_GAMELIFT_SERVER_SDK

	// Grab the current game session
	FNamedOnlineSession* GameSession = GetValidNamedSession(NAME_GameSession);
	if (!GameSession)
	{
		UE_LOG(LogBeacon, Warning, TEXT("... No Game session!"));
		ClientLobbySessionGameSessionInvalid();
		BeaconHost->DisconnectClient(this);
		return;
	}

	IOnlineIdentityPtr OnlineIdentity = Online::GetIdentityInterface();
	if (AuthorizationType == TEXT("steam"))
	{
		// Steam: Tell clients to lookup the gameaddr
		BeaconConnection->SetPlayerOnlinePlatformName(STEAM_SUBSYSTEM);
		UWorld* GameWorld = GetGameInstance()->GetWorld();
		ClientLobbySessionReceived(GameSession->SessionInfo->GetSessionId().ToString(), FString::FromInt(GameWorld->URL.Port));
	}
	else if (AuthorizationType == TEXT("psn"))
	{
		// PSN: Check if we need to create a session, or hand the existing one back
		BeaconConnection->SetPlayerOnlinePlatformName(PS4_SUBSYSTEM);
		UILLGameInstance* GameInstance = CastChecked<UILLGameInstance>(GetGameInstance());
		if (GameInstance->GetPSNRequestManager()->IsCreatingGameSession())
		{
			// Already creating a session, just wait for that to complete
			// AILLLobbyBeaconHostObject::OnLobbySessionPSNCreated will blast the session out to all authorized clients
		}
		else if (GameInstance->GetPSNRequestManager()->GetGameSessionId().IsEmpty())
		{
			// No session exists, create one
			GameInstance->GetPSNRequestManager()->CreateGameSession(FOnILLPSNCreateSessionCompleteDelegate::CreateUObject(BeaconHost, &AILLLobbyBeaconHostObject::OnLobbySessionPSNCreated));
		}
		else
		{
			// Notify user of existing session
			ClientLobbySessionReceived(GameInstance->GetPSNRequestManager()->GetGameSessionId(), GameInstance->GetPSNRequestManager()->GetGameSessionId());
		}
	}
	else if (AuthorizationType == TEXT("xbl"))
	{
		// XBL: Check if we need to create a session, or hand the existing one back
		BeaconConnection->SetPlayerOnlinePlatformName(LIVE_SUBSYSTEM);

		UILLGameInstance* GameInstance = CastChecked<UILLGameInstance>(GetGameInstance());
		if (GameInstance->GetBackendRequestManager()->IsCreatingXBLServerSession())
		{
			// Already creating a session, just wait for that to complete
			// AILLLobbyBeaconHostObject::OnLobbySessionXBLCreated will blast the session out to all authorized clients
		}
		else if (GameInstance->GetBackendRequestManager()->GetActiveXBLServerSessionUri().IsEmpty())
		{
			// No session exists, create one
			// In this case, AuthorizationToken is the initiating XUID
			GameInstance->GetBackendRequestManager()->SendXBLServerSessionRequest(AuthorizationToken, AuthSandbox, FOnILLPutXBLSessionCompleteDelegate::CreateUObject(BeaconHost, &AILLLobbyBeaconHostObject::OnLobbySessionXBLCreated));
		}
		else
		{
			// Notify user of existing session
			ClientLobbySessionReceived(GameInstance->GetBackendRequestManager()->GetActiveXBLSessionName(), GameInstance->GetBackendRequestManager()->GetActiveXBLServerSessionUri());
		}
	}
	else
	{
		// We do not understand this AuthorizationType
		ClientLobbySessionAuthorizationFailure();
		BeaconHost->DisconnectClient(this);
	}
}

void AILLLobbyBeaconClient::ClientLobbySessionAuthorizationFailure_Implementation()
{
	UE_LOG(LogBeacon, Warning, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Notify our initiator
	LobbyBeaconSessionRequestComplete.ExecuteIfBound(EILLLobbyBeaconSessionResult::FailedToAuthorize);
}

void AILLLobbyBeaconClient::ClientLobbySessionGameSessionInvalid_Implementation()
{
	UE_LOG(LogBeacon, Warning, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Notify our initiator
	LobbyBeaconSessionRequestComplete.ExecuteIfBound(EILLLobbyBeaconSessionResult::InvalidGameSession);
}

void AILLLobbyBeaconClient::ClientLobbySessionPlayerSessionInvalid_Implementation()
{
	UE_LOG(LogBeacon, Warning, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Notify our initiator
	LobbyBeaconSessionRequestComplete.ExecuteIfBound(EILLLobbyBeaconSessionResult::InvalidPlayerSession);
}

void AILLLobbyBeaconClient::ClientLobbySessionReceived_Implementation(const FString& SessionId, const FString& SessionLookupIdentifier)
{
	UE_LOG(LogBeacon, Log, TEXT("%s: SessionLookupIdentifier: %s"), ANSI_TO_TCHAR(__FUNCTION__), *SessionLookupIdentifier);

	// Notify our initiator
	LobbyBeaconSessionReceived.ExecuteIfBound(SessionId);

	IOnlineSessionPtr Sessions = Online::GetSessionInterface();

#if PLATFORM_DESKTOP
	// Steam case: Search by gameaddr
	// Create a search object to filter for our target session
	LobbySearchObject = MakeShareable(new FILLOnlineSessionSearch);
	LobbySearchObject->MaxSearchResults = 1;

	// In this case, SessionLookupIdentifier is the port
	const FString FixedUpIdentifier = FString::Printf(TEXT("%s:%s"), *BeaconConnection->URL.Host, *SessionLookupIdentifier);
	LobbySearchObject->QuerySettings.Set(SEARCH_STEAM_HOSTIP, FixedUpIdentifier, EOnlineComparisonOp::Equals);
	LobbySearchObject->QuerySettings.Set(SEARCH_DEDICATED_ONLY, 1, EOnlineComparisonOp::Equals);

	// Start a search for the session
	LobbySearchHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(LobbySearchDelegate);
	if (!Sessions->FindSessions(ClientControllerId, LobbySearchObject.ToSharedRef()))
	{
		// Cleanup
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(LobbySearchHandle);
		LobbySearchHandle.Reset();

		// NOTE: No need to broadcast on LobbyBeaconSessionRequestComplete because FindSessions triggers the delegate before returning false
	}
#else
	// PS4 and XB1 case: Search by session identifier
	Sessions->FindSessionById(*ClientUniqueId, FUniqueNetIdString(SessionLookupIdentifier), FUniqueNetIdString(), LobbySearchDelegate);
#endif
}

#if PLATFORM_DESKTOP
void AILLLobbyBeaconClient::OnLobbySearchCompleted(bool bSuccess)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: bSuccess? %s Results: %i"), ANSI_TO_TCHAR(__FUNCTION__), bSuccess ? TEXT("TRUE") : TEXT("FALSE"), LobbySearchObject.IsValid() ? LobbySearchObject->SearchResults.Num() : -1);

	IOnlineSessionPtr Sessions = Online::GetSessionInterface();
	Sessions->ClearOnFindSessionsCompleteDelegate_Handle(LobbySearchHandle);
	LobbySearchHandle.Reset();

	if (bSuccess && LobbySearchObject.IsValid() && LobbySearchObject->SearchResults.Num() > 0)
	{
		// Found something, join it
		UILLOnlineSessionClient* OSC = CastChecked<UILLOnlineSessionClient>(GetGameInstance()->GetOnlineSession());
		OSC->JoinSession(NAME_GameSession, LobbySearchObject->SearchResults[0], LobbyJoinDelegate, FString(), FString(), true);
		return;
	}

	// Notify our initiator
	LobbyBeaconSessionRequestComplete.ExecuteIfBound(EILLLobbyBeaconSessionResult::FailedToFindSession);
}
#else
void AILLLobbyBeaconClient::OnLobbySearchCompleted(int32 LocalUserNum, bool bWasSuccessful, const FOnlineSessionSearchResult& SearchResult)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: bWasSuccessful? %s"), ANSI_TO_TCHAR(__FUNCTION__), bWasSuccessful ? TEXT("TRUE") : TEXT("FALSE"));

	if (bWasSuccessful)
	{
		// Found something, join it
		UILLOnlineSessionClient* OSC = CastChecked<UILLOnlineSessionClient>(GetGameInstance()->GetOnlineSession());
		OSC->JoinSession(NAME_GameSession, SearchResult, LobbyJoinDelegate, FString(), FString(), true);
		return;
	}

	// Notify our initiator
	LobbyBeaconSessionRequestComplete.ExecuteIfBound(EILLLobbyBeaconSessionResult::FailedToFindSession);
}
#endif

void AILLLobbyBeaconClient::OnLobbyJoinCompleted(FName InSessionName, EOnJoinSessionCompleteResult::Type Result)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: Result: %s"), ANSI_TO_TCHAR(__FUNCTION__), *GetLocalizedDescription(InSessionName, Result).ToString());

	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		bPendingLobbySession = false;
		LobbyBeaconSessionRequestComplete.ExecuteIfBound(EILLLobbyBeaconSessionResult::Success);
	}
	else
	{
		LobbyBeaconSessionRequestComplete.ExecuteIfBound(EILLLobbyBeaconSessionResult::FailedToJoin);
	}
}

bool AILLLobbyBeaconClient::ServerRequestLobbySession_Validate(const FILLBackendPlayerIdentifier& AccountId, const FUniqueNetIdRepl& UniqueId, const FString& AuthParams, const FString& PlayerSessionId)
{
	return (AccountId.IsValid() && !AuthParams.IsEmpty() && !PlayerSessionId.IsEmpty());
}

void AILLLobbyBeaconClient::ServerRequestLobbySession_Implementation(const FILLBackendPlayerIdentifier& AccountId, const FUniqueNetIdRepl& UniqueId, const FString& AuthParams, const FString& PlayerSessionId)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: %s %s"), ANSI_TO_TCHAR(__FUNCTION__), *AccountId.ToDebugString(), *PlayerSessionId);

	// Store off information about the client
	ClientAccountId = AccountId;
	ClientUniqueId = UniqueId;
	AuthorizationParams = AuthParams;
	ClientPlayerSessionId = PlayerSessionId;

	// Begin the authorization sequence before handing back the session
	BeginAuthorization(FOnILLLobbyBeaconAuthorizationComplete::CreateUObject(this, &ThisClass::OnLobbySessionClientAuthorizationComplete));
}

void AILLLobbyBeaconClient::RequestLobbyReservation(const FOnlineSessionSearchResult& DesiredHost, UILLLocalPlayer* FirstLocalPlayer, AILLPartyBeaconState* HostingPartyState, const FILLBackendPlayerIdentifier& PartyLeaderAccountId, const bool bForMatchmaking)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	if (!DesiredHost.IsValid())
	{
		UE_LOG(LogBeacon, Error, TEXT("%s: Invalid DesiredHost!"), ANSI_TO_TCHAR(__FUNCTION__));
		OnFailure();
		return;
	}
	if (!FirstLocalPlayer)
	{
		UE_LOG(LogBeacon, Error, TEXT("%s: Invalid FirstLocalPlayer!"), ANSI_TO_TCHAR(__FUNCTION__));
		OnFailure();
		return;
	}

	FILLSessionBeaconReservation Reservation;

	// Always add the local player
	Reservation.Players.Add(FILLSessionBeaconPlayer(FirstLocalPlayer));

	// Generate a reservation from our party if we're the leader
	if (HostingPartyState)
	{
		// We should be in a party as a member...
		if (PartyLeaderAccountId.IsValid())
		{
			UE_LOG(LogBeacon, Error, TEXT("%s: Party leader somehow has a party leader: %s!"), ANSI_TO_TCHAR(__FUNCTION__), *PartyLeaderAccountId.ToDebugString());
			OnFailure();
			return;
		}

		// We are hosting a party, so send along that reservation
		for (int32 MemberIndex = 0; MemberIndex < HostingPartyState->GetNumMembers(); ++MemberIndex)
		{
			AILLSessionBeaconMemberState* PartyMember = HostingPartyState->GetMemberAtIndex(MemberIndex);
			check(PartyMember);

			if (PartyMember->bArbitrationMember)
			{
				// This should be us! And we are already in the Players list
				check(PartyMember->GetAccountId() == FirstLocalPlayer->GetBackendPlayer()->GetAccountId());
				Reservation.LeaderAccountId = PartyMember->GetAccountId();
			}
			else
			{
				// This is a fellow party member, send along their information
				Reservation.Players.AddUnique(FILLSessionBeaconPlayer(PartyMember->GetAccountId(), PartyMember->GetMemberUniqueId(), PartyMember->GetDisplayName()));
			}
		}
	}
	else
	{
		// When a party member, make sure to mention who our leader is
		Reservation.LeaderAccountId = PartyLeaderAccountId;
	}

	// Store off properties to be sent to the server
	ClientAccountId = FirstLocalPlayer->GetBackendPlayer()->GetAccountId();
	ClientUniqueId = FirstLocalPlayer->GetPreferredUniqueNetId();
	ClientControllerId = FirstLocalPlayer->GetControllerId();
	SessionOwnerUniqueId = DesiredHost.Session.OwningUserId;
	PendingReservation = Reservation;
	bPendingLobbySession = false;
	bMatchmakingReservation = bForMatchmaking;

	if (GetConnectionState() == EBeaconConnectionState::Open)
	{
		// Already connected, send the request immediately
		// We should have already authorized at this point, since we are resuming a RequestLobbySession join
		ServerRequestLobbyReservation(SessionOwnerUniqueId, PendingReservation, ClientAccountId, ClientUniqueId, AuthorizationParams, bMatchmakingReservation);
		return;
	}

	// Grab the connection address
	FString ConnectionString;
	IOnlineSessionPtr SessionInt = Online::GetSessionInterface();
	if (!SessionInt->GetResolvedConnectString(DesiredHost, BeaconPort, ConnectionString))
	{
		UE_LOG(LogBeacon, Error, TEXT("%s: Unable to find beacon connection string!"), ANSI_TO_TCHAR(__FUNCTION__));
		OnFailure();
		return;
	}

	// Store off the ConnectionString
	PendingConnectionString = ConnectionString;
	if (DesiredHost.Session.SessionSettings.bIsDedicated)
		PendingConnectionString += TEXT("?bIsDedicated");

	// Build our authorization token
	// Only request keys for network encryption when desktop network encryption is required and we're on desktop or connecting to a dedicated server
	BeginLoginForAuthorization(FirstLocalPlayer, FOnILLLobbyBeaconLoginComplete::CreateUObject(this, &ThisClass::OnLoginToConnectComplete, FirstLocalPlayer));
}

bool AILLLobbyBeaconClient::ServerRequestLobbyReservation_Validate(const FUniqueNetIdRepl& OwnerId, const FILLSessionBeaconReservation& Reservation, const FILLBackendPlayerIdentifier& AccountId, const FUniqueNetIdRepl& UniqueId, const FString& AuthParams, const bool bForMatchmaking)
{
	return (Reservation.IsValidReservation() && AccountId.IsValid() && !AuthParams.IsEmpty());
}

void AILLLobbyBeaconClient::ServerRequestLobbyReservation_Implementation(const FUniqueNetIdRepl& OwnerId, const FILLSessionBeaconReservation& Reservation, const FILLBackendPlayerIdentifier& AccountId, const FUniqueNetIdRepl& UniqueId, const FString& AuthParams, const bool bForMatchmaking)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	AuthorizationParams = AuthParams;
	bMatchmakingReservation = bForMatchmaking;

	ServerRequestReservation(OwnerId, Reservation, AccountId, UniqueId);
}

void AILLLobbyBeaconClient::RequestPartyAddition(AILLPartyBeaconMemberState* NewPartyMember)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *NewPartyMember->ToDebugString());

	// Request the addition from the server
	ServerRequestPartyAddition(NewPartyMember->GetAccountId(), NewPartyMember->GetMemberUniqueId(), NewPartyMember->GetDisplayName());
}

bool AILLLobbyBeaconClient::ServerNotifyLeftParty_Validate(const FILLBackendPlayerIdentifier& LeaderAccountId)
{
	return true;
}

void AILLLobbyBeaconClient::ServerNotifyLeftParty_Implementation(const FILLBackendPlayerIdentifier& LeaderAccountId)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: Leader: %s"), ANSI_TO_TCHAR(__FUNCTION__), *LeaderAccountId.ToDebugString());

	// Notify the beacon host object
	if (AILLLobbyBeaconHostObject* BeaconHost = Cast<AILLLobbyBeaconHostObject>(GetBeaconOwner()))
	{
		BeaconHost->NotifyPartyMemberRemoved(LeaderAccountId, GetClientAccountId());
	}
}

bool AILLLobbyBeaconClient::ServerNotifyPartyMemberRemoved_Validate(const FILLBackendPlayerIdentifier& MemberAccountId)
{
	return true;
}

void AILLLobbyBeaconClient::ServerNotifyPartyMemberRemoved_Implementation(const FILLBackendPlayerIdentifier& MemberAccountId)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: Member: %s"), ANSI_TO_TCHAR(__FUNCTION__), *MemberAccountId.ToDebugString());

	// Notify the beacon host object
	if (AILLLobbyBeaconHostObject* BeaconHost = Cast<AILLLobbyBeaconHostObject>(GetBeaconOwner()))
	{
		BeaconHost->NotifyPartyMemberRemoved(GetClientAccountId(), MemberAccountId);
	}
}

void AILLLobbyBeaconClient::ClientPartyAdditionAccepted_Implementation(const FILLBackendPlayerIdentifier& MemberAccountId)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: Member: %s"), ANSI_TO_TCHAR(__FUNCTION__), *MemberAccountId.ToDebugString());

	// Have our party handle the response
	UILLOnlineSessionClient* OSC = CastChecked<UILLOnlineSessionClient>(GetGameInstance()->GetOnlineSession());
	if (OSC->IsInPartyAsLeader())
	{
		OSC->GetPartyHostBeaconObject()->HandlePartyAdditionAccepted(MemberAccountId);
	}
}

void AILLLobbyBeaconClient::ClientPartyAdditionDenied_Implementation(const FILLBackendPlayerIdentifier& MemberAccountId)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: Member: %s"), ANSI_TO_TCHAR(__FUNCTION__), *MemberAccountId.ToDebugString());

	// Have our party handle the response
	UILLOnlineSessionClient* OSC = CastChecked<UILLOnlineSessionClient>(GetGameInstance()->GetOnlineSession());
	if (OSC->IsInPartyAsLeader())
	{
		OSC->GetPartyHostBeaconObject()->HandlePartyAdditionDenied(MemberAccountId);
	}
}

bool AILLLobbyBeaconClient::ServerRequestPartyAddition_Validate(const FILLBackendPlayerIdentifier& MemberAccountId, FUniqueNetIdRepl MemberUniqueId, const FString& DisplayName)
{
	return true;
}

void AILLLobbyBeaconClient::ServerRequestPartyAddition_Implementation(const FILLBackendPlayerIdentifier& MemberAccountId, FUniqueNetIdRepl MemberUniqueId, const FString& DisplayName)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *DisplayName);

	if (AILLLobbyBeaconHostObject* BeaconHost = Cast<AILLLobbyBeaconHostObject>(GetBeaconOwner()))
	{
		if (BeaconHost->AcceptPartyMember(GetClientAccountId(), MemberAccountId, MemberUniqueId, DisplayName))
		{
			ClientPartyAdditionAccepted(MemberAccountId);
		}
		else
		{
			ClientPartyAdditionDenied(MemberAccountId);
		}
	}
}

bool AILLLobbyBeaconClient::BeginAuthorization(FOnILLLobbyBeaconAuthorizationComplete CompletionDelegate)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *GetName());

	// Grab the identity interface
	IOnlineIdentityPtr OnlineIdentity = Online::GetIdentityInterface();
	if (!OnlineIdentity.IsValid())
	{
		UE_LOG(LogBeacon, Warning, TEXT("%s: No IdentityInterface!"), ANSI_TO_TCHAR(__FUNCTION__));
		CompletionDelegate.ExecuteIfBound(this, EILLLobbyBeaconClientAuthorizationResult::Failed);
		return false;
	}

	// Parse the AuthorizationParams
	// Stored in [AuthorizationType].[OnlineEnvironment].[AppId].[AuthToken] format
	TArray<FString> AuthParams;
	AuthorizationParams.ParseIntoArray(AuthParams, AUTH_PARAMS_SEPARATOR, false);
	if (AuthParams.Num() != 4 || AuthParams[0].IsEmpty() || AuthParams[1].IsEmpty() || AuthParams[2].IsEmpty() || AuthParams[3].IsEmpty())
	{
		UE_LOG(LogBeacon, Warning, TEXT("%s: Invalid AuthorizationParams!"), ANSI_TO_TCHAR(__FUNCTION__));
		CompletionDelegate.ExecuteIfBound(this, EILLLobbyBeaconClientAuthorizationResult::Failed);
		return false;
	}
	AuthorizationType = AuthParams[0];
	AppId = AuthParams[2];
	AuthorizationToken = AuthParams[3];
	
	// Set a timeout timer and copy off our completion delegate now
	AuthorizationDelegate = CompletionDelegate;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_AuthTimeout, this, &ThisClass::OnAuthorizationTimeout, AuthorizationTimeout);

	// Begin the authorization sequence
	if (OnlineIdentity->GetAuthType() == AuthorizationType)
	{
		// Start the local platform authorization sequence
		// NOTE: When AuthSessionStart returns true AuthorizationComplete was already hit on platforms that don't use this
		IOnlineSessionPtr SessionInt = Online::GetSessionInterface();
		AuthSessionStartCompleteDelegateHandle = SessionInt->AddOnAuthSessionStartCompleteDelegate_Handle(FOnAuthSessionStartCompleteDelegate::CreateUObject(this, &ThisClass::OnAuthSessionStartComplete));
		if (SessionInt->AuthSessionStart(AuthorizationToken, *ClientUniqueId))
		{
			return true;
		}
		else
		{
			UE_LOG(LogBeacon, Warning, TEXT("%s: AuthSessionStart failed!"), ANSI_TO_TCHAR(__FUNCTION__));
		}
	}
	else
	{
		// Receiving a connection from another platform
		if (AuthorizationType == TEXT("psn"))
		{
			const EOnlineEnvironment::Type OnlineEnvironment = FromUniqueCharacter(AuthParams[1][0]);
			if (OnlineEnvironment == EOnlineEnvironment::Unknown)
			{
				UE_LOG(LogBeacon, Warning, TEXT("%s: Invalid OnlineEnvironment \"%s\"!"), ANSI_TO_TCHAR(__FUNCTION__), *AuthParams[1]);
				//CompletionDelegate.ExecuteIfBound(this, EILLLobbyBeaconClientAuthorizationResult::Failed);
				AuthorizationComplete(EILLLobbyBeaconClientAuthorizationResult::Failed);
				return false;
			}

			// Start the PSN authorization sequence
			UILLGameInstance* GameInstance = CastChecked<UILLGameInstance>(GetGameInstance());
			if (GameInstance->GetPSNRequestManager()->StartAuthorizationSequence(AppId, AuthorizationToken, OnlineEnvironment, FOnILLPSNClientAuthorzationCompleteDelegate::CreateUObject(this, &ThisClass::OnPSNClientAuthorzationComplete)))
			{
				return true;
			}

			// StartAuthorizationSequence failed
			UE_LOG(LogBeacon, Warning, TEXT("%s: Failed to start PSN authorization sequence!"), ANSI_TO_TCHAR(__FUNCTION__));
		}
		else if (AuthorizationType == TEXT("xbl"))
		{
			// Skip to authorization completion, which will create the XBL session if needed
			AuthSandbox = AuthParams[1];
			//CompletionDelegate.ExecuteIfBound(this, EILLLobbyBeaconClientAuthorizationResult::Success);
			AuthorizationComplete(EILLLobbyBeaconClientAuthorizationResult::Success);
			return true;
		}
		else
		{
			UE_LOG(LogBeacon, Warning, TEXT("%s: Unknown AuthorizationType \"%s\"!"), ANSI_TO_TCHAR(__FUNCTION__), *AuthorizationType);
		}
	}

	// If we made it down here, authorization failed
	//CompletionDelegate.ExecuteIfBound(this, EILLLobbyBeaconClientAuthorizationResult::Failed);
	AuthorizationComplete(EILLLobbyBeaconClientAuthorizationResult::Failed);
	return false;
}

void AILLLobbyBeaconClient::EndAuthorization()
{
	if (AuthorizationDelegate.IsBound())
	{
		// We have not completed BeginAuthorization
		AuthorizationComplete(EILLLobbyBeaconClientAuthorizationResult::Failed);
	}
	else if (bAuthorized)
	{
		// We have completely authorized, clean that up
		if (ClientUniqueId.IsValid())
		{
			IOnlineSessionPtr SessionInt = Online::GetSessionInterface();
			SessionInt->EndAuthSession(*ClientUniqueId);
		}
	}
}

bool AILLLobbyBeaconClient::BeginLoginForAuthorization(UILLLocalPlayer* FirstLocalPlayer, FOnILLLobbyBeaconLoginComplete CompletionDelegate)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *GetName());

	// We need to run the Login process to generate a new auth code for the sake of PS4, Steam and XB1 generally just finish immediately
	IOnlineIdentityPtr OnlineIdentity = Online::GetIdentityInterface();
	LoginCompleteDelegate = OnlineIdentity->AddOnLoginCompleteDelegate_Handle(FirstLocalPlayer->GetControllerId(), FOnLoginCompleteDelegate::CreateUObject(this, &ThisClass::OnLoginForAuthorizationComplete, FirstLocalPlayer->GetOnlineEnvironment(), CompletionDelegate));
	return OnlineIdentity->AutoLogin(FirstLocalPlayer->GetControllerId());
}

void AILLLobbyBeaconClient::OnLoginForAuthorizationComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error, const EOnlineEnvironment::Type OnlineEnvironment, FOnILLLobbyBeaconLoginComplete CompletionDelegate)
{
	if (bWasSuccessful)
	{
		UE_LOG(LogBeacon, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *GetName());
	}
	else
	{
		UE_LOG(LogBeacon, Warning, TEXT("%s: %s Failed! Error: %s!"), ANSI_TO_TCHAR(__FUNCTION__), *GetName(), *Error);
	}

	AuthorizationParams.Empty();
	AuthorizationType.Empty();
	AppId.Empty();
	AuthorizationToken.Empty();

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	IOnlineIdentityPtr OnlineIdentity = Online::GetIdentityInterface();
	if (OnlineIdentity.IsValid())
	{
		OnlineIdentity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, LoginCompleteDelegate);
	}
	if (bWasSuccessful && ensure(OnlineSubsystem) && ensure(OnlineIdentity.IsValid()))
	{
		AuthorizationType = OnlineIdentity->GetAuthType();
		AppId = OnlineSubsystem->GetAppId();
#if PLATFORM_XBOXONE
		AuthorizationToken = UserId.ToString();
#else
		AuthorizationToken = OnlineIdentity->GetAuthToken(LocalUserNum);
#endif

		AuthorizationParams = AuthorizationType;
		AuthorizationParams += AUTH_PARAMS_SEPARATOR;
#if PLATFORM_XBOXONE
		AuthorizationParams += Windows::Xbox::Services::XboxLiveConfiguration::SandboxId->Data();
#else
		AuthorizationParams.AppendChar(ToUniqueCharacter(OnlineEnvironment));
#endif
		AuthorizationParams += AUTH_PARAMS_SEPARATOR;
		AuthorizationParams += AppId;
		AuthorizationParams += AUTH_PARAMS_SEPARATOR;
		AuthorizationParams += AuthorizationToken;

		CompletionDelegate.ExecuteIfBound(true, Error);
	}
	else
	{
		CompletionDelegate.ExecuteIfBound(false, Error);
	}
}

void AILLLobbyBeaconClient::AuthorizationComplete(const EILLLobbyBeaconClientAuthorizationResult Result)
{
	if (Result == EILLLobbyBeaconClientAuthorizationResult::Success)
	{
		UE_LOG(LogBeacon, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *GetName());
	}
	else
	{
		UE_LOG(LogBeacon, Warning, TEXT("%s: %s Result: %s!"), ANSI_TO_TCHAR(__FUNCTION__), *GetName(), ToString(Result));
	}

	// Cleanup our delegate
	IOnlineSessionPtr SessionInt = Online::GetSessionInterface();
	if (AuthSessionStartCompleteDelegateHandle.IsValid() && SessionInt.IsValid())
	{
		SessionInt->ClearOnAuthSessionStartCompleteDelegate_Handle(AuthSessionStartCompleteDelegateHandle);
		AuthSessionStartCompleteDelegateHandle.Reset();
	}

	// Cancel timeout
	if (TimerHandle_AuthTimeout.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_AuthTimeout);
		TimerHandle_AuthTimeout.Invalidate();
	}

	// Broadcast completion
	// Unbind the delegate so EndAuthorization is aware of completion
	bAuthorized = (Result == EILLLobbyBeaconClientAuthorizationResult::Success);
	AuthorizationDelegate.ExecuteIfBound(this, Result);
	AuthorizationDelegate.Unbind();
}

void AILLLobbyBeaconClient::OnAuthSessionStartComplete(bool bWasSuccessful, const FUniqueNetId& PlayerId)
{
	if (!ensure(ClientUniqueId.IsValid()) || *ClientUniqueId != PlayerId)
		return;

	// Broadcast completion
	AuthorizationComplete(bWasSuccessful ? EILLLobbyBeaconClientAuthorizationResult::Success : EILLLobbyBeaconClientAuthorizationResult::Failed);
}

void AILLLobbyBeaconClient::OnPSNClientAuthorzationComplete(bool bWasSuccessful)
{
	// Broadcast completion
	AuthorizationComplete(bWasSuccessful ? EILLLobbyBeaconClientAuthorizationResult::Success : EILLLobbyBeaconClientAuthorizationResult::Failed);
}

void AILLLobbyBeaconClient::OnAuthorizationTimeout()
{
	UE_LOG(LogBeacon, Warning, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Clear timer
	check(TimerHandle_AuthTimeout.IsValid());
	TimerHandle_AuthTimeout.Invalidate();
	
	// Fail to authenticate
	AuthorizationComplete(EILLLobbyBeaconClientAuthorizationResult::TimedOut);
}

#if USE_GAMELIFT_SERVER_SDK
bool AILLLobbyBeaconClient::PerformMatchmakingBypass(AILLLobbyBeaconState* LobbyState, FOnILLLobbyBeaconMatchmakingBypassComplete CompletionDelegate)
{
	// Build a bypass request
	UILLGameInstance* GI = CastChecked<UILLGameInstance>(GetGameInstance());
	UILLBackendRequestManager* BRM = GI->GetBackendRequestManager();
	AILLLobbyBeaconMemberState* MemberState = CastChecked<AILLLobbyBeaconMemberState>(LobbyState->FindMember(this));
	UILLOnlineMatchmakingClient* OMC = GI->GetOnlineMatchmaking();

	// Grab our long region name
	FString ServerRegion;
	FParse::Value(FCommandLine::Get(), TEXT("ServerRegion="), ServerRegion);
	if (ServerRegion.IsEmpty())
	{
		UE_LOG(LogBeacon, Warning, TEXT("%s: ServerRegion is empty! HTTP bypass will not function!"), ANSI_TO_TCHAR(__FUNCTION__));
	}
	else
	{
		// Build and queue an HTTP request
		UILLGameEngine* GameEngine = CastChecked<UILLGameEngine>(GEngine);
		UILLBackendSimpleHTTPRequestTask* MatchmakingBypassRequestTask = BRM->CreateSimpleRequest(
			BRM->MatchmakingService_Bypass,
			FOnILLBackendSimpleRequestDelegate::CreateUObject(this, &ThisClass::MatchmakingBypassResponse),
			FString::Printf(TEXT("?ClientRegion=%s&GameSessionID=%s&PlayerID=%s"), *ServerRegion, *GameEngine->GetGameLiftGameSessionId(), *MemberState->GetAccountId().GetIdentifier()));
		if (MatchmakingBypassRequestTask)
		{
			// Send it
			MatchmakingBypassDelegate = CompletionDelegate;
			MatchmakingBypassRequestTask->QueueRequest();
			return true;
		}
	}

	CompletionDelegate.ExecuteIfBound(this, false);
	return false;
}

void AILLLobbyBeaconClient::MatchmakingBypassResponse(int32 ResponseCode, const FString& ResponseContent)
{
	bool bWasSuccessful = false;
	if (ResponseCode == 200 && !ResponseContent.IsEmpty())
	{
		UE_LOG(LogBeacon, Verbose, TEXT("%s: ResponseCode: %i"), ANSI_TO_TCHAR(__FUNCTION__), ResponseCode);

		// Parse response JSON
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ResponseContent);
		if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
		{
			FString IpAddress;
			int32 Port = 0;
			FString PlayerId;
			FString PlayerSessionId;
			if (!JsonObject->TryGetStringField(TEXT("IpAddress"), IpAddress))
			{
				UE_LOG(LogBeacon, Warning, TEXT("%s: Failed to find IpAddress field!"), ANSI_TO_TCHAR(__FUNCTION__));
			}
			else if (!JsonObject->TryGetNumberField(TEXT("Port"), Port))
			{
				UE_LOG(LogBeacon, Warning, TEXT("%s: Failed to find Port field!"), ANSI_TO_TCHAR(__FUNCTION__));
			}
			else if (!JsonObject->TryGetStringField(TEXT("PlayerId"), PlayerId))
			{
				UE_LOG(LogBeacon, Warning, TEXT("%s: Failed to find PlayerId field!"), ANSI_TO_TCHAR(__FUNCTION__));
			}
			else if (!JsonObject->TryGetStringField(TEXT("PlayerSessionId"), PlayerSessionId))
			{
				UE_LOG(LogBeacon, Warning, TEXT("%s: Failed to find PlayerSessionId field!"), ANSI_TO_TCHAR(__FUNCTION__));
			}
			else
			{
				// Success!
				bWasSuccessful = true;
				ClientPlayerSessionId = PlayerSessionId;
			}
		}
		else
		{
			// Flag failure
			UE_LOG(LogBeacon, Warning, TEXT("%s: Failed to parse ResponseContent as JSON!"), ANSI_TO_TCHAR(__FUNCTION__));
		}
	}
	else
	{
		UE_LOG(LogBeacon, Error, TEXT("%s: ResponseCode: %i, ResponseContent: '%s'"), ANSI_TO_TCHAR(__FUNCTION__), ResponseCode, *ResponseContent);
	}

	MatchmakingBypassDelegate.ExecuteIfBound(this, bWasSuccessful);
	MatchmakingBypassDelegate.Unbind();
}
#endif // USE_GAMELIFT_SERVER_SDK
