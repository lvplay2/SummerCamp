// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "OnlineSessionInterface.h"

#include "ILLOnlineSessionSearch.h"
#include "ILLSessionBeaconClient.h"
#include "ILLPartyBeaconClient.generated.h"

class UILLLocalPlayer;

enum class EILLLobbyBeaconSessionResult : uint8;
enum class EILLMatchmakingState : uint8;

/**
 * Delegate fired when the party leader has started matchmaking.
 */
DECLARE_DELEGATE(FILLOnPartyMatchmakingStarted);

/**
 * Delegate fired for party leader matchmaking status updates.
 */
DECLARE_DELEGATE_OneParam(FILLOnPartyMatchmakingUpdated, EILLMatchmakingState/* State*/);

/**
 * Delegate fired when the party leader has stopped matchmaking.
 */
DECLARE_DELEGATE(FILLOnPartyMatchmakingStopped);

/**
 * Delegate fired when the party leader has requested you join their game session.
 * The first step of that is searching for it, when found FILLOnPartyTravelRequested will be triggered.
 * May fire several times due to search retries!
 */
DECLARE_DELEGATE(FILLOnPartyFindingLeaderGameSession);

/**
 * Delegate fired when party leader game session searching or joining fails.
 */
DECLARE_DELEGATE(FILLOnPartyJoinLeaderGameSessionFailed);

/**
 * Delegate fired when party travel has been requested from our leader.
 *
 * @param Session Session we are joining, or invalid for entry/main menu.
 * @param Delegate Delegate to trigger when joining completes.
 * @return true if travel was able to start.
 */
DECLARE_DELEGATE_RetVal_TwoParams(bool, FILLOnPartyTravelRequested, const FOnlineSessionSearchResult&/* Session*/, FOnJoinSessionCompleteDelegate/* Delegate*/);

/**
 * @class AILLPartyBeaconClient
 */
