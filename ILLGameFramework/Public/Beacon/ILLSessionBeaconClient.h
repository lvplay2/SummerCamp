// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLOnlineBeaconClient.h"
#include "ILLSessionBeaconMemberState.h"
#include "ILLSessionBeaconClient.generated.h"

class FNamedOnlineSession;

class AILLSessionBeaconState;

/**
 * @enum EILLSessionBeaconReservationResult
 */
UENUM()
namespace EILLSessionBeaconReservationResult
{
	enum Type
	{
		Success,
		NotInitialized, // Has not had a state initialized yet
		NoSession, // Has been initialized but has not created an associated session yet
		InvalidReservation, // Reservation data is not correct
		Denied, // Reservations not being allowed at this time
		GameFull, // Game beacon is full
		PartyFull, // Party beacon is full
		AuthFailed, // Authorization failed
		AuthTimedOut, // Authorization timed out
		BypassFailed, // Matchmaking service bypass failure
		MatchmakingClosed, // Matchmaking special case: Checks if IsOpenToMatchmaking is still true
	};
}

namespace EILLSessionBeaconReservationResult
{
	extern ILLGAMEFRAMEWORK_API FText GetLocalizedFailureText(const EILLSessionBeaconReservationResult::Type Response);

	/** @return String name for Response. */
	FORCEINLINE const TCHAR* ToString(const EILLSessionBeaconReservationResult::Type Response)
	{
		switch (Response)
		{
		case EILLSessionBeaconReservationResult::Success: return TEXT("Success");
		case EILLSessionBeaconReservationResult::NotInitialized: return TEXT("NotInitialized");
		case EILLSessionBeaconReservationResult::NoSession: return TEXT("NoSession");
		case EILLSessionBeaconReservationResult::InvalidReservation: return TEXT("InvalidReservation");
		case EILLSessionBeaconReservationResult::Denied: return TEXT("Denied");
		case EILLSessionBeaconReservationResult::GameFull: return TEXT("GameFull");
		case EILLSessionBeaconReservationResult::PartyFull: return TEXT("PartyFull");
		case EILLSessionBeaconReservationResult::AuthFailed: return TEXT("AuthFailed");
		case EILLSessionBeaconReservationResult::AuthTimedOut: return TEXT("AuthTimedOut");
		case EILLSessionBeaconReservationResult::BypassFailed: return TEXT("BypassFailed");
		case EILLSessionBeaconReservationResult::MatchmakingClosed: return TEXT("MatchmakingClosed");
		}

		return TEXT("UNKNOWN");
	}
}

/**
 * Delegate triggered when a reservation request from the session beacon host has been denied.
 *
 * @param ReservationResponse response from the server
 */
DECLARE_DELEGATE_OneParam(FILLOnBeaconReservationRequestFailed, EILLSessionBeaconReservationResult::Type/* Response*/);

/**
 * Delegate triggered when a reservation request from the session beacon host has been acknowledged and is pending.
 */
DECLARE_DELEGATE(FILLOnBeaconReservationPending);

/**
 * Delegate triggered when a reservation request from the session beacon host has been accepted.
 */
DECLARE_DELEGATE(FILLOnBeaconReservationAccepted);

/**
 * Delegate triggered when a leave request has been acknowledged by the server.
 */
DECLARE_DELEGATE(FILLOnBeaconLeaveAcked);

/**
 * Delegate triggered when a travel request has been acknowledged by the server.
 */
DECLARE_DELEGATE(FILLOnBeaconTravelAcked);

/**
 * Delegate triggered when a shutdown notification was sent from the server.
 */
DECLARE_DELEGATE(FILLOnBeaconHostShutdown);

/**
 * @class AILLSessionBeaconClient
 * @brief Client connection actor and server representation actor for a session beacon.
 *		  You can think of these as controllers, because all of them exist on the server and players only know of their own.
 */
