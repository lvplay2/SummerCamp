// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLSessionBeaconHostObject.h"

#include "OnlineSubsystem.h"
#include "OnlineSessionInterface.h"
#include "OnlineSubsystemUtils.h"

#include "ILLGameEngine.h"
#include "ILLGameMode.h"
#include "ILLSessionBeaconClient.h"
#include "ILLSessionBeaconState.h"

#define LOCTEXT_NAMESPACE "ILLFonic.IGF.ILLSessionBeaconHostObject"

AILLSessionBeaconHostObject::AILLSessionBeaconHostObject(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bAllowTickOnDedicatedServer = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	ClientBeaconActorClass = AILLSessionBeaconClient::StaticClass();
	BeaconTypeName = ClientBeaconActorClass->GetName();

	BeaconStateClass = AILLSessionBeaconState::StaticClass();

	MemberTickInterval = .1f;

	InitialConnectionTimeout = 20;

	ShutdownAckTimeout = 4.f;
}

void AILLSessionBeaconHostObject::Tick(float DeltaTime)
{
	if (!IsInitialized())
		return;

	// Tick member states
	TimeUntilMemberTick -= DeltaTime;
	if (TimeUntilMemberTick <= 0.f)
	{
		const float MemberDeltaTime = MemberTickInterval - TimeUntilMemberTick;
		TickMembers(MemberDeltaTime);
		TimeUntilMemberTick = MemberTickInterval;
	}
}

void AILLSessionBeaconHostObject::NotifyClientDisconnected(AOnlineBeaconClient* LeavingClientActor)
{
	AILLSessionBeaconClient* SBC = Cast<AILLSessionBeaconClient>(LeavingClientActor);
	if (AILLSessionBeaconMemberState* MemberState = SessionBeaconState->FindMember(SBC))
	{
		if (SBC->IsPendingLeave())
		{
			UE_LOG(LogBeacon, Verbose, TEXT("%s: Client pending leave disconnected, cleaning up: %s"), ANSI_TO_TCHAR(__FUNCTION__), *MemberState->ToDebugString());
		}
		else
		{
			UE_LOG(LogBeacon, Log, TEXT("%s: Lost connection for: %s!"), ANSI_TO_TCHAR(__FUNCTION__), *MemberState->ToDebugString());
		}

		SessionBeaconState->RemoveMember(SBC);
	}
	else
	{
		UE_LOG(LogBeacon, Verbose, TEXT("%s: No member state found to cleanup for client: %s"), ANSI_TO_TCHAR(__FUNCTION__), *SBC->GetName());
	}

	// Now destroy it...
	Super::NotifyClientDisconnected(LeavingClientActor);

	SessionBeaconState->DumpState();
}

FNamedOnlineSession* AILLSessionBeaconHostObject::GetValidNamedSession(FName InSessionName) const
{
	IOnlineSessionPtr Sessions = Online::GetSessionInterface();
	FNamedOnlineSession* Result = Sessions.IsValid() ? Sessions->GetNamedSession(InSessionName) : nullptr;

	// Verify it's not a bunch of bullshit
	if (Result && (!Result->OwningUserId.IsValid() || !Result->SessionInfo.IsValid() || !Result->SessionInfo->IsValid() || Result->SessionState == EOnlineSessionState::Destroying))
		Result = nullptr;

	return Result;
}

