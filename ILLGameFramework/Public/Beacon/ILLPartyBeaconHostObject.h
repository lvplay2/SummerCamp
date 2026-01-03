// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "OnlineSessionInterface.h"

#include "ILLSessionBeaconHostObject.h"
#include "ILLPartyBeaconHostObject.generated.h"

class AILLPartyBeaconClient;
class UILLLocalPlayer;

enum class EILLMatchmakingState : uint8;

/**
 * Delegate fired when party travel coordination completes.
 * At this point party members will have started traveling or will have been kicked from the party for failing to.
 */
DECLARE_DELEGATE(FILLOnPartyTravelCoordinationComplete);

/**
 * @class AILLPartyBeaconHostObject
 */
UCLASS(Config=Engine)
class ILLGAMEFRAMEWORK_API AILLPartyBeaconHostObject
: public AILLSessionBeaconHostObject
{
	GENERATED_UCLASS_BODY()

	// Begin AOnlineBeaconHostObject interface
	virtual void NotifyClientDisconnected(AOnlineBeaconClient* LeavingClientActor) override;
	// End AOnlineBeaconHostObject interface

protected:
	// Begin AILLSessionBeaconHostObject interface
	virtual void HandleClientReservationPending(AILLSessionBeaconClient* BeaconClient, const FILLSessionBeaconReservation& Reservation) override;
	virtual bool CanFinishAccepting(AILLSessionBeaconMemberState* MemberState) const override;
	// End AILLSessionBeaconHostObject interface

public:
	/**
	 * Initializes this party beacon for use.
	 *
	 * @param InMaxMembers Maximum number of people allowed in this session.
	 * @param FirstLocalPlayer First local player that will own the party.
	 * @return true if successful.
	 */
	virtual bool InitPartyBeacon(int32 InMaxMembers, UILLLocalPlayer* FirstLocalPlayer);

	/**
	 * Notification from our managing ILLOnlineSessionClient that the party session has been created and started.
	 */
	virtual void NotifyPartySessionStarted();

	/** Kicks ClientActor from the party. */
	virtual void KickPlayer(AILLPartyBeaconClient* ClientActor, const FText& KickReason);

protected:
	// Party leading local player
	UPROPERTY(Transient)
	UILLLocalPlayer* LocalPartyLeader;
	
	//////////////////////////////////////////////////
	// Reservation handling
public:
	/** Accepted response for the addition of a party member with the lobby beacon. */
	virtual void HandlePartyAdditionAccepted(const FILLBackendPlayerIdentifier& MemberAccountId);

	/** Denied response for the addition of a party member with the lobby beacon. */
	virtual void HandlePartyAdditionDenied(const FILLBackendPlayerIdentifier& MemberAccountId);

	//////////////////////////////////////////////////
	// Matchmaking
public:
	/** Notifies all party members that we have started matchmaking. */
	virtual void NotifyPartyMatchmakingStarted();

	/** Notifies all party members of matchmaking state changes. */
	virtual void NotifyPartyMatchmakingState(const EILLMatchmakingState NewState);

	/** Notifies all party members that matchmaking has stopped. */
	virtual void NotifyPartyMatchmakingStopped();

	//////////////////////////////////////////////////
	// Party leader following
public:
	/** Notifies all party members to travel to our game session. */
	virtual bool NotifyTravelToGameSession();

	/** Notifies all party members to travel to the main menu.*/
	virtual void NotifyTravelToMainMenu();

	//////////////////////////////////////////////////
	// Party travel coordination
public:
	/**
	 * Begins coordination of party travel to our currently advertised game session.
	 *
	 * Broadcasts PartyTravelCoordinationComplete upon full client acknowledgment.
	 */
	virtual bool NotifyLobbyReservationAccepted();

	/** Called from AILLPartyBeaconClient::ServerAckJoinLeaderSession. */
	virtual void HandleMemberAckJoinGameSession(AILLPartyBeaconClient* PartyClient);

	/** Called from AILLPartyBeaconClient::ServerJoinedLeaderSession. */
	virtual void HandleMemberJoinedGameSession(AILLPartyBeaconClient* PartyClient);

	/** Called from AILLPartyBeaconClient::ServerFailedToFindLeaderSession. */
	virtual void HandleMemberFailedToFindGameSession(AILLPartyBeaconClient* PartyClient);

	/** Called from AILLPartyBeaconClient::ServerFailedToJoinLeaderSession. */
	virtual void HandleMemberFailedToJoinGameSession(AILLPartyBeaconClient* PartyClient, EOnJoinSessionCompleteResult::Type Result);

	/** @return Delegate fired when party travel coordination completes. */
	FORCEINLINE FILLOnPartyTravelCoordinationComplete& OnPartyTravelCoordinationComplete() { return PartyTravelCoordinationComplete; }

protected:
	/** Helper function for HandleMemberJoinedGameSession and HandleMemberFailedToFind/JoinGameSession. */
	virtual void RemoveAndCheckCoordinationCompletion(AILLPartyBeaconClient* PartyClient);

	/** Called when RemoveAndCheckCoordinationCompletion determines party travel coordination has completed. */
	virtual void TravelCoordinationComplete();

	/** Called when party travel coordination has timed out. */
	virtual void TravelCoordinationTimout();

	// List of party members pending travel coordination ack
	UPROPERTY(Transient)
	TArray<AILLPartyBeaconClient*> ClientsPendingTravelCoordination;

	// Timer handle for TravelCoordinationTimout
	FTimerHandle TimerHandle_TravelCoordinationTimout;

	// Delay after telling all clients to travel before the party host will do so themselves
	UPROPERTY(Config)
	float PartyHostTravelCoordinationTimeout;

	// Delegate fired when party travel coordination completes
	FILLOnPartyTravelCoordinationComplete PartyTravelCoordinationComplete;
};
