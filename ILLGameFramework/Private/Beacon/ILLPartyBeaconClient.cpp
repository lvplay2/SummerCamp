// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLPartyBeaconClient.h"

#include "OnlineSubsystem.h"
#include "OnlineSessionInterface.h"
#include "OnlineSubsystemUtils.h"

#include "ILLBackendPlayer.h"
#include "ILLBuildDefines.h"
#include "ILLGameInstance.h"
#include "ILLGame_Menu.h"
#include "ILLLobbyBeaconClient.h"
#include "ILLLobbyBeaconHostObject.h"
#include "ILLLobbyBeaconMemberState.h"
#include "ILLLobbyBeaconState.h"
#include "ILLLocalPlayer.h"
#include "ILLOnlineMatchmakingClient.h"
#include "ILLOnlineSessionClient.h"
#include "ILLPartyBeaconHostObject.h"
#include "ILLPartyBeaconMemberState.h"
#include "ILLPartyBeaconState.h"

AILLPartyBeaconClient::AILLPartyBeaconClient(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	JoinLeaderSearchAttempts = 8; // You will want this to be higher than your party max size for Steam
	JoinLeaderRetryDelay = 2.f;

#if PLATFORM_DESKTOP
	FindLeaderGameSessionCompleteDelegate = FOnFindFriendSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnFindLeaderGameSessionComplete);
#endif
	JoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete);
}

void AILLPartyBeaconClient::Destroyed()
{
	// Notify our "parent" lobby beacon that we're leaving a party
	if (UILLOnlineSessionClient* OSC = GetGameInstance() ? CastChecked<UILLOnlineSessionClient>(GetGameInstance()->GetOnlineSession()) : nullptr)
	{
		// NOTE: AILLPartyBeaconHostObject::NotifyClientDisconnected takes care of this when leading
		if (!OSC->IsInPartyAsLeader())
		{
			if (AILLLobbyBeaconClient* LobbyClient = OSC->GetLobbyClientBeacon())
			{
				LobbyClient->ServerNotifyLeftParty(GetBeaconOwnerAccountId());
			}
			else if (AILLLobbyBeaconHostObject* LobbyHost = OSC->GetLobbyHostBeaconObject())
			{
				LobbyHost->NotifyPartyMemberRemoved(GetBeaconOwnerAccountId(), GetClientAccountId());
			}
		}
	}

	Super::Destroyed();
}

bool AILLPartyBeaconClient::ShouldReceiveVoiceFrom(const FUniqueNetIdRepl& Sender) const
{
	return CanUsePartyVoice();
}

bool AILLPartyBeaconClient::ShouldSendVoice() const
{
	return CanUsePartyVoice();
}

bool AILLPartyBeaconClient::InitClientBeacon(const FString& ConnectionString, const FILLBackendPlayerIdentifier& AccountId, const FUniqueNetIdRepl& UniqueId)
{
#if PLATFORM_DESKTOP && ILLNETENCRYPTION_DESKTOP_REQUIRED
	// Use party socket encryption
	UILLGameInstance* GI = CastChecked<UILLGameInstance>(GetGameInstance());
	UILLBackendRequestManager* BRM = GI->GetBackendRequestManager();
	if (BRM->KeyDistributionService_Create && BRM->KeyDistributionService_Retrieve)
	{
		FString Token;
		if (GI->KeySessionToEncryptionToken(TEXT("Party"), Token))
		{
			SetEncryptionToken(Token);
		}
	}
#endif

	return Super::InitClientBeacon(ConnectionString, AccountId, UniqueId);
}

void AILLPartyBeaconClient::SynchedBeaconMembers()
{
	// Start the session
	UILLOnlineSessionClient* OSC = CastChecked<UILLOnlineSessionClient>(GetGameInstance()->GetOnlineSession());
	OSC->StartOnlineSession(NAME_PartySession);

	// Register existing members with the party session
	// CheckHasSynchronized should call to RegisterWithPartySession
	if (AILLPartyBeaconState* PartyState = Cast<AILLPartyBeaconState>(SessionBeaconState))
	{
		for (int32 MemberIndex = 0; MemberIndex < PartyState->GetNumMembers(); ++MemberIndex)
		{
			if (AILLPartyBeaconMemberState* MemberState = Cast<AILLPartyBeaconMemberState>(PartyState->GetMemberAtIndex(MemberIndex)))
			{
				MemberState->CheckHasSynchronized();
			}
		}
	}

	// Broadcast completion
	Super::SynchedBeaconMembers();
}