void AILLSessionBeaconHostObject::TickMembers(float DeltaTime)
{
	IOnlineSessionPtr Sessions = Online::GetSessionInterface();
	FNamedOnlineSession* Session = GetValidNamedSession(SessionName);
	AILLGameMode* CurrentGameMode = GetGameMode();

	// Check if we have loaded in
	bool bLocallyLoadedIn = true;
	if (SessionName == NAME_GameSession)
	{
		bLocallyLoadedIn = false;

		// TODO: pjackson: Make this a virtual function on the game mode instead?
		// Verify we have a game mode and are setup to be a server
		if (IsValid(CurrentGameMode) && (CurrentGameMode->GetNetMode() == NM_DedicatedServer || CurrentGameMode->GetNetMode() == NM_ListenServer))
		{
			// Verify we are in a match state that allows this
			const FName MatchState = CurrentGameMode->GetMatchState();
			if (MatchState != MatchState::EnteringMap && MatchState != MatchState::LeavingMap && MatchState != MatchState::Aborted)
				bLocallyLoadedIn = true;
		}
	}

	// Check if everybody is loaded in
	bool bEverybodyLoadedIn = true;
	for (int32 ClientIndex = 0; ClientIndex < ClientActors.Num(); ++ClientIndex)
	{
		if (AILLSessionBeaconClient* ClientActor = Cast<AILLSessionBeaconClient>(ClientActors[ClientIndex]))
		{
			if (!ClientActor->UseShortConnectTimeout())
			{
				bEverybodyLoadedIn = false;
				break;
			}
		}
	}
	if (bEverybodyLoadedIn)
	{
		bEverybodyLoadedIn = false;
		if (SessionName == NAME_GameSession)
		{
			// Only perform game session registration timeouts when we are not waiting for someone to load
			// There is no way to see who is loading right now (game layer is not made aware of timeouts as far as I can tell)
			if (IsValid(CurrentGameMode) && bLocallyLoadedIn && CurrentGameMode->HasEveryPlayerLoadedIn(false))
			{
				bEverybodyLoadedIn = true;
			}
		}
		else if (SessionName == NAME_PartySession)
		{
			bEverybodyLoadedIn = true;
		}
	}

	for (int32 MemberIndex = 0; MemberIndex < SessionBeaconState->GetNumMembers(); )
	{
		AILLSessionBeaconMemberState* MemberState = SessionBeaconState->GetMemberAtIndex(MemberIndex);
		if (MemberState->bArbitrationMember)
		{
			// Do not timeout the beacon arbiter
			++MemberIndex;
			continue;
		}

		// Check for a connection
		const EBeaconConnectionState ConnectionState = MemberState->GetBeaconClientActor() ? MemberState->GetBeaconClientActor()->GetConnectionState() : EBeaconConnectionState::Invalid;
		if (ConnectionState == EBeaconConnectionState::Invalid || ConnectionState == EBeaconConnectionState::Closed)
		{
			// Bump our timer and check our timeout
			MemberState->TimeoutDuration += DeltaTime;
			if (int32(MemberState->TimeoutDuration) >= InitialConnectionTimeout)
			{
				UE_LOG(LogBeacon, Verbose, TEXT("%s: Connection timeout for member %s after %d seconds"), ANSI_TO_TCHAR(__FUNCTION__), *MemberState->ToDebugString(), FMath::FloorToInt(MemberState->TimeoutDuration));

				// Timeout!
				SessionBeaconState->RemoveMemberAtIndex(MemberIndex);
			}
			else
			{
				++MemberIndex;
			}
			continue;
		}

		if (MemberState->HasReservationBeenAccepted())
		{
			if (!bEverybodyLoadedIn)
			{
				// Reset timeouts while people are loading
				MemberState->TimeoutDuration = 0.f;
			}
		}
		else
		{
			// Wait for authorization
			if (MemberState->GetBeaconClientActor()->IsPendingAuthorization())
			{
				++MemberIndex;
				continue;
			}

			// Determine if we are accepting reservations right now
			bool bAcceptingReservations = false;
			if (Session && Session->SessionState == EOnlineSessionState::InProgress)
			{
				if (SessionName == NAME_GameSession)
				{
					if (bLocallyLoadedIn)
					{
						bAcceptingReservations = true;
					}
					else if (UILLGameEngine* GameEngine = Cast<UILLGameEngine>(GEngine))
					{
						// Handle suspended dedicated servers
						bAcceptingReservations = !GameEngine->bHasStarted;
					}
				}
				else if (SessionName == NAME_PartySession)
				{
					bAcceptingReservations = true;
				}
			}

			if (bAcceptingReservations)
			{
				// Check if we want to fail this pending reservation
				if (FailedPendingMember(MemberState))
				{
					++MemberIndex;
					continue;
				}

				// Do not time them out while waiting on CanFinishAccepting
				MemberState->TimeoutDuration = 0.f;

				if (CanFinishAccepting(MemberState))
				{
					// Accept the member
					HandleClientReservationAccepted(MemberState);
				}
			}
		}

		++MemberIndex;
	}
}

