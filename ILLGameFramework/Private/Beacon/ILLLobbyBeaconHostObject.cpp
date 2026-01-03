// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLLobbyBeaconHostObject.h"

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
#include "ILLGameMode.h"
#include "ILLGameSession.h"
#include "ILLLobbyBeaconClient.h"
#include "ILLLobbyBeaconMemberState.h"
#include "ILLLobbyBeaconState.h"
#include "ILLLocalPlayer.h"
#include "ILLPartyBeaconState.h"
#include "ILLPlayerController.h"
#include "ILLPSNRequestManager.h"

AILLLobbyBeaconHostObject::AILLLobbyBeaconHostObject(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	ClientBeaconActorClass = AILLLobbyBeaconClient::StaticClass();
	BeaconTypeName = ClientBeaconActorClass->GetName();

	BeaconStateClass = AILLLobbyBeaconState::StaticClass();
}

void AILLLobbyBeaconHostObject::NotifyClientDisconnected(AOnlineBeaconClient* LeavingClientActor)
{
	FString AuthorizationType;
	if (AILLLobbyBeaconClient* LobbyClient = Cast<AILLLobbyBeaconClient>(LeavingClientActor))
	{
		// Store this for below
		AuthorizationType = LobbyClient->GetAuthorizationType();

		// Find the corresponding player controller and make sure they are booted
		AILLGameMode* CurrentGameMode = GetGameMode();
		AILLGameSession* CurrentGameSession = CurrentGameMode ? Cast<AILLGameSession>(CurrentGameMode->GameSession) : nullptr;
		AILLPlayerController* RelatedPlayerController = LobbyClient->FindGamePlayerController();
		if (CurrentGameSession && IsValid(RelatedPlayerController))
		{
			UE_LOG(LogBeacon, Verbose, TEXT("%s: Kicking related PlayerController %s"), ANSI_TO_TCHAR(__FUNCTION__), *RelatedPlayerController->GetName());
			if (!CurrentGameSession->KickPlayer(RelatedPlayerController, FText()))
			{
				UE_LOG(LogBeacon, Error, TEXT("%s: Failed to kick related PlayerController %s!"), ANSI_TO_TCHAR(__FUNCTION__), *RelatedPlayerController->GetName());
			}
		}

#if USE_GAMELIFT_SERVER_SDK
		// Notify GameLift that this player slot is now available
		if (IsRunningDedicatedServer() && !LobbyClient->GetClientPlayerSessionId().IsEmpty())
		{
			UE_LOG(LogBeacon, Verbose, TEXT("%s: Removing PlayerSession %s"), ANSI_TO_TCHAR(__FUNCTION__), *LobbyClient->GetClientPlayerSessionId());
			UILLGameEngine* GameEngine = CastChecked<UILLGameEngine>(GEngine);
			FGameLiftServerSDKModule& GameLiftModule = GameEngine->GetGameLiftModule();
			GameLiftModule.RemovePlayerSession(LobbyClient->GetClientPlayerSessionId());
		}
#endif // USE_GAMELIFT_SERVER_SDK

		// Cleanup our auth session
		LobbyClient->EndAuthorization();
	}

	// Now destroy it, removing it from ClientActors
	Super::NotifyClientDisconnected(LeavingClientActor);

#if PLATFORM_DESKTOP
	// Cleanup PSN and XBL sessions when all of those types of users have left
	if (IsRunningDedicatedServer())
	{
		int32 ClientsWithSameAuthorizationType = 0;
		for (int32 ClientIndex = 0; ClientIndex < ClientActors.Num(); ++ClientIndex)
		{
			if (AILLLobbyBeaconClient* ClientActor = Cast<AILLLobbyBeaconClient>(ClientActors[ClientIndex]))
			{
				if (ClientActor->GetAuthorizationType() == AuthorizationType)
					++ClientsWithSameAuthorizationType;
			}
		}

		if (ClientsWithSameAuthorizationType == 0)
		{
			UILLGameInstance* GameInstance = CastChecked<UILLGameInstance>(GetGameInstance());
			if (AuthorizationType == TEXT("psn"))
			{
				GameInstance->GetPSNRequestManager()->DestroyGameSession();
			}
			else if (AuthorizationType == TEXT("xbl"))
			{
				GameInstance->GetBackendRequestManager()->ClearActiveXBLSessionUri();
			}
		}
	}
#endif
}

void AILLLobbyBeaconHostObject::HandleReservationRequest(AILLSessionBeaconClient* BeaconClient, const FILLSessionBeaconReservation& Reservation)
{
#if USE_GAMELIFT_SERVER_SDK
	// Do not allow people in until GameLift activates us
	UILLGameEngine* GameEngine = Cast<UILLGameEngine>(GEngine);
	if (IsRunningDedicatedServer() && GameEngine->GetGameLiftGameSessionId().IsEmpty())
	{
		HandleClientReservationFailed(BeaconClient, EILLSessionBeaconReservationResult::NoSession);
		return;
	}
#endif // USE_GAMELIFT_SERVER_SDK

	Super::HandleReservationRequest(BeaconClient, Reservation);
}