void AILLPartyBeaconClient::OnConnected()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Request membership immediately
	ServerRequestReservation(SessionOwnerUniqueId, PendingReservation, ClientAccountId, ClientUniqueId);
}

void AILLPartyBeaconClient::HandleReservationAccepted(AILLSessionBeaconState* InSessionBeaconState)
{
	Super::HandleReservationAccepted(InSessionBeaconState);

	if (bInitialMemberSyncComplete && bReservationAccepted)
	{
		// Tell them to join us immediately
		JoinLeadersGameSession(GetValidNamedSession(NAME_GameSession));
	}
}

void AILLPartyBeaconClient::ServerSynchronizedMembers_Implementation()
{
	Super::ServerSynchronizedMembers_Implementation();

	if (bInitialMemberSyncComplete && bReservationAccepted)
	{
		// Tell them to join us immediately
		JoinLeadersGameSession(GetValidNamedSession(NAME_GameSession));
	}
}

bool AILLPartyBeaconClient::CanUsePartyVoice() const
{
	if (UILLOnlineSessionClient* OSC = Cast<UILLOnlineSessionClient>(GetGameInstance()->GetOnlineSession()))
	{
		if (AILLLobbyBeaconState* LobbyState = OSC->GetLobbyBeaconState())
		{
			return false;
		}
	}

	return true;
}

bool AILLPartyBeaconClient::RequestJoinParty(const FOnlineSessionSearchResult& DesiredHost, UILLLocalPlayer* FirstLocalPlayer)
{
	if (!FirstLocalPlayer)
		return false;

	// Send a reservation with us as the only member, no leader since that is DesiredHost
	FILLSessionBeaconReservation Reservation;
	Reservation.Players.Add(FILLSessionBeaconPlayer(FirstLocalPlayer));

	return RequestReservation(DesiredHost, Reservation, FirstLocalPlayer->GetBackendPlayer()->GetAccountId(), FirstLocalPlayer->GetPreferredUniqueNetId());
}

void AILLPartyBeaconClient::ClientNotifyMatchmakingStart_Implementation()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	PartyMatchmakingStarted.ExecuteIfBound();
}

void AILLPartyBeaconClient::ClientMatchmakingStateUpdate_Implementation(const uint8 NewState)
{
	const EILLMatchmakingState _NewState = static_cast<EILLMatchmakingState>(NewState);

	UE_LOG(LogBeacon, Verbose, TEXT("%s: NewState: %s"), ANSI_TO_TCHAR(__FUNCTION__), ToString(_NewState));

	PartyMatchmakingUpdated.ExecuteIfBound(_NewState);
}

void AILLPartyBeaconClient::ClientNotifyMatchmakingStop_Implementation()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	PartyMatchmakingStopped.ExecuteIfBound();
}

void AILLPartyBeaconClient::ClientMatchmakingFollow_Implementation(const FString& SessionId, const FString& ConnectionString, const FString& PlayerSessionId)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: SessionId: %s"), ANSI_TO_TCHAR(__FUNCTION__), *SessionId);

	// Store the SessionId to suppress subsequent calls to ClientJoinLeadersGameSession suppress it for this session
	FollowingLeaderToSessionId = SessionId;

	// Start our own request to join locally
	UILLOnlineSessionClient* OSC = CastChecked<UILLOnlineSessionClient>(GetGameInstance()->GetOnlineSession());
	OSC->BeginRequestLobbySession(ConnectionString, PlayerSessionId, FILLOnLobbyBeaconSessionReceived(), FILLOnLobbyBeaconSessionRequestComplete::CreateUObject(this, &ThisClass::OnMatchmakingRequestSessionComplete));
}

void AILLPartyBeaconClient::OnMatchmakingRequestSessionComplete(const EILLLobbyBeaconSessionResult Response)
{
	if (Response != EILLLobbyBeaconSessionResult::Success)
	{
		// Leave the party for follow failure
		PartyJoinLeaderGameSessionFailed.ExecuteIfBound();
	}
}