bool AILLSessionBeaconHostObject::InitHostBeacon(FName InSessionName, int32 InMaxMembers, const FILLSessionBeaconReservation& ArbiterReservation)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: %s MaxMembers:%d"), ANSI_TO_TCHAR(__FUNCTION__), *InSessionName.ToString(), InMaxMembers);
	check(InMaxMembers > 0);

	SessionName = InSessionName;
	MaxMembers = InMaxMembers;

	// Spawn the session beacon state
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Owner = this;
	SessionBeaconState = GetWorld()->SpawnActor<AILLSessionBeaconState>(BeaconStateClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);
	if (!SessionBeaconState)
	{
		UE_LOG(LogBeacon, Error, TEXT("%s: Unable to spawn beacon state!"), ANSI_TO_TCHAR(__FUNCTION__));
		return false;
	}

	// Associate with this objects net driver for proper replication
	SessionBeaconState->SetNetDriverName(GetNetDriverName());

	if (ArbiterReservation.IsValidReservation())
	{
		// Convert ArbiterReservation to member states
		AILLSessionBeaconMemberState* LeaderMember = nullptr;
		TArray<AILLSessionBeaconMemberState*> NewMembers;
		for (const FILLSessionBeaconPlayer& Player : ArbiterReservation.Players)
		{
			AILLSessionBeaconMemberState* MemberState = SessionBeaconState->SpawnMember(Player.AccountId, Player.UniqueId, Player.DisplayName);

			if (Player.AccountId == ArbiterReservation.LeaderAccountId)
			{
				LeaderMember = MemberState;
				MemberState->InitializeArbiterLeader();
			}
			else
			{
				MemberState->InitializeArbiterFollower(ArbiterReservation.LeaderAccountId);
			}

			NewMembers.Add(MemberState);
		}
		if (!LeaderMember)
		{
			UE_LOG(LogBeacon, Error, TEXT("%s: Failed to find leader for reservation %s!"), ANSI_TO_TCHAR(__FUNCTION__), *ArbiterReservation.ToDebugString());
			return false;
		}

		// Now add the new pending members to the state
		for (AILLSessionBeaconMemberState* Member : NewMembers)
		{
			SessionBeaconState->AddMember(Member);
		}
	}

	SessionBeaconState->DumpState();
	return true;
}

void AILLSessionBeaconHostObject::HandleReservationRequest(AILLSessionBeaconClient* BeaconClient, const FILLSessionBeaconReservation& Reservation)
{
	check(BeaconClient);

	UE_LOG(LogBeacon, Verbose, TEXT("%s: Client: %s Reservation: %s"), ANSI_TO_TCHAR(__FUNCTION__), *BeaconClient->GetName(), *Reservation.ToDebugString());

	EILLSessionBeaconReservationResult::Type Result = EILLSessionBeaconReservationResult::Success;
	if (GetBeaconState() == EBeaconState::DenyRequests)
	{
		Result = EILLSessionBeaconReservationResult::Denied;
	}
	else if (!ValidateReservation(BeaconClient, Reservation))
	{
		Result = EILLSessionBeaconReservationResult::InvalidReservation;
	}
	else if (!IsInitialized())
	{
		Result = EILLSessionBeaconReservationResult::NotInitialized;
	}
	else
	{
		// Check if we already have any of these players
		int32 NumMembersFound = 0;
		int32 NumMembersNotFound = 0;
		for (const FILLSessionBeaconPlayer& Player : Reservation.Players)
		{
			if (AILLSessionBeaconMemberState* MemberState = SessionBeaconState->FindMember(Player.AccountId))
			{
				++NumMembersFound;
			}
			else
			{
				++NumMembersNotFound;
			}
		}

		// Check for enough room
		// We check the raw number of members to include pending members!
		if (SessionBeaconState->GetNumMembers()+NumMembersNotFound > MaxMembers)
		{
			if (SessionName == NAME_PartySession)
				Result = EILLSessionBeaconReservationResult::PartyFull;
			else
				Result = EILLSessionBeaconReservationResult::GameFull;
		}
	}

	if (Result == EILLSessionBeaconReservationResult::Success)
	{
		FNamedOnlineSession* Session = GetValidNamedSession(SessionName);

		// Verify the session state
		if (!Session || Session->SessionState != EOnlineSessionState::InProgress)
		{
			Result = EILLSessionBeaconReservationResult::NoSession;
		}
	}

	if (Result == EILLSessionBeaconReservationResult::Success)
	{
		HandleClientReservationPending(BeaconClient, Reservation);
	}
	else
	{
		HandleClientReservationFailed(BeaconClient, Result);
	}
}