UCLASS(Config=Engine)
class ILLGAMEFRAMEWORK_API AILLPartyBeaconClient
: public AILLSessionBeaconClient
{
	GENERATED_UCLASS_BODY()

	// Begin AActor interface
	virtual void Destroyed() override;
	virtual bool ShouldReceiveVoiceFrom(const FUniqueNetIdRepl& Sender) const override;
	virtual bool ShouldSendVoice() const override;
	// End AActor interface

	// Begin AOnlineBeaconClient interface
	virtual void OnConnected() override;
	// End AOnlineBeaconClient interface

	// Begin AILLSessionBeaconClient interface
	virtual bool InitClientBeacon(const FString& ConnectionString, const FILLBackendPlayerIdentifier& AccountId, const FUniqueNetIdRepl& UniqueId) override;
	virtual void SynchedBeaconMembers() override;
	virtual void HandleReservationAccepted(AILLSessionBeaconState* InSessionBeaconState) override;
	virtual void ServerSynchronizedMembers_Implementation() override;
	// End AILLSessionBeaconClient interface
	
	//////////////////////////////////////////////////
	// Voice
public:
	/** @return true if we are allowed to use party voice. */
	virtual bool CanUsePartyVoice() const;

	//////////////////////////////////////////////////
	// Reservations
public:
	/**
	 * Establish a connection with DesiredHost and request a party reservation.
	 *
	 * @param DesiredHost Host to connect to.
	 * @param FirstLocalPlayer First local player requesting to join the party.
	 * @return true if the request was sent. Otherwise HostConnectionFailure will trigger.
	 */
	virtual bool RequestJoinParty(const FOnlineSessionSearchResult& DesiredHost, UILLLocalPlayer* FirstLocalPlayer);

	//////////////////////////////////////////////////
	// Matchmaking
public:
	/** @return Matchmaking delegates. */
	FORCEINLINE FILLOnPartyMatchmakingStarted& OnPartyMatchmakingStarted() { return PartyMatchmakingStarted; }
	FORCEINLINE FILLOnPartyMatchmakingUpdated& OnPartyMatchmakingUpdated() { return PartyMatchmakingUpdated; }
	FORCEINLINE FILLOnPartyMatchmakingStopped& OnPartyMatchmakingStopped() { return PartyMatchmakingStopped; }

	/** Party member (client) notification that the server (leader) has started matchmaking. */
	UFUNCTION(Client, Reliable)
	virtual void ClientNotifyMatchmakingStart();

	/** Party member notification for leader matchmaking status changes. */
	UFUNCTION(Client, Reliable)
	virtual void ClientMatchmakingStateUpdate(const uint8 NewState);

	/** Party member notification that the server has stopped matchmaking. */
	UFUNCTION(Client, Reliable)
	virtual void ClientNotifyMatchmakingStop();

protected:
	// Delegate fired when the party leader notifies they have started matchmaking
	FILLOnPartyMatchmakingStarted PartyMatchmakingStarted;

	// Delegate fired for party leader matchmaking status updates
	FILLOnPartyMatchmakingUpdated PartyMatchmakingUpdated;

	// Delegate fired for party leader matchmaking stop
	FILLOnPartyMatchmakingStopped PartyMatchmakingStopped;

	//////////////////////////////////////////////////
	// Party session negotiation
public:
	/** Client notification that this party member needs to follow their leader to ConnectionString. */
	UFUNCTION(Client, Reliable)
	virtual void ClientMatchmakingFollow(const FString& SessionId, const FString& ConnectionString, const FString& PlayerSessionId);

protected:
	/** Result handler for ClientMatchmakingFollow. */
	virtual void OnMatchmakingRequestSessionComplete(const EILLLobbyBeaconSessionResult Response);

	// SessionId for the lobby session we are following our party leader to
	FString FollowingLeaderToSessionId;

	//////////////////////////////////////////////////
	// Party travel coordination
public:
	/**
	 * When OnlineSession is valid:
	 * - Searches for the session that our party owner is in.
	 * - Notifies our party leader of the result via ServerJoinedLeaderSession, ServerFailedToFindLeaderSession or ServerFailedToJoinLeaderSession.
	 * Otherwise:
	 * - Notifies the party leader we are returning to the main menu with ServerJoinedLeaderSession.
	 * - Calls to ClientReturnToMainMenu on the first local PlayerController.
	 */
	virtual void JoinLeadersGameSession(FNamedOnlineSession* OnlineSession);

	/** @return Party travel delegates. */
	FORCEINLINE FILLOnPartyFindingLeaderGameSession& OnPartyFindingLeaderGameSession() { return PartyFindingLeaderGameSession; }
	FORCEINLINE FILLOnPartyJoinLeaderGameSessionFailed& OnPartyJoinLeaderGameSessionFailed() { return PartyJoinLeaderGameSessionFailed; }
	FORCEINLINE FILLOnPartyTravelRequested& OnPartyTravelRequested() { return PartyTravelRequested; }

protected:
	/** Tells the client to find their game session with the identifier WithSessionID and the owner WithSessionOwnerID. */
	UFUNCTION(Client, Reliable)
	virtual void ClientJoinLeadersGameSession(const FString& WithSessionID, const FUniqueNetIdRepl& WithSessionOwnerID);

	/** Client handler for joining the party leader's game session. */
	virtual void AckJoinLeaderSession();

	/** Server notification that we have found the leader's game session and joined it. */
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerAckJoinLeaderSession();

	/** Client handler for completely joining the party leader's game session. */
	virtual void JoinedLeaderSession();

	/** Server notification that we have found the leader's game session and joined it. */
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerJoinedLeaderSession();

	/** Client handler for failing to find the party leader's game session. */
	virtual void FailedToFindLeaderSession();

	/** Server notification that we either could not find the leader's game session. */
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerFailedToFindLeaderSession();

	/** Client handler for failing to join the party leader's game session. */
	virtual void FailedToJoinLeaderSession(uint8 Result);

	/** Server notification that we either could not join the leader's game session. */
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerFailedToJoinLeaderSession(uint8 Result);

	/** Helper function that simply tries to find the party leader's game session. */
	virtual void StartOrRetryFindLeaderGameSession();

	/** Helper function for handling a search failure. */
	virtual void HandleFindResult(const FOnlineSessionSearchResult& FriendSearchResult);

#if PLATFORM_DESKTOP
	/** Callback for when searching for our party leader's game session completes. */
	virtual void OnFindLeaderGameSessionComplete(int32 LocalUserNum, bool bWasSuccessful, const TArray<FOnlineSessionSearchResult>& SearchResults);
#else
	/** Callback for when searching for our party leader's game session completes. */
	virtual void OnFindLeaderGameSessionComplete(int32 LocalUserNum, bool bWasSuccessful, const FOnlineSessionSearchResult& SearchResult);
#endif

	/** Callback for when joining our party leader's game session completes. */
	virtual void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	// Number of search attempts to make to join the leader's game session
	UPROPERTY(Config)
	int32 JoinLeaderSearchAttempts;

	// Time between retries
	UPROPERTY(Config)
	float JoinLeaderRetryDelay;

#if PLATFORM_DESKTOP
	// Delegate for OnFindLeaderGameSessionComplete
	FOnFindFriendSessionCompleteDelegate FindLeaderGameSessionCompleteDelegate;

	// Delegate handle for FindLeaderGameSessionCompleteDelegate
	FDelegateHandle FindLeaderGameSessionCompleteDelegateHandle;
#else
	// Delegate for OnFindLeaderGameSessionComplete
	FOnSingleSessionResultCompleteDelegate FindLeaderGameSessionCompleteDelegate;
#endif

	// Delegate for OnJoinSessionComplete
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;

	// Session identifier of the game session we are looking for
	FString DesiredGameSessionId;

	// Owner of the game session we are looking for, to verify
	FUniqueNetIdRepl DesiredGameSessionOwner;

	// Delegate fired when the party leader has requested you join their game session
	FILLOnPartyFindingLeaderGameSession PartyFindingLeaderGameSession;

	// Delegate fired when party leader game session searching or joining fails
	FILLOnPartyJoinLeaderGameSessionFailed PartyJoinLeaderGameSessionFailed;

	// Delegate fired when party travel has been requested from our leader
	FILLOnPartyTravelRequested PartyTravelRequested;

	// Number of search attempts we have left (server only)
	int32 RemainingSearchAttempts;

public:
	/** Tells this party member to leave the game session they were told to join previously, likely due to some failure. */
	UFUNCTION(Client, Reliable)
	virtual void ClientCancelGameSessionTravel();
};