void AILLPartyBeaconClient::JoinLeadersGameSession(FNamedOnlineSession* OnlineSession)
{
	if (OnlineSession && OnlineSession->SessionInfo.IsValid() && OnlineSession->SessionInfo->IsValid())
	{
		DesiredGameSessionId = OnlineSession->SessionInfo->GetSessionId().ToString();
		DesiredGameSessionOwner = OnlineSession->OwningUserId;

		UE_LOG(LogBeacon, Verbose, TEXT("%s: DesiredGameSessionId: %s, DesiredGameSessionOwner: %s"), ANSI_TO_TCHAR(__FUNCTION__), *DesiredGameSessionId, SafeUniqueIdTChar(DesiredGameSessionOwner));
	}
	else
	{
		UE_LOG(LogBeacon, Verbose, TEXT("%s: Default map"), ANSI_TO_TCHAR(__FUNCTION__));

		DesiredGameSessionId = FString();
		DesiredGameSessionOwner = FUniqueNetIdRepl();
	}

	// Tell the client
	ClientJoinLeadersGameSession(DesiredGameSessionId, DesiredGameSessionOwner);
	ForceNetUpdate();
}

void AILLPartyBeaconClient::ClientJoinLeadersGameSession_Implementation(const FString& WithSessionID, const FUniqueNetIdRepl& WithSessionOwnerID)
{
	if (!WithSessionID.IsEmpty() || WithSessionOwnerID.IsValid())
	{
		UE_LOG(LogBeacon, Verbose, TEXT("%s: SessionId: %s, OwnerID: %s"), ANSI_TO_TCHAR(__FUNCTION__), *WithSessionID, SafeUniqueIdTChar(WithSessionOwnerID));
	}
	else
	{
		UE_LOG(LogBeacon, Verbose, TEXT("%s: Default map"), ANSI_TO_TCHAR(__FUNCTION__));
	}

	IOnlineSessionPtr Sessions = Online::GetSessionInterface();
	FNamedOnlineSession* CurrentGameSession = Sessions.IsValid() ? Sessions->GetNamedSession(NAME_GameSession) : nullptr;

	// Setup retries
	DesiredGameSessionId = WithSessionID;
	DesiredGameSessionOwner = WithSessionOwnerID;

	if (!DesiredGameSessionId.IsEmpty() || WithSessionOwnerID.IsValid())
	{
#if PLATFORM_DESKTOP
		if (FindLeaderGameSessionCompleteDelegateHandle.IsValid())
#else
		if (FindLeaderGameSessionCompleteDelegate.IsBound())
#endif
		{
			// If we already are searching, do nothing and just wait for the previous search to complete
			// The previous search attempt should fail due to DesiredGameSessionOwner no longer matching in OnFindLeaderGameSessionComplete anyways
			AckJoinLeaderSession();
		}
		else if ((!FollowingLeaderToSessionId.IsEmpty() && FollowingLeaderToSessionId == WithSessionID))
		{
			AckJoinLeaderSession();
		}
		else if (CurrentGameSession &&
			((WithSessionOwnerID.IsValid() && CurrentGameSession->OwningUserId.IsValid() && *CurrentGameSession->OwningUserId == *WithSessionOwnerID) ||
			(!WithSessionID.IsEmpty() && CurrentGameSession->SessionInfo.IsValid() && CurrentGameSession->SessionInfo->GetSessionId().ToString() == WithSessionID))
		)
		{
			// Already in a matching game session
			JoinedLeaderSession();
		}
		else
		{
			// Search for the leader's game session, and tell them we are
			RemainingSearchAttempts = JoinLeaderSearchAttempts;
			StartOrRetryFindLeaderGameSession();
			AckJoinLeaderSession();
		}
	}
	else
	{
		UWorld* GameWorld = GetGameInstance()->GetWorld();
		if (!CurrentGameSession && GameWorld && GameWorld->GetAuthGameMode<AILLGame_Menu>())
		{
			// Not in a game session
			// Notify the party leader we are good to go
			JoinedLeaderSession();
		}
		else
		{
			// We are in a game session
			// Just tell the server we have joined after assigning an invalid DesiredGameSessionOwner, before returning to the main menu
			DesiredGameSessionId = FString();
			DesiredGameSessionOwner = FUniqueNetIdRepl();
			JoinedLeaderSession();
			GetGameInstance()->GetFirstLocalPlayerController()->ClientReturnToMainMenu(FText());
		}
	}
}