bool AILLSessionBeaconHostObject::ValidateReservation(AILLSessionBeaconClient* BeaconClient, const FILLSessionBeaconReservation& Reservation) const
{
	return (BeaconClient && BeaconClient->GetClientAccountId().IsValid() && Reservation.IsValidReservation());
}

void AILLSessionBeaconHostObject::HandleClientReservationFailed(AILLSessionBeaconClient* BeaconClient, const EILLSessionBeaconReservationResult::Type Response)
{
	UE_LOG(LogBeacon, Warning, TEXT("%s: Client: %s Failed! Result: %s"), ANSI_TO_TCHAR(__FUNCTION__), *BeaconClient->GetName(), EILLSessionBeaconReservationResult::ToString(Response));

	// Notify them of the failure then disconnect
	BeaconClient->HandleReservationFailed(Response);
	DisconnectClient(BeaconClient);
}

void AILLSessionBeaconHostObject::HandleClientReservationPending(AILLSessionBeaconClient* BeaconClient, const FILLSessionBeaconReservation& Reservation)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: Client: %s Reservation: %s"), ANSI_TO_TCHAR(__FUNCTION__), *BeaconClient->GetName(), *Reservation.ToDebugString());

	// Handle existing members correctly, while allowing new ones as well
	TArray<AILLSessionBeaconMemberState*> NewMembers;
	for (const FILLSessionBeaconPlayer& Player : Reservation.Players)
	{
		AILLSessionBeaconMemberState* MemberState = SessionBeaconState->FindMember(Player.AccountId);

		if (Player.AccountId == BeaconClient->GetClientAccountId())
		{
			// Handle receiving a new BeaconClient for an already accepted member by kicking the old member
			if (MemberState && MemberState->GetBeaconClientActor() && MemberState->GetBeaconClientActor() != BeaconClient)
			{
				AILLSessionBeaconClient* OldMemberClient = MemberState->GetBeaconClientActor();

				UE_LOG(LogBeacon, Verbose, TEXT("... Connected to previously pending or accepted member %s, kicking old duplicate member %s and removing %s"), *Player.AccountId.ToDebugString(), *GetNameSafe(OldMemberClient), *MemberState->ToDebugString());

				OldMemberClient->WasKicked(LOCTEXT("BeaconKick_DuplicateReservation", "Duplicate reservation request received!"));
				SessionBeaconState->RemoveMember(OldMemberClient);
				DisconnectClient(OldMemberClient);
				MemberState = nullptr;
			}
		}

		if (MemberState)
		{
			if (Player.AccountId == BeaconClient->GetClientAccountId())
			{
				UE_LOG(LogBeacon, Verbose, TEXT("... Connected to previously-reserved member %s"), *Player.AccountId.ToDebugString());

				// Direct connection established for an existing member
				MemberState->ReceiveClient(BeaconClient);
			}
			else
			{
				// We already knew about this person, probably because of a previous party reservation
			}
		}
		else
		{
			// Spawn a new member entry
			MemberState = SessionBeaconState->SpawnMember(Player.AccountId, Player.UniqueId, Player.DisplayName);

			// Check if it matches the connected BeaconClient
			if (Player.AccountId == BeaconClient->GetClientAccountId())
			{
				UE_LOG(LogBeacon, Verbose, TEXT("... Connected to new member %s"), *Player.AccountId.ToDebugString());

				if (Reservation.LeaderAccountId.IsValid())
				{
					// This is the leader, so we should be talking with them
					MemberState->ReceiveLeadClient(Reservation.LeaderAccountId, BeaconClient);
				}
				else
				{
					// This is a leader-less member
					MemberState->ReceiveClient(BeaconClient);
				}
			}
			else if (Reservation.LeaderAccountId.IsValid())
			{
				// This is an accompanying member, no client yet
				MemberState->ReceiveLeadClient(Reservation.LeaderAccountId);
			}
			else
			{
				// Leader-less and client-less?
				check(0);
			}

			NewMembers.Add(MemberState);
		}
	}

	// Now add the new pending members to the state
	for (AILLSessionBeaconMemberState* Member : NewMembers)
	{
		SessionBeaconState->AddMember(Member);
	}

	BeaconClient->HandleReservationPending();

	SessionBeaconState->DumpState();
}

