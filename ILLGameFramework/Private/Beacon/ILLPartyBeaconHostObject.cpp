// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLPartyBeaconHostObject.h"

#include "OnlineSubsystem.h"
#include "OnlineSessionInterface.h"
#include "OnlineSubsystemUtils.h"

#include "ILLBackendPlayer.h"
#include "ILLLobbyBeaconClient.h"
#include "ILLLobbyBeaconHostObject.h"
#include "ILLLocalPlayer.h"
#include "ILLOnlineSessionClient.h"
#include "ILLPartyBeaconClient.h"
#include "ILLPartyBeaconHost.h"
#include "ILLPartyBeaconMemberState.h"
#include "ILLPartyBeaconState.h"

AILLPartyBeaconHostObject::AILLPartyBeaconHostObject(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	ClientBeaconActorClass = AILLPartyBeaconClient::StaticClass();
	BeaconTypeName = ClientBeaconActorClass->GetName();

	BeaconStateClass = AILLPartyBeaconState::StaticClass();

	PartyHostTravelCoordinationTimeout = 30.f;
}

void AILLPartyBeaconHostObject::NotifyClientDisconnected(AOnlineBeaconClient* LeavingClientActor)
{
	if (AILLPartyBeaconClient* PartyClient = Cast<AILLPartyBeaconClient>(LeavingClientActor))
	{
		// Notify our "parent" lobby beacon of the membership removal
		UILLOnlineSessionClient* OSC = CastChecked<UILLOnlineSessionClient>(GetGameInstance()->GetOnlineSession());
		if (AILLLobbyBeaconClient* LobbyClient = OSC->GetLobbyClientBeacon())
		{
			LobbyClient->ServerNotifyPartyMemberRemoved(PartyClient->GetClientAccountId());
		}
		else if (AILLLobbyBeaconHostObject* LobbyHost = OSC->GetLobbyHostBeaconObject())
		{
			LobbyHost->NotifyPartyMemberRemoved(LocalPartyLeader->GetBackendPlayer()->GetAccountId(), PartyClient->GetClientAccountId());
		}
	}

	Super::NotifyClientDisconnected(LeavingClientActor);
}

void AILLPartyBeaconHostObject::HandleClientReservationPending(AILLSessionBeaconClient* BeaconClient, const FILLSessionBeaconReservation& Reservation)
{
	Super::HandleClientReservationPending(BeaconClient, Reservation);

	// Make a request to our "parent" lobby beacon
	if (AILLPartyBeaconMemberState* NewPartyMember = Cast<AILLPartyBeaconMemberState>(SessionBeaconState->FindMember(BeaconClient->GetClientUniqueId())))
	{
		UILLOnlineSessionClient* OSC = CastChecked<UILLOnlineSessionClient>(GetGameInstance()->GetOnlineSession());
		if (AILLLobbyBeaconClient* LobbyClient = OSC->GetLobbyClientBeacon())
		{
			// Request a reservation for NewPartyMember
			LobbyClient->RequestPartyAddition(NewPartyMember);
			NewPartyMember->bLobbyReservationPending = true; // TODO: pjackson: Listen for the lobby client going away and just allow FinishAccepting to return true
		}
		else if (AILLLobbyBeaconHostObject* LobbyHost = OSC->GetLobbyHostBeaconObject())
		{
			// Verify acceptance against the local lobby host object
			const FILLBackendPlayerIdentifier& LeaderAccountId = LocalPartyLeader->GetBackendPlayer()->GetAccountId();
			if (!LobbyHost->AcceptPartyMember(LeaderAccountId, NewPartyMember->GetAccountId(), NewPartyMember->GetMemberUniqueId(), NewPartyMember->GetDisplayName()))
			{
				// Kick them because we are full
				HandleClientReservationFailed(BeaconClient, EILLSessionBeaconReservationResult::GameFull);
			}
		}
	}
}

bool AILLPartyBeaconHostObject::CanFinishAccepting(AILLSessionBeaconMemberState* MemberState) const
{
	if (!Super::CanFinishAccepting(MemberState))
		return false;

	if (AILLPartyBeaconMemberState* NewPartyMember = Cast<AILLPartyBeaconMemberState>(MemberState))
	{
		return !NewPartyMember->bLobbyReservationPending;
	}

	return false;
}

bool AILLPartyBeaconHostObject::InitPartyBeacon(int32 InMaxMembers, UILLLocalPlayer* FirstLocalPlayer)
{
	if (!ensure(FirstLocalPlayer))
		return false;

	LocalPartyLeader = FirstLocalPlayer;

	// Send a reservation with us as the LeaderId
	FILLSessionBeaconReservation ArbiterReservation;
	ArbiterReservation.LeaderAccountId = FirstLocalPlayer->GetBackendPlayer()->GetAccountId();
	ArbiterReservation.Players.Add(FILLSessionBeaconPlayer(FirstLocalPlayer));

	return InitHostBeacon(NAME_PartySession, InMaxMembers, ArbiterReservation);
}

void AILLPartyBeaconHostObject::NotifyPartySessionStarted()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Register existing members with the party session
	// CheckHasSynchronized should call to RegisterWithPartySession
	for (int32 MemberIndex = 0; MemberIndex < SessionBeaconState->GetNumMembers(); ++MemberIndex)
	{
		if (AILLPartyBeaconMemberState* MemberState = Cast<AILLPartyBeaconMemberState>(SessionBeaconState->GetMemberAtIndex(MemberIndex)))
		{
			MemberState->CheckHasSynchronized();
		}
	}

	// Enable requests on the host listener
	AILLPartyBeaconHost* BeaconHost = Cast<AILLPartyBeaconHost>(GetOwner());
	BeaconHost->PauseBeaconRequests(false);
}

void AILLPartyBeaconHostObject::KickPlayer(AILLPartyBeaconClient* ClientActor, const FText& KickReason)
{
	UE_LOG(LogBeacon, Log, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *GetNameSafe(ClientActor));

	ClientActor->WasKicked(KickReason);
	DisconnectClient(ClientActor);
}

void AILLPartyBeaconHostObject::HandlePartyAdditionAccepted(const FILLBackendPlayerIdentifier& MemberAccountId)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *MemberAccountId.ToDebugString());

	if (AILLPartyBeaconMemberState* MemberState = Cast<AILLPartyBeaconMemberState>(SessionBeaconState->FindMember(MemberAccountId)))
	{
		MemberState->bLobbyReservationPending = false;
	}
}

void AILLPartyBeaconHostObject::HandlePartyAdditionDenied(const FILLBackendPlayerIdentifier& MemberAccountId)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *MemberAccountId.ToDebugString());

	if (AILLPartyBeaconMemberState* NewPartyMember = Cast<AILLPartyBeaconMemberState>(SessionBeaconState->FindMember(MemberAccountId)))
	{
		if (AILLPartyBeaconClient* NewPartyClient = Cast<AILLPartyBeaconClient>(NewPartyMember->GetBeaconClientActor()))
		{
			if (NewPartyMember->bLobbyReservationPending)
			{
				// Kick them because we are full
				HandleClientReservationFailed(NewPartyClient, EILLSessionBeaconReservationResult::GameFull);
			}
		}
	}
}

void AILLPartyBeaconHostObject::NotifyPartyMatchmakingStarted()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	for (int32 ClientIndex = 0; ClientIndex < ClientActors.Num(); ++ClientIndex)
	{
		if (AILLPartyBeaconClient* ClientActor = Cast<AILLPartyBeaconClient>(ClientActors[ClientIndex]))
		{
			ClientActor->ClientNotifyMatchmakingStart();
		}
	}
}

void AILLPartyBeaconHostObject::NotifyPartyMatchmakingState(const EILLMatchmakingState NewState)
{
	for (int32 ClientIndex = 0; ClientIndex < ClientActors.Num(); ++ClientIndex)
	{
		if (AILLPartyBeaconClient* ClientActor = Cast<AILLPartyBeaconClient>(ClientActors[ClientIndex]))
		{
			ClientActor->ClientMatchmakingStateUpdate(static_cast<uint8>(NewState));
		}
	}
}

void AILLPartyBeaconHostObject::NotifyPartyMatchmakingStopped()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	for (int32 ClientIndex = 0; ClientIndex < ClientActors.Num(); ++ClientIndex)
	{
		if (AILLPartyBeaconClient* ClientActor = Cast<AILLPartyBeaconClient>(ClientActors[ClientIndex]))
		{
			ClientActor->ClientNotifyMatchmakingStop();
		}
	}
}

bool AILLPartyBeaconHostObject::NotifyTravelToGameSession()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// We have just finished loading a map
	// Tell all of our party members to follow us to our game session
	if (FNamedOnlineSession* Session = GetValidNamedSession(NAME_GameSession))
	{
		for (int32 ClientIndex = 0; ClientIndex < ClientActors.Num(); ++ClientIndex)
		{
			if (AILLPartyBeaconClient* ClientActor = Cast<AILLPartyBeaconClient>(ClientActors[ClientIndex]))
			{
				ClientActor->JoinLeadersGameSession(Session);
			}
		}

		return true;
	}

	return false;
}

void AILLPartyBeaconHostObject::NotifyTravelToMainMenu()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	for (int32 ClientIndex = 0; ClientIndex < ClientActors.Num(); ++ClientIndex)
	{
		if (AILLPartyBeaconClient* ClientActor = Cast<AILLPartyBeaconClient>(ClientActors[ClientIndex]))
		{
			ClientActor->JoinLeadersGameSession(nullptr);
		}
	}
}

bool AILLPartyBeaconHostObject::NotifyLobbyReservationAccepted()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Grab our game session handle so that we can pass some information about it to our party members
	FNamedOnlineSession* Session = GetValidNamedSession(NAME_GameSession);
	if (!Session)
	{
		UE_LOG(LogBeacon, Warning, TEXT("%s: Could not find game session!"), ANSI_TO_TCHAR(__FUNCTION__));
		return false;
	}

	if (ClientActors.Num() > 0)
	{
		// Tell all clients to join the session and report back
		for (int32 ClientIndex = 0; ClientIndex < ClientActors.Num(); ++ClientIndex)
		{
			if (AILLPartyBeaconClient* ClientActor = Cast<AILLPartyBeaconClient>(ClientActors[ClientIndex]))
			{
				ClientsPendingTravelCoordination.AddUnique(ClientActor);
				ClientActor->JoinLeadersGameSession(Session);
			}
		}

		// Set a timeout for coordination completion
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_TravelCoordinationTimout, this, &ThisClass::TravelCoordinationTimout, PartyHostTravelCoordinationTimeout);
	}
	else
	{
		// No members to notify, skip to completion
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::TravelCoordinationComplete);
	}

	return true;
}

void AILLPartyBeaconHostObject::HandleMemberAckJoinGameSession(AILLPartyBeaconClient* PartyClient)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *PartyClient->GetName());

	// Check for coordination completion
	RemoveAndCheckCoordinationCompletion(PartyClient);
}

void AILLPartyBeaconHostObject::HandleMemberJoinedGameSession(AILLPartyBeaconClient* PartyClient)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *PartyClient->GetName());

	// Check for coordination completion
	RemoveAndCheckCoordinationCompletion(PartyClient);
}

void AILLPartyBeaconHostObject::HandleMemberFailedToFindGameSession(AILLPartyBeaconClient* PartyClient)
{
	UE_LOG(LogBeacon, Warning, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *PartyClient->GetName());

	// Tell PartyClient to leave
	PartyClient->ClientCancelGameSessionTravel();
	DisconnectClient(PartyClient);

	// Check for coordination completion
	RemoveAndCheckCoordinationCompletion(PartyClient);
}

void AILLPartyBeaconHostObject::HandleMemberFailedToJoinGameSession(AILLPartyBeaconClient* PartyClient, EOnJoinSessionCompleteResult::Type Result)
{
	UE_LOG(LogBeacon, Warning, TEXT("%s: %s Result: %s"), ANSI_TO_TCHAR(__FUNCTION__), *PartyClient->GetName(), *GetLocalizedDescription(NAME_PartySession, Result).ToString());

	// Tell PartyClient to leave
	PartyClient->ClientCancelGameSessionTravel();
	DisconnectClient(PartyClient);

	// Check for coordination completion
	RemoveAndCheckCoordinationCompletion(PartyClient);
}

void AILLPartyBeaconHostObject::RemoveAndCheckCoordinationCompletion(AILLPartyBeaconClient* PartyClient)
{
	if (TimerHandle_TravelCoordinationTimout.IsValid())
	{
		check(ClientsPendingTravelCoordination.Num() > 0);
		ClientsPendingTravelCoordination.Remove(PartyClient);
		if (ClientsPendingTravelCoordination.Num() == 0)
		{
			TravelCoordinationComplete();
		}
	}
}

void AILLPartyBeaconHostObject::TravelCoordinationComplete()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Cleanup
	if (TimerHandle_TravelCoordinationTimout.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_TravelCoordinationTimout);
		TimerHandle_TravelCoordinationTimout.Invalidate();
	}
	ClientsPendingTravelCoordination.Empty();

	// Notify outwards
	PartyTravelCoordinationComplete.ExecuteIfBound();
}

void AILLPartyBeaconHostObject::TravelCoordinationTimout()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	TravelCoordinationComplete();
}
