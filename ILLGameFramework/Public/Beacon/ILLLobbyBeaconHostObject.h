// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "OnlineSessionInterface.h"

#include "ILLProjectSettings.h"
#include "ILLSessionBeaconHostObject.h"
#include "ILLLobbyBeaconHostObject.generated.h"

class AILLLobbyBeaconClient;
class AILLPartyBeaconState;
class UILLLocalPlayer;
struct FILLBackendPlayerIdentifier;
enum class EILLLobbyBeaconClientAuthorizationResult : uint8;

/**
 * @class AILLLobbyBeaconHostObject
 */
UCLASS()
class ILLGAMEFRAMEWORK_API AILLLobbyBeaconHostObject
: public AILLSessionBeaconHostObject
{
	GENERATED_UCLASS_BODY()
	
	// Begin AOnlineBeaconHostObject interface
	virtual void NotifyClientDisconnected(AOnlineBeaconClient* LeavingClientActor) override;
	// End AOnlineBeaconHostObject interface

protected:
	// Begin AILLSessionBeaconHostObject interface
	virtual void HandleReservationRequest(AILLSessionBeaconClient* BeaconClient, const FILLSessionBeaconReservation& Reservation) override;
	virtual void HandleClientReservationPending(AILLSessionBeaconClient* BeaconClient, const FILLSessionBeaconReservation& Reservation) override;
	virtual bool FailedPendingMember(AILLSessionBeaconMemberState* MemberState) override;
	// End AILLSessionBeaconHostObject interface

public:
	/**
	 * Initializes this party beacon for use.
	 *
	 * @param InMaxMembers Maximum number of people allowed in this session.
	 * @param HostingPartyState Party we are the leader of.
	 * @param FirstLocalPlayer First local player that is initializing the lobby beacon. Must be set if HostingPartyState is not NULL.
	 * @return true if successful.
	 */
	virtual bool InitLobbyBeacon(int32 InMaxMembers, AILLPartyBeaconState* HostingPartyState, UILLLocalPlayer* FirstLocalPlayer);

	//////////////////////////////////////////////////
	// Party handling
public:
	/**
	 * Accepts AccountId as a pending party member for LeaderAccountId.
	 *
	 * @param LeaderAccountId Backend account identifier of the party leader making the request.
	 * @param AccountId Account identifier for the member.
	 * @param UniqueId Who the request is for.
	 * @param DisplayName Name for them.
	 * @return true if there is room for this member.
	 */
	virtual bool AcceptPartyMember(const FILLBackendPlayerIdentifier& LeaderAccountId, const FILLBackendPlayerIdentifier& AccountId, FUniqueNetIdRepl UniqueId, const FString& DisplayName);

	/**
	 * Updates the party leadership of AccountId to be none.
	 *
	 * @param LeaderAccountId Backend account identifier of the party leader.
	 * @param AccountId Backend account identifier to make leader-less.
	 */
	virtual void NotifyPartyMemberRemoved(const FILLBackendPlayerIdentifier& LeaderAccountId, const FILLBackendPlayerIdentifier& AccountId);

	//////////////////////////////////////////////////
	// Authorization
protected:
	/** Callback for when client authorization is completed. */
	virtual void OnClientAuthorizationComplete(AILLLobbyBeaconClient* LobbyClient, EILLLobbyBeaconClientAuthorizationResult Result);

#if USE_GAMELIFT_SERVER_SDK
	/** Callback for when client matchmaking bypass is completed. */
	virtual void OnClientMatchmakingBypassComplete(AILLLobbyBeaconClient* LobbyClient, bool bWasSuccessful);
#endif

	//////////////////////////////////////////////////
	// Console server sessions
protected:
	friend AILLLobbyBeaconClient;

	/** Callback for when PSN game session creation completes. */
	virtual void OnLobbySessionPSNCreated(const FString& SessionId);

	/** Callback for when XBL game session creation completes. */
	virtual void OnLobbySessionXBLCreated(const FString& SessionId, const FString& SessionUri);
};