void AILLPartyBeaconClient::AckJoinLeaderSession()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Let the party leader know
	ServerAckJoinLeaderSession();
}

bool AILLPartyBeaconClient::ServerAckJoinLeaderSession_Validate()
{
	return true;
}

void AILLPartyBeaconClient::ServerAckJoinLeaderSession_Implementation()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	if (AILLPartyBeaconHostObject* BeaconHost = Cast<AILLPartyBeaconHostObject>(GetBeaconOwner()))
	{
		BeaconHost->HandleMemberAckJoinGameSession(this);
	}
}

void AILLPartyBeaconClient::JoinedLeaderSession()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Let the party leader know
	ServerJoinedLeaderSession();
}

bool AILLPartyBeaconClient::ServerJoinedLeaderSession_Validate()
{
	return true;
}

void AILLPartyBeaconClient::ServerJoinedLeaderSession_Implementation()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	if (AILLPartyBeaconHostObject* BeaconHost = Cast<AILLPartyBeaconHostObject>(GetBeaconOwner()))
	{
		BeaconHost->HandleMemberJoinedGameSession(this);
	}
}

void AILLPartyBeaconClient::FailedToFindLeaderSession()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Let the party leader know
	ServerFailedToFindLeaderSession();
}

bool AILLPartyBeaconClient::ServerFailedToFindLeaderSession_Validate()
{
	return true;
}

void AILLPartyBeaconClient::ServerFailedToFindLeaderSession_Implementation()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	if (AILLPartyBeaconHostObject* BeaconHost = Cast<AILLPartyBeaconHostObject>(GetBeaconOwner()))
	{
		BeaconHost->HandleMemberFailedToFindGameSession(this);
	}
}

void AILLPartyBeaconClient::FailedToJoinLeaderSession(uint8 Result)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Let the party leader know
	ServerFailedToJoinLeaderSession(Result);
}

bool AILLPartyBeaconClient::ServerFailedToJoinLeaderSession_Validate(uint8 Result)
{
	// Kick them on join failure
	return (Result != static_cast<uint8>(EOnJoinSessionCompleteResult::Success));
}

void AILLPartyBeaconClient::ServerFailedToJoinLeaderSession_Implementation(uint8 Result)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	if (AILLPartyBeaconHostObject* BeaconHost = Cast<AILLPartyBeaconHostObject>(GetBeaconOwner()))
	{
		BeaconHost->HandleMemberFailedToJoinGameSession(this, EOnJoinSessionCompleteResult::Type(Result));
	}
}

void AILLPartyBeaconClient::StartOrRetryFindLeaderGameSession()
{	
	IOnlineSessionPtr Sessions = Online::GetSessionInterface();

	UWorld* GameWorld = GetGameInstance()->GetWorld();
	AILLPartyBeaconState* PartyState = Cast<AILLPartyBeaconState>(SessionBeaconState);
	if (UILLLocalPlayer* FirstLocalPlayer = (GameWorld && PartyState) ? Cast<UILLLocalPlayer>(GameWorld->GetFirstLocalPlayerFromController()) : nullptr)
	{
		UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

		// Failure handling will happen in the delegate since that always fires for this call, nice and consistent!
#if PLATFORM_DESKTOP
		FindLeaderGameSessionCompleteDelegateHandle = Sessions->AddOnFindFriendSessionCompleteDelegate_Handle(FirstLocalPlayer->GetControllerId(), FindLeaderGameSessionCompleteDelegate);

		// With Steam try every member of the party
		FUniqueNetIdRepl MemberId;
		const int32 MemberToTry = RemainingSearchAttempts % PartyState->GetNumMembers();
		if (AILLSessionBeaconMemberState* Member = PartyState->GetMemberAtIndex(MemberToTry))
		{
			if (Member->HasFullySynchronized())
				MemberId = Member->GetMemberUniqueId();
		}

		// Fallback to the owner for incomplete members
		if (!MemberId.IsValid())
			MemberId = SessionOwnerUniqueId;
		const FUniqueNetId& FriendId = *MemberId.GetUniqueNetId().Get();
		if (Sessions->FindFriendSession(FirstLocalPlayer->GetControllerId(), FriendId))
		{
			PartyFindingLeaderGameSession.ExecuteIfBound();
		}
#else
		const FUniqueNetId& FriendId = *SessionOwnerUniqueId.GetUniqueNetId().Get();
		FindLeaderGameSessionCompleteDelegate = FOnSingleSessionResultCompleteDelegate::CreateUObject(this, &ThisClass::OnFindLeaderGameSessionComplete);
		Sessions->FindSessionById(*FirstLocalPlayer->GetCachedUniqueNetId(), FUniqueNetIdString(DesiredGameSessionId), FriendId, FindLeaderGameSessionCompleteDelegate);
#endif
	}
}

void AILLPartyBeaconClient::HandleFindResult(const FOnlineSessionSearchResult& FriendSearchResult)
{
	if (FriendSearchResult.IsValid())
	{
		// Fire our delegate to start joining
		if (PartyTravelRequested.Execute(FriendSearchResult, JoinSessionCompleteDelegate))
		{
			UE_LOG(LogBeacon, Verbose, TEXT("%s: Travel started"), ANSI_TO_TCHAR(__FUNCTION__));
			return;
		}

		UE_LOG(LogBeacon, Error, TEXT("%s: Unable to send join request!"), ANSI_TO_TCHAR(__FUNCTION__));
	}
	else
	{
		UE_LOG(LogBeacon, Warning, TEXT("%s: Not successful!"), ANSI_TO_TCHAR(__FUNCTION__));
	}

	// Retry the search
	--RemainingSearchAttempts;
	if (RemainingSearchAttempts >= 0 && (!DesiredGameSessionId.IsEmpty() || DesiredGameSessionOwner.IsValid()))
	{
		// Try harder!
		FTimerHandle ThrowAwayHandle;
		GetWorld()->GetTimerManager().SetTimer(ThrowAwayHandle, this, &ThisClass::StartOrRetryFindLeaderGameSession, JoinLeaderRetryDelay);
	}
	else
	{
		PartyJoinLeaderGameSessionFailed.ExecuteIfBound();
		FailedToFindLeaderSession();
	}
}

#if PLATFORM_DESKTOP
void AILLPartyBeaconClient::OnFindLeaderGameSessionComplete(int32 LocalUserNum, bool bWasSuccessful, const TArray<FOnlineSessionSearchResult>& SearchResults)
{
	IOnlineSessionPtr Sessions = Online::GetSessionInterface();
	Sessions->ClearOnFindFriendSessionCompleteDelegate_Handle(LocalUserNum, FindLeaderGameSessionCompleteDelegateHandle);
	FindLeaderGameSessionCompleteDelegateHandle.Reset();

	FOnlineSessionSearchResult FriendSearchResult;
	if (SearchResults.Num() > 0)
		FriendSearchResult = SearchResults[0];

	// Attempt travel
	FOnlineSessionSearchResult Result;
	if (bWasSuccessful)
	{
		Result = FriendSearchResult;
	}
	HandleFindResult(Result);
}
#else
void AILLPartyBeaconClient::OnFindLeaderGameSessionComplete(int32 LocalUserNum, bool bWasSuccessful, const FOnlineSessionSearchResult& SearchResult)
{
	FindLeaderGameSessionCompleteDelegate.Unbind();

	// Attempt travel
	FOnlineSessionSearchResult Result;
	if (bWasSuccessful)
	{
		Result = SearchResult;
	}
	HandleFindResult(Result);
}
#endif

void AILLPartyBeaconClient::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: Result: %s"), ANSI_TO_TCHAR(__FUNCTION__), *GetLocalizedDescription(SessionName, Result).ToString());

	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		// Joined and resolved, let our party leader know
		JoinedLeaderSession();
	}

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LogBeacon, Error, TEXT("%s: Failed to join session! Result: %d"), ANSI_TO_TCHAR(__FUNCTION__), static_cast<uint8>(Result));

		PartyJoinLeaderGameSessionFailed.ExecuteIfBound();
		FailedToJoinLeaderSession(Result);
	}
}

void AILLPartyBeaconClient::ClientCancelGameSessionTravel_Implementation()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	PartyJoinLeaderGameSessionFailed.ExecuteIfBound();
}