void AILLLobbyBeaconHostObject::HandleClientReservationPending(AILLSessionBeaconClient* BeaconClient, const FILLSessionBeaconReservation& Reservation)
{
	Super::HandleClientReservationPending(BeaconClient, Reservation);

	AILLLobbyBeaconClient* LobbyClient = CastChecked<AILLLobbyBeaconClient>(BeaconClient);

	// Begin authorization if we have not already
	if (!LobbyClient->HasAuthorized())
	{
		// NOTE: This will call our delegate immediately if it returns false
		if (!LobbyClient->BeginAuthorization(FOnILLLobbyBeaconAuthorizationComplete::CreateUObject(this, &ThisClass::OnClientAuthorizationComplete)))
			return;
	}

#if USE_GAMELIFT_SERVER_SDK
	// Simultaneously request a bypass if there is no PlayerSessionId
	if (IsRunningDedicatedServer() && LobbyClient->GetClientPlayerSessionId().IsEmpty())
	{
		// NOTE: This will call our delegate immediately if it returns false
		if (!LobbyClient->PerformMatchmakingBypass(CastChecked<AILLLobbyBeaconState>(SessionBeaconState), FOnILLLobbyBeaconMatchmakingBypassComplete::CreateUObject(this, &ThisClass::OnClientMatchmakingBypassComplete)))
			return;
	}
#endif // USE_GAMELIFT_SERVER_SDK
}

bool AILLLobbyBeaconHostObject::FailedPendingMember(AILLSessionBeaconMemberState* MemberState)
{
	AILLLobbyBeaconClient* LobbyClient = CastChecked<AILLLobbyBeaconClient>(MemberState->GetBeaconClientActor());

	// Double check matchmaking state, for an efficient skip
	if (LobbyClient->HasMatchmakingReservation() && ensure(SessionName == NAME_GameSession))
	{
		AILLGameMode* CurrentGameMode = GetGameMode();
		if (!IsValid(CurrentGameMode) || !CurrentGameMode->IsOpenToMatchmaking())
		{
			// Wait for dedicated suspend to end
			UILLGameEngine* GameEngine = Cast<UILLGameEngine>(GEngine);
			if (!GameEngine || GameEngine->bHasStarted)
			{
				HandleClientReservationFailed(LobbyClient, EILLSessionBeaconReservationResult::MatchmakingClosed);
				return true;
			}
		}
	}

	return false;
}

bool AILLLobbyBeaconHostObject::InitLobbyBeacon(int32 InMaxMembers, AILLPartyBeaconState* HostingPartyState, UILLLocalPlayer* FirstLocalPlayer)
{
	// Attempt to build the arbiter reservation from our party
	FILLSessionBeaconReservation ArbiterReservation;
	if (HostingPartyState)
	{
		check(!IsRunningDedicatedServer());
		check(FirstLocalPlayer);

		// Always add the local player
		ArbiterReservation.Players.Add(FILLSessionBeaconPlayer(FirstLocalPlayer));

		for (int32 MemberIndex = 0; MemberIndex < HostingPartyState->GetNumMembers(); ++MemberIndex)
		{
			AILLSessionBeaconMemberState* PartyMember = HostingPartyState->GetMemberAtIndex(MemberIndex);
			check(PartyMember);

			if (PartyMember->bArbitrationMember)
			{
				// This should be us! And we are already in the Players list
				check(PartyMember->GetAccountId() == FirstLocalPlayer->GetBackendPlayer()->GetAccountId());
				ArbiterReservation.LeaderAccountId = PartyMember->GetAccountId();
			}
			else
			{
				ArbiterReservation.Players.Add(FILLSessionBeaconPlayer(PartyMember->GetAccountId(), PartyMember->GetMemberUniqueId(), PartyMember->GetDisplayName()));
			}
		}
	}
	else if (!IsRunningDedicatedServer())
	{
		check(FirstLocalPlayer);

		ArbiterReservation.LeaderAccountId = FirstLocalPlayer->GetBackendPlayer()->GetAccountId();
		ArbiterReservation.Players.Add(FILLSessionBeaconPlayer(FirstLocalPlayer));
	}

	// Initialize the host beacon
	return InitHostBeacon(NAME_GameSession, InMaxMembers, ArbiterReservation);
}