UCLASS(Config=Engine)
class ILLGAMEFRAMEWORK_API AILLSessionBeaconClient
: public AILLOnlineBeaconClient
{
	GENERATED_UCLASS_BODY()

	// Begin AActor interface
	virtual bool UseShortConnectTimeout() const override;
	// End AActor interface

protected:
	// Begin OnlineBeacon interface
	virtual void OnFailure() override;
	// End OnlineBeacon interface

protected:
	/** @return Named session for InSessionName, if one exists. */
	FNamedOnlineSession* GetValidNamedSession(FName InSessionName) const;

	// Initial connection timeout for SteamP2P connections, which need a little more time
	UPROPERTY(Config)
	float SteamP2PConnectionInitialTimeout;

	// Time beacon will wait for packets after establishing a connection before giving up in Shipping
	UPROPERTY(Config)
	float BeaconConnectionTimeoutShipping;

	//////////////////////////////////////////////////
	// Connectivity
public:
	/**
	 * Version of InitClient that's virtual! Also performs some additional special setup.
	 *
	 * @param ConnectionString IpAddress:Port to establish a connection with.
	 * @param AccountId Backend account identifier of the connecting client.
	 * @param UniqueId Platform account identifier of the connecting client.
	 * @return true if successful.
	 */
	virtual bool InitClientBeacon(const FString& ConnectionString, const FILLBackendPlayerIdentifier& AccountId, const FUniqueNetIdRepl& UniqueId);

protected:
	// Last ConnectionString called for InitClientBeacon
	FString CurrentConnectionString;

	//////////////////////////////////////////////////
	// Authorization
public:
	/** @return true if we can not finish accepting this member due to pending authorization checks. */
	virtual bool IsPendingAuthorization() const { return false; }

	//////////////////////////////////////////////////
	// Reservations
public:
	/** @return Backend player identifier for this client. */
	FORCEINLINE const FILLBackendPlayerIdentifier& GetClientAccountId() const { return ClientAccountId; }

	/** @return Unique identifier for this client. */
	FORCEINLINE FUniqueNetIdRepl GetClientUniqueId() const { return ClientUniqueId; }

	/** @return Backend player identifier for the beacon owner. */
	FORCEINLINE const FILLBackendPlayerIdentifier& GetBeaconOwnerAccountId() const { return BeaconOwnerAccountId; }

	/** @return Unique identifier for the session owner. */
	FORCEINLINE FUniqueNetIdRepl GetSessionOwnerUniqueId() const { return SessionOwnerUniqueId; }

	/** @return true if we have connected and our reservation was accepted. Reservations are not accepted until after authorization completes. */
	virtual bool IsReservationAccepted() const { return bReservationAccepted; }

	/** @return Delegate fired when the reservation request completes. */
	FORCEINLINE FILLOnBeaconReservationRequestFailed& OnBeaconReservationRequestFailed() { return BeaconReservationRequestFailed; }
	FORCEINLINE FILLOnBeaconReservationPending& OnBeaconReservationPending() { return BeaconReservationPending; }
	FORCEINLINE FILLOnBeaconReservationAccepted& OnBeaconReservationAccepted() { return BeaconReservationAccepted; }

protected:
	/**
	 * Establish a connection with DesiredHost and request a reservation.
	 *
	 * @param DesiredHost Host to connect to.
	 * @param Reservation Party/group members.
	 * @param AccountId Backend account identifier of the connecting member.
	 * @param UniqueId Platform account identifier of the connecting member.
	 * @return true if the request was sent. Otherwise HostConnectionFailure will trigger.
	 */
	virtual bool RequestReservation(const FOnlineSessionSearchResult& DesiredHost, const FILLSessionBeaconReservation& Reservation, const FILLBackendPlayerIdentifier& AccountId, const FUniqueNetIdRepl& UniqueId);

	// Our backend account identifier
	FILLBackendPlayerIdentifier ClientAccountId;

	// Our platform identifier
	FUniqueNetIdRepl ClientUniqueId;

	// Our controller ID, not valid on the server/host
	int32 ClientControllerId;

	// Beacon owner backend identifier
	FILLBackendPlayerIdentifier BeaconOwnerAccountId;

	// Session owner platform identifier
	FUniqueNetIdRepl SessionOwnerUniqueId;

	// Reservation we are waiting to send when we get connected
	FILLSessionBeaconReservation PendingReservation;

	// Has the reservation been accepted?
	bool bReservationAccepted;
	
public:
	/** Reply for ServerRequestReservation. */
	virtual void HandleReservationFailed(const EILLSessionBeaconReservationResult::Type Response);

	/** Reply for ServerRequestReservation. */
	virtual void HandleReservationPending();

	/** Reply for ServerRequestReservation. */
	virtual void HandleReservationAccepted(AILLSessionBeaconState* InSessionBeaconState);

protected:
	/** Reply for ServerRequestReservation. */
	UFUNCTION(Client, Reliable)
	virtual void ClientReservationFailed(const EILLSessionBeaconReservationResult::Type Response);

	/** Reply for ServerRequestReservation. */
	UFUNCTION(Client, Reliable)
	virtual void ClientReservationPending();

	/** Reply for ServerRequestReservation. */
	UFUNCTION(Client, Reliable)
	virtual void ClientReservationAccepted(const FILLBackendPlayerIdentifier& InBeaconOwnerAccountId);

	/** Sends a reservation request to the server. */
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerRequestReservation(const FUniqueNetIdRepl& OwnerId, const FILLSessionBeaconReservation& Reservation, const FILLBackendPlayerIdentifier& AccountId, const FUniqueNetIdRepl& UniqueId);

	// Delegates fired when we have received a reservation response
	FILLOnBeaconReservationRequestFailed BeaconReservationRequestFailed;
	FILLOnBeaconReservationPending BeaconReservationPending;
	FILLOnBeaconReservationAccepted BeaconReservationAccepted;

	//////////////////////////////////////////////////
	// Beacon state
public:
	/** @return Session beacon state instance. */
	FORCEINLINE AILLSessionBeaconState* GetSessionBeaconState() const { return SessionBeaconState; }

protected:
	/** Helper function to check if we have our SessionBeaconState and it's members. */
	virtual void CheckSessionBeaconStateHasMembers();

	/** Called when CheckSessionBeaconStateHasMembers confirms we have fully synchronized all members. */
	virtual void SynchedBeaconMembers();

	/** Replication handler for SessionBeaconState. */
	UFUNCTION()
	virtual void OnRep_SessionBeaconState();

	/** Called when the SessionBeaconState replicates it's members so that we can accept our reservation. */
	UFUNCTION()
	virtual void OnStateMembersSentAcceptReservation();

	/** Client notification that they have fully synchronized all members and accepted themselves into our beacon. */
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerSynchronizedMembers();

	// Replicated beacon state
	UPROPERTY(ReplicatedUsing=OnRep_SessionBeaconState)
	AILLSessionBeaconState* SessionBeaconState;

	// Delegate handle for OnStateMembersSentAcceptReservation
	FDelegateHandle StateMembersSentDelegateHandle;

	// Have we done the initial member synchronization?
	bool bInitialMemberSyncComplete;

	//////////////////////////////////////////////////
	// Beacon leaving notification
public:
	/**
	 * Requests to take our reservation and leave.
	 * When the server sends back it's acknowledgment of this, BeaconLeaveAcked will be executed.
	 */
	virtual bool StartLeaving();

	/** @return true if this client is planning to leave next. */
	FORCEINLINE bool IsPendingLeave() const { return (bWaitingLeaveAck || bPendingLeave); }

	/** Delegate fired when our leave has been acknowledged. */
	FORCEINLINE FILLOnBeaconLeaveAcked& OnBeaconLeaveAcked() { return BeaconLeaveAcked; }

protected:
	/** Timeout handler for StartLeaving. */
	UFUNCTION()
	virtual void ServerLeaveTimedOut();

	/** Notifies the server to expect us to be leaving next. */
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerNotifyLeaving();

	/** Reply for ServerNotifyLeaving. */
	UFUNCTION(Client, Reliable)
	virtual void ClientAckLeaving();

	// Client leave server timeout
	UPROPERTY(Config)
	float LeaveServerTimeout;

	// Delegate fired when our leave has been acknowledged
	FILLOnBeaconLeaveAcked BeaconLeaveAcked;

	// Timer handle for ServerLeaveTimedOut
	FTimerHandle TimerHandle_LeaveServerTimeout;

	// Waiting for server to acknowledge leave?
	bool bWaitingLeaveAck;

	// Planning to leave?
	bool bPendingLeave;
	
	//////////////////////////////////////////////////
	// Beacon shutdown notification
public:
	/** Notifies this client that the shutdown has been acknowledged. */
	UFUNCTION(Client, Reliable)
	virtual void ClientNotifyShutdown();

	/** @return Beacon shutdown delegate. */
	FORCEINLINE FILLOnBeaconHostShutdown& OnBeaconHostShutdown() { return BeaconHostShutdown; }

protected:
	/** Reply for ClientNotifyShutdown. */
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerAckShutdown();

	// Delegate fired when the server has said it's going to shutdown
	FILLOnBeaconLeaveAcked BeaconHostShutdown;

	//////////////////////////////////////////////////
	// Beacon travel notification
public:
	/**
	 * Notifies the server that we are about to start traveling to them.
	 *
	 * @return true if the request was sent.
	 */
	virtual bool StartTraveling();

	/** Delegate fired when our travel has been acknowledged. */
	FORCEINLINE FILLOnBeaconTravelAcked& OnBeaconTravelAcked() { return BeaconTravelAcked; }

protected:
	/** Timeout handler for StartTraveling. */
	virtual void ServerTravelAckTimedOut();

	/** Notifies the server this client is planning to travel. */
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerNotifyTraveling();

	/** Reply for ServerNotifyTraveling. */
	UFUNCTION(Client, Reliable)
	virtual void ClientAckTraveling();

	// Client travel server notification ack timeout
	UPROPERTY(Config)
	float TravelServerAckTimeout;
	
	// Delegate fired when our travel has been acknowledged
	FILLOnBeaconTravelAcked BeaconTravelAcked;

	// Timer handle for TravelServerAckTimeout
	FTimerHandle TimerHandle_TravelServerTimeout;
};
