// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLOnlineBeaconHostObject.h"
#include "ILLSessionBeaconClient.h"
#include "ILLSessionBeaconMemberState.h"
#include "ILLSessionBeaconHostObject.generated.h"

class FNamedOnlineSession;

class AILLGameMode;
class AILLSessionBeaconState;

/**
 * Delegate triggered when a shutdown request has been acknowledged by all connected clients.
 */
DECLARE_DELEGATE(FILLOnBeaconShutdownAcked);

/**
 * @class AILLSessionBeaconHostObject
 * @brief Session beacon host actor.
 */
UCLASS(Config=Engine)
class ILLGAMEFRAMEWORK_API AILLSessionBeaconHostObject
: public AILLOnlineBeaconHostObject
{
	GENERATED_UCLASS_BODY()

	// Begin AActor Interface
	virtual void Tick(float DeltaTime) override;
	// End AActor Interface

	// Begin AOnlineBeaconHostObject interface
	virtual void NotifyClientDisconnected(AOnlineBeaconClient* LeavingClientActor) override;
	// End AOnlineBeaconHostObject interface
	
protected:
	/** @return Named session for InSessionName, if one exists. */
	FNamedOnlineSession* GetValidNamedSession(FName InSessionName) const;

	/** Verifies beacon member states. Performs timeouts, accepts pending reservations. */
	virtual void TickMembers(float DeltaTime);

	// Time between calls to TickMembers
	float MemberTickInterval;

	// Time until we need to TickMembers
	float TimeUntilMemberTick;

	//////////////////////////////////////////////////
	// State
public:
	/** @return true if this host beacon has been initialized. */
	FORCEINLINE bool IsInitialized() const { return (!SessionName.IsNone() && MaxMembers > 0 && SessionBeaconState); }

	/** @return Maximum number of people allowed. */
	FORCEINLINE int32 GetMaxMembers() const { return MaxMembers; }

	/** @return Session beacon state instance. */
	FORCEINLINE AILLSessionBeaconState* GetSessionBeaconState() const { return SessionBeaconState; }

	/** @return Session tied to this beacon. */
	FORCEINLINE FName GetSessionName() const { return SessionName; }

protected:
	/**
	 * Initializes this beacon for first use.
	 *
	 * @param InSessionName Session this is tied to.
	 * @param InMaxMembers Maximum number of people allowed in this session.
	 * @param ArbiterReservation Reservation for our owner/arbiter.
	 * @return true if successful.
	 */
	virtual bool InitHostBeacon(FName InSessionName, int32 InMaxMembers, const FILLSessionBeaconReservation& ArbiterReservation);

	// Class to use for the session beacon state
	UPROPERTY()
	TSubclassOf<AILLSessionBeaconState> BeaconStateClass;

	// Beacon state
	UPROPERTY()
	AILLSessionBeaconState* SessionBeaconState;

	// Session tied to this beacon
	FName SessionName;

	// Maximum number of people allowed
	int32 MaxMembers;

	//////////////////////////////////////////////////
	// Reservation handling
public:
	/**
	 * Handle a reservation request.
	 * Reply by calling to HandleClientReservationFailed or HandleClientReservationPending on the BeaconClient.
	 */
	virtual void HandleReservationRequest(AILLSessionBeaconClient* BeaconClient, const FILLSessionBeaconReservation& Reservation);

protected:
	/** @return true if this is considered a valid reservation by this beacon. */
	virtual bool ValidateReservation(AILLSessionBeaconClient* BeaconClient, const FILLSessionBeaconReservation& Reservation) const;

	/** Handles a client reservation failure. */
	virtual void HandleClientReservationFailed(AILLSessionBeaconClient* BeaconClient, const EILLSessionBeaconReservationResult::Type Response);

	/** Handles a pending client reservation. */
	virtual void HandleClientReservationPending(AILLSessionBeaconClient* BeaconClient, const FILLSessionBeaconReservation& Reservation);

	/** @return true if HandleClientReservationFailed was called on MemberState. */
	virtual bool FailedPendingMember(AILLSessionBeaconMemberState* MemberState) { return false; }

	/** @return true if we can finally call HandleClientReservationAccepted. */
	virtual bool CanFinishAccepting(AILLSessionBeaconMemberState* MemberState) const { return true; }

	/** Handles a client reservation acceptance. */
	virtual void HandleClientReservationAccepted(AILLSessionBeaconMemberState* MemberState);

	// Initial connection timeout, time limit to becoming an active member
	UPROPERTY(Config)
	int32 InitialConnectionTimeout;

	//////////////////////////////////////////////////
	// Shutdown notification
public:
	/** Notifies all clients that we are about to shut down! */
	virtual void StartShutdown();

	/** Reply handler for StartShutdown ClientActors. */
	virtual void ClientAckedShutdown(AILLSessionBeaconClient* ClientActor);

	/** Delegate fired when our shutdown has been acknowledged. */
	FORCEINLINE FILLOnBeaconShutdownAcked& OnBeaconShutdownAcked() { return BeaconShutdownAcked; }

protected:
	/** Helper function for checking if all ClientActors have acknowledged the shutdown. */
	virtual void CheckShutdownAcked();

	/** Timeout handler for StartShutdown. */
	UFUNCTION()
	virtual void ServerShutdownAckTimeout();

	// Server shutdown client acknowledgment timeout
	UPROPERTY(Config)
	float ShutdownAckTimeout;

	// Delegate fired when all of our clients have acknowledged the shutdown
	FILLOnBeaconShutdownAcked BeaconShutdownAcked;

	// Timer handle for ShutdownAckTimeout
	FTimerHandle TimerHandle_ShutdownAckTimeout;

	//////////////////////////////////////////////////
	// Game mode binding
public:
	/** @return Game mode instance in the game world. */
	virtual AILLGameMode* GetGameMode() const;
};