bool AILLLobbyBeaconHostObject::AcceptPartyMember(const FILLBackendPlayerIdentifier& LeaderAccountId, const FILLBackendPlayerIdentifier& AccountId, FUniqueNetIdRepl UniqueId, const FString& DisplayName)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: LeaderAccountId: %s, AccountId: %s, DisplayName: %s"), ANSI_TO_TCHAR(__FUNCTION__), *LeaderAccountId.ToDebugString(), *AccountId.ToDebugString(), *DisplayName);

	if (LeaderAccountId.IsValid() && AccountId.IsValid() && !DisplayName.IsEmpty())
	{
		if (SessionBeaconState->GetNumMembers()+1 > MaxMembers)
			return false;

		if (AILLLobbyBeaconMemberState* LeaderMember = Cast<AILLLobbyBeaconMemberState>(SessionBeaconState->FindMember(LeaderAccountId)))
		{
			if (AILLLobbyBeaconMemberState* ExistingMember = Cast<AILLLobbyBeaconMemberState>(SessionBeaconState->FindMember(AccountId)))
			{
				// Update their leadership
				ExistingMember->SetLeaderAccountId(LeaderAccountId);
				return true;
			}
			else
			{
				// Spawn a member entry for them
				AILLSessionBeaconMemberState* NewMember = SessionBeaconState->SpawnMember(AccountId, UniqueId, DisplayName);
				NewMember->ReceiveLeadClient(LeaderAccountId);
				SessionBeaconState->AddMember(NewMember);
				return true;
			}
		}
	}

	return false;
}

void AILLLobbyBeaconHostObject::NotifyPartyMemberRemoved(const FILLBackendPlayerIdentifier& LeaderAccountId, const FILLBackendPlayerIdentifier& AccountId)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: Leader: %s, Member: %s"), ANSI_TO_TCHAR(__FUNCTION__), *LeaderAccountId.ToDebugString(), *AccountId.ToDebugString());

	if (AILLLobbyBeaconMemberState* ExistingMember = Cast<AILLLobbyBeaconMemberState>(SessionBeaconState->FindMember(AccountId)))
	{
		// Only clear their LeaderAccountId if it matches
		if (ExistingMember->GetLeaderAccountId() == LeaderAccountId)
		{
			ExistingMember->SetLeaderAccountId(FILLBackendPlayerIdentifier::GetInvalid());
		}
	}
}

void AILLLobbyBeaconHostObject::OnClientAuthorizationComplete(AILLLobbyBeaconClient* LobbyClient, EILLLobbyBeaconClientAuthorizationResult Result)
{
	if (Result != EILLLobbyBeaconClientAuthorizationResult::Success)
	{
		UE_LOG(LogBeacon, Warning, TEXT("%s: Authorization failed for %s!"), ANSI_TO_TCHAR(__FUNCTION__), *LobbyClient->GetName());
		if (Result == EILLLobbyBeaconClientAuthorizationResult::TimedOut)
			HandleClientReservationFailed(LobbyClient, EILLSessionBeaconReservationResult::AuthTimedOut);
		else
			HandleClientReservationFailed(LobbyClient, EILLSessionBeaconReservationResult::AuthFailed);
	}
}

#if USE_GAMELIFT_SERVER_SDK
void AILLLobbyBeaconHostObject::OnClientMatchmakingBypassComplete(AILLLobbyBeaconClient* LobbyClient, bool bWasSuccessful)
{
	if (!bWasSuccessful)
	{
		UE_LOG(LogBeacon, Warning, TEXT("%s: Matchmaking bypass authorization failed for %s!"), ANSI_TO_TCHAR(__FUNCTION__), *LobbyClient->GetName());
		HandleClientReservationFailed(LobbyClient, EILLSessionBeaconReservationResult::BypassFailed);
	}
}
#endif // USE_GAMELIFT_SERVER_SDK

void AILLLobbyBeaconHostObject::OnLobbySessionPSNCreated(const FString& SessionId)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *SessionId);

	// Hand the session over to all connected authorized clients
	// Clients that have not authorized will receive the session at the end of that sequence
	for (int32 ClientIndex = 0; ClientIndex < ClientActors.Num(); ++ClientIndex)
	{
		if (AILLLobbyBeaconClient* ClientActor = Cast<AILLLobbyBeaconClient>(ClientActors[ClientIndex]))
		{
			if (ClientActor->HasAuthorized() && ClientActor->GetAuthorizationType() == TEXT("psn"))
			{
				if (!SessionId.IsEmpty())
				{
					ClientActor->ClientLobbySessionReceived(SessionId, SessionId);
				}
				else
				{
					ClientActor->ClientLobbySessionGameSessionInvalid();
					DisconnectClient(ClientActor);
				}
			}
		}
	}
}

void AILLLobbyBeaconHostObject::OnLobbySessionXBLCreated(const FString& SessionId, const FString& SessionUri)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *SessionUri);

	// Hand the session over to all connected authorized clients
	// Clients that have not authorized will receive the session at the end of that sequence
	for (int32 ClientIndex = 0; ClientIndex < ClientActors.Num(); ++ClientIndex)
	{
		if (AILLLobbyBeaconClient* ClientActor = Cast<AILLLobbyBeaconClient>(ClientActors[ClientIndex]))
		{
			if (ClientActor->HasAuthorized() && ClientActor->GetAuthorizationType() == TEXT("xbl"))
			{
				if (!SessionId.IsEmpty() && !SessionUri.IsEmpty())
				{
					ClientActor->ClientLobbySessionReceived(SessionId, SessionUri);
				}
				else
				{
					ClientActor->ClientLobbySessionGameSessionInvalid();
					DisconnectClient(ClientActor);
				}
			}
		}
	}
}