void AILLSessionBeaconHostObject::HandleClientReservationAccepted(AILLSessionBeaconMemberState* MemberState)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *MemberState->ToDebugString());

	MemberState->NotifyAccepted();
	MemberState->GetBeaconClientActor()->HandleReservationAccepted(SessionBeaconState);

	SessionBeaconState->DumpState();
}

void AILLSessionBeaconHostObject::StartShutdown()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	if (ClientActors.Num() > 0)
	{
		// Notify all clients to anticipate a shutdown
		for (int32 ClientIndex = 0; ClientIndex < ClientActors.Num(); ++ClientIndex)
		{
			if (AILLSessionBeaconClient* ClientActor = Cast<AILLSessionBeaconClient>(ClientActors[ClientIndex]))
			{
				ClientActor->ClientNotifyShutdown();
			}
		}

		GetWorld()->GetTimerManager().SetTimer(TimerHandle_ShutdownAckTimeout, this, &ThisClass::ServerShutdownAckTimeout, ShutdownAckTimeout);
	}
	else
	{
		// No members, skip to acknowledgment
		CheckShutdownAcked();
	}
}

void AILLSessionBeaconHostObject::ClientAckedShutdown(AILLSessionBeaconClient* ClientActor)
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s: Client: %s"), ANSI_TO_TCHAR(__FUNCTION__), *ClientActor->GetName());

	// Disconnect them immediately
	DisconnectClient(ClientActor);

	// Check that everybody is gone
	CheckShutdownAcked();
}

void AILLSessionBeaconHostObject::CheckShutdownAcked()
{
	if (ClientActors.Num() == 0)
	{
		UWorld* World = GetWorld();
		World->GetTimerManager().ClearTimer(TimerHandle_ShutdownAckTimeout);

		// Delay the delegate trigger slightly so we're not processing the packet still and cause a crash... sigh.
		FTimerDelegate Delegate;
		Delegate.BindLambda([this]() { BeaconShutdownAcked.ExecuteIfBound(); });
		FTimerHandle ThrowAwayHandle;
		World->GetTimerManager().SetTimer(ThrowAwayHandle, Delegate, 0.5f, false);
	}
}

void AILLSessionBeaconHostObject::ServerShutdownAckTimeout()
{
	UE_LOG(LogBeacon, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	BeaconShutdownAcked.ExecuteIfBound();
}

AILLGameMode* AILLSessionBeaconHostObject::GetGameMode() const
{
	if (UWorld* World = GetGameInstance()->GetWorld())
	{
		return World->GetAuthGameMode<AILLGameMode>();
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE
