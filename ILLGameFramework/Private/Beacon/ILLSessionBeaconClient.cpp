// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLSessionBeaconClient.h"

#include "Online.h"
#include "OnlineSubsystemUtils.h"

#include "ILLSessionBeaconHostObject.h"
#include "ILLSessionBeaconState.h"

#define LOCTEXT_NAMESPACE "ILLFonic.IGF.ILLSessionBeaconClient"

FText EILLSessionBeaconReservationResult::GetLocalizedFailureText(const EILLSessionBeaconReservationResult::Type Response)
{
	switch (Response)
	{
	case EILLSessionBeaconReservationResult::Success: // Never displayed, but something here for debugging
		return FText::FromString(TEXT("Success"));

	case EILLSessionBeaconReservationResult::NotInitialized:
	case EILLSessionBeaconReservationResult::NoSession:
	case EILLSessionBeaconReservationResult::InvalidReservation:
	case EILLSessionBeaconReservationResult::Denied:
	case EILLSessionBeaconReservationResult::MatchmakingClosed: // Should not be displayed if matchmaking!
		return FText::Format(LOCTEXT("SessionReservation_Error", "Reservation error {0}!"), FText::FromString(EILLSessionBeaconReservationResult::ToString(Response)));

	case EILLSessionBeaconReservationResult::GameFull:
		return FText(LOCTEXT("SessionReservation_GameFull", "Game is full!"));

	case EILLSessionBeaconReservationResult::PartyFull:
		return FText(LOCTEXT("SessionReservation_PartyFull", "Party is full!"));

	case EILLSessionBeaconReservationResult::AuthFailed: // Should not be displayed if matchmaking?
		return FText(LOCTEXT("SessionReservation_AuthFailed", "Authorization failed!"));

	case EILLSessionBeaconReservationResult::AuthTimedOut: // Should not be displayed if matchmaking?
		return FText(LOCTEXT("SessionReservation_AuthTimedOut", "Authorization timed out!"));

	case EILLSessionBeaconReservationResult::BypassFailed: // Should not be displayed if matchmaking?
		return FText(LOCTEXT("SessionReservation_BypassFailed", "Bypass failed! Server is not accepting new player sessions."));

	default:
		check(0);
		break;
	}

	return FText::GetEmpty();
}

AILLSessionBeaconClient::AILLSessionBeaconClient(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bOnlyRelevantToOwner = true;

	LeaveServerTimeout = 3.f;
	TravelServerAckTimeout = 5.f;

	SteamP2PConnectionInitialTimeout = 45.f;
	BeaconConnectionTimeoutShipping = 60.f;
}

void AILLSessionBeaconClient::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AILLSessionBeaconClient, SessionBeaconState);
}

bool AILLSessionBeaconClient::UseShortConnectTimeout() const
{
	// NOTE: Beacons are typically configured with their InitialConnectTimeout lower, for matchmaking, and the ConnectionTimeout higher to handle loading times
	// Beacons are typically attempting to coordinate outside of level changes (except sometimes on the accepting end, obviously) so need to use a lower timeout for the initial connect, and a longer one after
	if (Super::UseShortConnectTimeout())
	{
		// Also wait for the reservation to be accepted, which happens after authorization
		return bReservationAccepted;
	}

	return false;
}

void AILLSessionBeaconClient::OnFailure()
{
	UE_LOG(LogBeacon, Warning, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// TEMP CALLSTACK! Print callstack so we can maybe see where the failure is coming from.
	{
		const SIZE_T StackTraceSize = 65535;
		ANSICHAR* StackTrace = (ANSICHAR*)FMemory::Malloc(StackTraceSize);
		StackTrace[0] = 0;

		// Walk the stack and dump it to the allocated memory.
		FPlatformStackWalk::StackWalkAndDump(StackTrace, StackTraceSize, 0);
		UE_LOG(LogOnlineGame, Log, TEXT("%s"), ANSI_TO_TCHAR(StackTrace));
		FMemory::Free(StackTrace);
	}

	CurrentConnectionString.Empty();
	SessionBeaconState = nullptr;

	Super::OnFailure();
}

bool AILLSessionBeaconClient::InitClientBeacon(const FString& ConnectionString, const FILLBackendPlayerIdentifier& AccountId, const FUniqueNetIdRepl& UniqueId)
{
	// Start connecting
	CurrentConnectionString = ConnectionString;
	FURL ConnectURL(NULL, *ConnectionString, TRAVEL_Absolute);
	GetWorld()->URL = ConnectURL;
	if (!InitClient(ConnectURL))
	{
		// InitClient will call OnFailure when it returns false
		UE_LOG(LogBeacon, Warning, TEXT("%s: Failure to init client beacon with %s."), ANSI_TO_TCHAR(__FUNCTION__), *ConnectionString);
		return false;
	}

#if PLATFORM_DESKTOP
	// SteamP2P gets a special timeout because it can take 10-15s just to establish one of those connections
	if (ConnectURL.Host.StartsWith(TEXT("steam.")))//STEAM_URL_PREFIX); // FIXME: pjackson: Would be nice if this were an OSS call or something...
	{
		NetDriver->InitialConnectTimeout = SteamP2PConnectionInitialTimeout;
	}
#endif
	NetDriver->ConnectionTimeout = UE_BUILD_SHIPPING ? BeaconConnectionTimeoutShipping : BeaconConnectionTimeout;

	// Update the connection PlayerId
	ClientAccountId = AccountId;
	ClientUniqueId = UniqueId;
	BeaconConnection->PlayerId = UniqueId;

	return true;
}

FNamedOnlineSession* AILLSessionBeaconClient::GetValidNamedSession(FName InSessionName) const
{
	IOnlineSessionPtr Sessions = Online::GetSessionInterface();
	FNamedOnlineSession* Result = Sessions.IsValid() ? Sessions->GetNamedSession(InSessionName) : nullptr;

	// Verify it's not a bunch of bullshit
	if (Result && (!Result->OwningUserId.IsValid() || !Result->SessionInfo.IsValid() || !Result->SessionInfo->IsValid() || Result->SessionState == EOnlineSessionState::Destroying))
		Result = nullptr;

	return Result;
}

bool AILLSessionBeaconClient::RequestReservation(const FOnlineSessionSearchResult& DesiredHost, const FILLSessionBeaconReservation& Reservation, const FILLBackendPlayerIdentifier& AccountId, const FUniqueNetIdRepl& UniqueId)
{
	if (!Reservation.IsValidReservation())
	{
		UE_LOG(LogBeacon, Warning, TEXT("%s: Invalid Reservation"), ANSI_TO_TCHAR(__FUNCTION__));
		OnFailure();
		return false;
	}
	if (!DesiredHost.IsValid())
	{
		UE_LOG(LogBeacon, Error, TEXT("%s: Invalid DesiredHost!"), ANSI_TO_TCHAR(__FUNCTION__));
		OnFailure();
		return false;
	}

	IOnlineSessionPtr SessionInt = Online::GetSessionInterface();
	if (AccountId.IsValid() && SessionInt.IsValid())
	{
		FString ConnectionString;
		if (!SessionInt->GetResolvedConnectString(DesiredHost, BeaconPort, ConnectionString))
		{
			UE_LOG(LogBeacon, Error, TEXT("%s: Unable to find beacon connection string!"), ANSI_TO_TCHAR(__FUNCTION__));
			OnFailure();
			return false;
		}

		UE_LOG(LogBeacon, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *ConnectionString);

		if (GetConnectionState() == EBeaconConnectionState::Open)
		{
			// Resume an existing client beacon
			if (CurrentConnectionString != ConnectionString)
			{
				UE_LOG(LogBeacon, Warning, TEXT("%s: CurrentConnectionString mismatch! %s != %s"), ANSI_TO_TCHAR(__FUNCTION__), *CurrentConnectionString, *ConnectionString);
				return false;
			}
		}
		else
		{
			// Spawn a new client beacon
			check(GetConnectionState() == EBeaconConnectionState::Invalid);
			if (!InitClientBeacon(ConnectionString, AccountId, UniqueId))
			{
				// InitClientBeacon will call OnFailure when it returns false
				return false;
			}
		}

		// Copy off our settings
		SessionOwnerUniqueId = DesiredHost.Session.OwningUserId;
		PendingReservation = Reservation;

		// OnConnected will take care of the rest
		return true;
	}

	OnFailure();
	return false;
}

void AILLSessionBeaconClient::HandleReservationFailed(const EILLSessionBeaconReservationResult::Type Response)
{
	UE_LOG(LogBeacon, Log, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *EILLSessionBeaconReservationResult::GetLocalizedFailureText(Response).ToString());

	ClientReservationFailed(Response);
	ForceNetUpdate();
}

void AILLSessionBeaconClient::HandleReservationPending()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	ClientReservationPending();
	ForceNetUpdate();
}

void AILLSessionBeaconClient::HandleReservationAccepted(AILLSessionBeaconState* InSessionBeaconState)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Store off the state to mark that we are connected and our reservation was accepted
	SessionBeaconState = InSessionBeaconState;
	bReservationAccepted = true;
	if (AILLSessionBeaconMemberState* ArbitrationMember = SessionBeaconState->GetArbitrationMember())
	{
		BeaconOwnerAccountId = ArbitrationMember->GetAccountId();
	}
	else
	{
		BeaconOwnerAccountId.Reset();
	}

	// Tell the client they are accepted
	ClientReservationAccepted(BeaconOwnerAccountId);
	ForceNetUpdate();
}

void AILLSessionBeaconClient::ClientReservationFailed_Implementation(const EILLSessionBeaconReservationResult::Type Response)
{
	UE_LOG(LogBeacon, Log, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *EILLSessionBeaconReservationResult::GetLocalizedFailureText(Response).ToString());

	OnBeaconReservationRequestFailed().ExecuteIfBound(Response);
}

void AILLSessionBeaconClient::ClientReservationPending_Implementation()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	OnBeaconReservationPending().ExecuteIfBound();
}

void AILLSessionBeaconClient::ClientReservationAccepted_Implementation(const FILLBackendPlayerIdentifier& InBeaconOwnerAccountId)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	BeaconOwnerAccountId = InBeaconOwnerAccountId;

	// Broadcast OnBeaconReservationAccepted if there is a SessionBeaconState, or wait for OnRep_SessionBeaconState
	if (SessionBeaconState)
	{
		CheckSessionBeaconStateHasMembers();
	}
}

bool AILLSessionBeaconClient::ServerRequestReservation_Validate(const FUniqueNetIdRepl& OwnerId, const FILLSessionBeaconReservation& Reservation, const FILLBackendPlayerIdentifier& AccountId, const FUniqueNetIdRepl& UniqueId)
{
	return true;
}

void AILLSessionBeaconClient::ServerRequestReservation_Implementation(const FUniqueNetIdRepl& OwnerId, const FILLSessionBeaconReservation& Reservation, const FILLBackendPlayerIdentifier& AccountId, const FUniqueNetIdRepl& UniqueId)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	if (AILLSessionBeaconHostObject* BeaconHost = Cast<AILLSessionBeaconHostObject>(GetBeaconOwner()))
	{
		// Copy off client settings
		ClientAccountId = AccountId;
		ClientUniqueId = UniqueId;
		SessionOwnerUniqueId = OwnerId;

		// Update the connection PlayerId
		BeaconConnection->PlayerId = UniqueId;

		// Now handle the reservation
		BeaconHost->HandleReservationRequest(this, Reservation);
	}
}

void AILLSessionBeaconClient::CheckSessionBeaconStateHasMembers()
{
	check(SessionBeaconState);
	if (SessionBeaconState->CheckHasReceivedMembers())
	{
		UE_LOG(LogBeacon, Verbose, TEXT("... members synced!"));
		SynchedBeaconMembers();
	}
	else
	{
		UE_LOG(LogBeacon, Verbose, TEXT("... waiting for members to sync"));
		StateMembersSentDelegateHandle = SessionBeaconState->OnBeaconMembersReceived().AddUObject(this, &ThisClass::OnStateMembersSentAcceptReservation);
	}
}

void AILLSessionBeaconClient::SynchedBeaconMembers()
{
	UE_LOG(LogBeacon, Log, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	bInitialMemberSyncComplete = true;

	// Accept our beacon membership
	check(SessionBeaconState);
	bReservationAccepted = true;
	OnBeaconReservationAccepted().ExecuteIfBound();

	// Tell the server/host
	ServerSynchronizedMembers();
}

void AILLSessionBeaconClient::OnRep_SessionBeaconState()
{
	CheckSessionBeaconStateHasMembers();
}

void AILLSessionBeaconClient::OnStateMembersSentAcceptReservation()
{
	UE_LOG(LogBeacon, Log, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	SessionBeaconState->OnBeaconMembersReceived().Remove(StateMembersSentDelegateHandle);

	SynchedBeaconMembers();
}

bool AILLSessionBeaconClient::ServerSynchronizedMembers_Validate()
{
	return true;
}

void AILLSessionBeaconClient::ServerSynchronizedMembers_Implementation()
{
	UE_LOG(LogBeacon, Log, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	bInitialMemberSyncComplete = true;
}

bool AILLSessionBeaconClient::StartLeaving()
{
	if (GetConnectionState() != EBeaconConnectionState::Open)
	{
		UE_LOG(LogBeacon, Warning, TEXT("%s: Connection not open!"), ANSI_TO_TCHAR(__FUNCTION__));
		return false;
	}

	if (TimerHandle_LeaveServerTimeout.IsValid() && GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_LeaveServerTimeout))
	{
		UE_LOG(LogBeacon, Warning, TEXT("%s: Already pending leave!"), ANSI_TO_TCHAR(__FUNCTION__));
		return false;
	}

	bWaitingLeaveAck = true;

	// Send the message
	ServerNotifyLeaving();

	// Set a timeout timer, we don't want this to take too long
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_LeaveServerTimeout, this, &ThisClass::ServerLeaveTimedOut, LeaveServerTimeout);
	return true;
}

void AILLSessionBeaconClient::ServerLeaveTimedOut()
{
	bWaitingLeaveAck = false;

	if (!bPendingLeave)
	{
		UE_LOG(LogBeacon, Log, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

		bPendingLeave = true;
		OnBeaconLeaveAcked().ExecuteIfBound();
	}
}

bool AILLSessionBeaconClient::ServerNotifyLeaving_Validate()
{
	return true;
}

void AILLSessionBeaconClient::ServerNotifyLeaving_Implementation()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Acknowledge it
	bPendingLeave = true;
	ClientAckLeaving();
	ForceNetUpdate();

	// Drop the connection immediately
	if (AILLSessionBeaconHostObject* BeaconHost = Cast<AILLSessionBeaconHostObject>(GetBeaconOwner()))
	{
		BeaconHost->DisconnectClient(this);
	}
}

void AILLSessionBeaconClient::ClientAckLeaving_Implementation()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	bWaitingLeaveAck = false;

	if (!bPendingLeave)
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_LeaveServerTimeout);
		TimerHandle_LeaveServerTimeout.Invalidate();

		bPendingLeave = true;
		OnBeaconLeaveAcked().ExecuteIfBound();
	}
}

void AILLSessionBeaconClient::ClientNotifyShutdown_Implementation()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	bPendingLeave = true;
	ServerAckShutdown();

	OnBeaconHostShutdown().ExecuteIfBound();
}

bool AILLSessionBeaconClient::ServerAckShutdown_Validate()
{
	return true;
}

void AILLSessionBeaconClient::ServerAckShutdown_Implementation()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	bPendingLeave = true;

	if (AILLSessionBeaconHostObject* BeaconHost = Cast<AILLSessionBeaconHostObject>(GetBeaconOwner()))
	{
		BeaconHost->ClientAckedShutdown(this);
	}
}

bool AILLSessionBeaconClient::StartTraveling()
{
	if (GetConnectionState() != EBeaconConnectionState::Open)
	{
		UE_LOG(LogBeacon, Warning, TEXT("%s: Connection not open!"), ANSI_TO_TCHAR(__FUNCTION__));
		return false;
	}

	if (GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_TravelServerTimeout))
	{
		UE_LOG(LogBeacon, Warning, TEXT("%s: Already pending travel!"), ANSI_TO_TCHAR(__FUNCTION__));
		return false;
	}

	// Send the message
	ServerNotifyTraveling();

	// Set a timeout timer, we don't want this to take too long
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_TravelServerTimeout, this, &ThisClass::ServerTravelAckTimedOut, TravelServerAckTimeout);
	return true;
}

void AILLSessionBeaconClient::ServerTravelAckTimedOut()
{
	UE_LOG(LogBeacon, Log, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	OnBeaconTravelAcked().ExecuteIfBound();
}

bool AILLSessionBeaconClient::ServerNotifyTraveling_Validate()
{
	return true;
}

void AILLSessionBeaconClient::ServerNotifyTraveling_Implementation()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	ClientAckTraveling();
	ForceNetUpdate();
}

void AILLSessionBeaconClient::ClientAckTraveling_Implementation()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Make sure we haven't already timed out
	if (GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_TravelServerTimeout))
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_TravelServerTimeout);
		OnBeaconTravelAcked().ExecuteIfBound();
	}
}

#undef LOCTEXT_NAMESPACE
