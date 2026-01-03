// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "OnlineEngineInterface.h"
#include "OnlineSessionClient.h"

#include "ILLLobbyBeaconClient.h"
#include "ILLOnlineSessionSearch.h"
#include "ILLOnlineSessionSettings.h"
#include "ILLPartyBeaconClient.h"
#include "ILLPartyBeaconHostObject.h"
#include "ILLPartyLocalMessage.h"
#include "ILLProjectSettings.h"
#include "ILLOnlineSessionClient.generated.h"

class AILLGameSession;

class AILLLobbyBeaconClient;
class AILLLobbyBeaconHostObject;
class AILLLobbyBeaconHost;
class AILLLobbyBeaconMemberState;
class AILLLobbyBeaconState;

class AILLPartyBeaconHost;
class AILLPartyBeaconMemberState;
class AILLPartyBeaconState;
class UILLPartyLocalMessage;

class UILLOnlineMatchmakingClient;

enum class EILLBackendArbiterHeartbeatResult : uint8;
enum class EILLBackendMatchmakingStatusType : uint8;

namespace EOnJoinSessionCompleteResult
{
	/** @return Localized description of Result. */
	FText ILLGAMEFRAMEWORK_API GetLocalizedDescription(FName SessionName, const EOnJoinSessionCompleteResult::Type Result);

	/** @return String name for Result. */
	FORCEINLINE const TCHAR* ToString(const EOnJoinSessionCompleteResult::Type Result)
	{
		switch (Result)
		{
		case EOnJoinSessionCompleteResult::Success: return TEXT("Success");
		case EOnJoinSessionCompleteResult::SessionIsFull: return TEXT("SessionIsFull");
		case EOnJoinSessionCompleteResult::SessionDoesNotExist: return TEXT("SessionDoesNotExist");
		case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress: return TEXT("CouldNotRetrieveAddress");
		case EOnJoinSessionCompleteResult::AlreadyInSession: return TEXT("AlreadyInSession");
		case EOnJoinSessionCompleteResult::PlayOnlinePrivilege: return TEXT("PlayOnlinePrivilege");
		case EOnJoinSessionCompleteResult::UnknownError: return TEXT("UnknownError");
		}

		return TEXT("UNKNOWN");
	}
}

/**
 * @class FILLGameSessionSettings
 * @brief General session settings for a game
 */
class ILLGAMEFRAMEWORK_API FILLGameSessionSettings
: public FILLOnlineSessionSettings
{
public:
	FILLGameSessionSettings() {}
	FILLGameSessionSettings(bool bIsLAN, int32 MaxNumPlayers, bool bAdvertise, bool bPassworded);

	virtual ~FILLGameSessionSettings() {}
};

/**
 * Delegate fired for each Lobby beacon member addition/removal.
 *
 * Additions fired off with initial members on lobby host beacon creation, and for each member discovered after joining a lobby beacon, as well as members to join later.
 * Removals fired off when the either the host or client beacons are torn down, as well as for people leaving.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FILLOnLobbyMemberAddedOrRemoved, AILLLobbyBeaconMemberState*, LobbyMember);

/**
 * Delegate fired when CreateParty completes.
 */
DECLARE_DYNAMIC_DELEGATE_OneParam(FILLOnCreatePartyComplete, bool, bWasSuccessful);

/**
 * Delegate fired when DestroyParty completes.
 */
DECLARE_DYNAMIC_DELEGATE_OneParam(FILLOnDestroyPartySessionComplete, bool, bWasSuccessful);

/**
 * Delegate fired for each Party member addition/removal.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FILLOnPartyMemberAddedOrRemoved, AILLPartyBeaconMemberState*, PartyMember);

/**
 * @enum EILLPartyClientState
 */
enum class EILLPartyClientState : uint8
{
	Disconnected,
	InitialConnect,
	Connected,
};

/**
 * @class FILLPartySessionSettings
 */
class ILLGAMEFRAMEWORK_API FILLPartySessionSettings
: public FILLOnlineSessionSettings
{
public:
	FILLPartySessionSettings()
	{
		bAllowInvites = true;
		bAllowJoinInProgress = true;
		bUsesPresence = true;
	}

	virtual ~FILLPartySessionSettings() {}
};

/**
 * @struct FILLPlayTogetherInfo
 */
struct FILLPlayTogetherInfo
{
	FILLPlayTogetherInfo()
	: UserIndex(-1) {}

	FILLPlayTogetherInfo(int32 InUserIndex, const TArray<TSharedPtr<const FUniqueNetId>>& InUserIdList)
	: UserIndex(InUserIndex)
	{
		UserIdList.Append(InUserIdList);
	}

	FORCEINLINE bool IsValid() const { return (UserIndex != -1); }

	int32 UserIndex;
	TArray<TSharedPtr<const FUniqueNetId>> UserIdList;
};

/**
 * @class UILLOnlineSessionClient
 */
UCLASS(Config=Game, Within=ILLGameInstance)
class ILLGAMEFRAMEWORK_API UILLOnlineSessionClient
: public UOnlineSessionClient
{
	GENERATED_UCLASS_BODY()

	// Begin UObject interface
	virtual UWorld* GetWorld() const override;
	// End UObject interface

	// Begin UOnlineSession interface
	virtual void StartOnlineSession(FName SessionName) override;
	// End UOnlineSession interface

	// Begin UOnlineSessionClient interface
	virtual void DestroyExistingSession_Impl(FDelegateHandle& OutResult, FName SessionName, FOnDestroySessionCompleteDelegate& Delegate) override; // NOTE: Intentionally public now
	// End UOnlineSessionClient interface

protected:
	// Begin UOnlineSession interface
	virtual void RegisterOnlineDelegates() override;
	virtual void ClearOnlineDelegates() override;
	virtual void OnDestroyForJoinSessionComplete(FName SessionName, bool bWasSuccessful) override;
	virtual void OnDestroyForMainMenuComplete(FName SessionName, bool bWasSuccessful) override;
	virtual void OnSessionUserInviteAccepted(const bool bWasSuccess, const int32 ControllerId, TSharedPtr<const FUniqueNetId> UserId, const FOnlineSessionSearchResult& InviteResult) override;
	virtual void OnPlayTogetherEventReceived(int32 UserIndex, TArray<TSharedPtr<const FUniqueNetId>> UserIdList) override;
	// End UOnlineSession interface

	// Begin UOnlineSessionClient interface
	virtual bool HandleDisconnectInternal(UWorld* World, UNetDriver* NetDriver) override;
	virtual void OnStartSessionComplete(FName InSessionName, bool bWasSuccessful) override;
	virtual void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result) override;
	virtual void JoinSession(FName SessionName, const FOnlineSessionSearchResult& SearchResult) override;
	// End UOnlineSessionClient interface

public:
	/** @return Online matchmaking instance from our outer game instance. */
	UILLOnlineMatchmakingClient* GetOnlineMatchmaking() const;

	/** Called from AILLGameMode::StartPlay when GetNetMode() returns NM_Standalone, to allow us to cleanup after RegisterServer in the case of NM_ListenServers. */
	virtual void OnRegisteredStandalone();

	// Updated in OnPreLoadMap/OnPostLoadMap
	bool bLoadingMap; // CLEANUP: pjackson: This probably makes more sense in UILLGameInstance

protected:
	/** FWorldDelegates::OnWorldCleanup delegate. */
	virtual void OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);

	/** FCoreUObjectDelegates::PreLoadMap delegate. */
	virtual void OnPreLoadMap(const FString& MapName);

	/** FCoreUObjectDelegates::PostLoadMap delegate. */
	virtual void OnPostLoadMap(UWorld* World);

private:
	// Delegate handle for OnWorldCleanup
	FDelegateHandle OnWorldCleanupDelegateHandle;

	// Delegate handle for OnPreLoadMap
	FDelegateHandle OnPreLoadMapDelegateHandle;

	// Delegate handle for OnPostLoadMap
	FDelegateHandle OnPostLoadMapDelegateHandle;

	//////////////////////////////////////////////////
	// Connection handling
public:
	/**
	 * Signs out of everything and disconnects, reloading the entry level.
	 *
	 * @param Player Local player to be signed out.
	 * @param FailureText Reason for being signed out.
	 */
	virtual void PerformSignoutAndDisconnect(UILLLocalPlayer* Player, FText FailureText);

protected:
	// When non-empty, PerformSignoutAndDisconnect will be called on the next tick after PostLoadMap
	UILLLocalPlayer* PendingSignoutDisconnectFailurePlayer;
	FText PendingSignoutDisconnectFailureText;

	//////////////////////////////////////////////////
	// Invite handling
protected:
	/**
	 * Check if we can join DeferredInviteResult and do so.
	 * @return true if there was a DeferredInviteResult and we joined it.
	 */
	UFUNCTION()
	virtual void CheckDeferredInvite();

	/** Callback for when CheckDeferredInvite requests the party matchmaking status. */
	virtual void OnInvitePartyStatusResponse(EILLBackendMatchmakingStatusType AggregateStatus, float UnBanRealTimeSeconds);

public:
	/** Queues an invite flush. */
	virtual void QueueFlushDeferredInvite();

	/** Clears any pending invite. */
	FORCEINLINE void ClearDeferredInvite()
	{
		DeferredInviteResult = FOnlineSessionSearchResult();
		bInviteSearchFailed = false;
	}

	/** @return true if we have a deferred invite. */
	FORCEINLINE bool HasDeferredInvite() const { return (DeferredInviteResult.IsValid() || bInviteSearchFailed); }

protected:
	/** Callback for the invite JoinSession call made from CheckDeferredInvite.*/
	void OnInviteJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	// Invite result that is pending some blocking condition, like backend auth
	FOnlineSessionSearchResult DeferredInviteResult;

	// Invite result failure flag, to be deferred like DeferredInviteResult
	bool bInviteSearchFailed;

	//////////////////////////////////////////////////
	// Session joining
public:
	/**
	 * Version of JoinSession that accepts a callback, supports a password and deferred map loading.
	 *
	 * @param SessionName Session name. Typically NAME_GameSession.
	 * @param SearchResult Session to join.
	 * @param Callback Completion callback delegate.
	 * @param Password Optional password.
	 * @param TravelOptions Optional additional travel options.
	 * @param bAllowLobbyClientResume Should we allow the lobby beacon client to not be destroyed for recreation?
	 * @return true if the session join request was started, or was deferred pending another blocking session operation.
	 */
	virtual bool JoinSession(FName SessionName, const FOnlineSessionSearchResult& SearchResult, FOnJoinSessionCompleteDelegate Callback, const FString Password = FString(), const FString TravelOptions = FString(), const bool bAllowLobbyClientResume = false);

	/** FIXME: pjackson: Remove this function somehow. */
	virtual void FakeJoinCompletion(FOnJoinSessionCompleteDelegate Callback, EOnJoinSessionCompleteResult::Type Result);

private:
	/** Callback delegate for when NAME_GameSession was destroyed due to some join or beacon failure. */
	virtual void OnDestroyGameForMainMenuComplete(FName SessionName, bool bWasSuccessful);

	// Cached results and callbacks for JoinSession
	FOnlineSessionSearchResult CachedPartySessionResult;
	FOnJoinSessionCompleteDelegate CachedGameSessionCallback;
	FOnJoinSessionCompleteDelegate CachedPartySessionCallback;
	FDelegateHandle OnJoinPartySessionCompleteDelegateHandle;

	// Cached results and callbacks for JoinSession + OnDestroyForJoinSessionComplete
	FOnlineSessionSearchResult EndForGameSessionResult;
	FOnJoinSessionCompleteDelegate EndForGameSessionCallback;
	FOnlineSessionSearchResult EndForPartySessionResult;
	FOnJoinSessionCompleteDelegate EndForPartySessionCallback;

	// Cached password for the travel portion of joining a session
	FString CachedSessionPassword;

	// Cached travel options for the travel portion
	FString CachedGameTravelOptions;

	// Delegate for destroying the game session for a join or beacon failure
	FOnDestroySessionCompleteDelegate OnDestroyGameForMainMenuCompleteDelegate;
	FDelegateHandle OnDestroyGameForMainMenuCompleteDelegateHandle;

	// Total number of times to retry looking up the connection details for a game session
	UPROPERTY(Config)
	int32 GameJoinConnectionAttempts;

	// Time to wait after OnJoinSessionComplete fails to look up connection details before trying again
	UPROPERTY(Config)
	float GameJoinConnectionRetryDelay;

	// Total attempts we have made towards looking up game session connection details
	int32 TotalGameJoinAttempts;

	// Timer handle for retrying OnJoinSessionComplete
	FTimerHandle TimerHandle_RetryGameOnJoinSessionComplete;

	//////////////////////////////////////////////////
	// Basic beacon management
protected:
	/** Helper function that spawns BeaconWorld if it does not exist. */
	virtual UWorld* SpawnBeaconWorld(const FName& WorldName);

	//////////////////////////////////////////////////
	// Game session support
public:
	/** @return true if we are joining a lobby beacon. */
	FORCEINLINE bool IsJoiningGameSession() const { return CachedSessionResult.IsValid(); }

	/** Clears the pending session result. */
	virtual void ClearPendingGameSession()
	{
		CachedGameSessionCallback.Unbind();
		CachedSessionResult = FOnlineSessionSearchResult();
	}

	/**
	 * Host a new online game session.
	 * Called from RegisterServer.
	 *
	 * @param SessionSettings Settings for the game session to host. Will be modified.
	 * @return bool true if successful.
	 */
	virtual bool HostGameSession(TSharedPtr<FILLGameSessionSettings> SessionSettings);

	/**
	 * Attempts to resume a previous game session.
	 * Called from AILLGameMode::StartPlay when RegisterServer is skipped due to the game session already existing.
	 */
	virtual void ResumeGameSession();

	/**
	 * Sends a game session info update to the backend and OSS.
	 */
	virtual void SendGameSessionUpdate();

	/** @return true if the locally hosted game session is open to matchmaking. */
	virtual bool IsHostedGameSessionOpenToMatchmaking() const;

	/** Responsible for updating core settings on InOutSessionSettings. */
	virtual void UpdateGameSessionSettings(FOnlineSessionSettings& InOutSessionSettings);

protected:
	/** Delegate fired when a session create request has completed. */
	virtual void OnCreateGameSessionComplete(FName InSessionName, bool bWasSuccessful);

	/** Delegate fired when a destroying an online session has completed. */
	virtual void OnDestroyGameSessionComplete(FName InSessionName, bool bWasSuccessful);

	/** Delegate fired when NAME_GameSession was destroyed due to game session creation failure. */
	virtual void OnDestroyForGameSessionCreationFailure(FName SessionName, bool bWasSuccessful);

	/** Notification that game session creation has failed, and that we want to return to the main menu. */
	virtual void HandleGameSessionCreationFailure(const FText& FailureReason);

	/** Called after OnPartyTravelCoordinationComplete finishes to perform the actual client travel. */
	virtual void FinishLobbyTravel();

	// Lobby travel URL to connect to once OnLobbyClientReservationAccepted hits
	UPROPERTY(Transient)
	FString LobbyTravelURL;

	// Delegate for creating a new game session
	FOnCreateSessionCompleteDelegate OnCreateGameSessionCompleteDelegate;
	FDelegateHandle OnCreateGameSessionCompleteDelegateHandle;

	// Delegate for destroying our game session
	FOnDestroySessionCompleteDelegate OnDestroyGameSessionCompleteDelegate;
	FDelegateHandle OnDestroyGameSessionCompleteDelegateHandle;
	
	// Delegate for destroying the game session for creation failure
	FOnDestroySessionCompleteDelegate OnDestroyForGameSessionCreationFailureCompleteDelegate;
	FDelegateHandle OnDestroyForGameSessionCreationFailureCompleteDelegateHandle;

	// Minimum time between sending a redundant (when there are no differences in the session settings) game session updates to the OSS in SendGameSessionUpdate
	float RedundantGameSessionUpdateInterval;

	// Last time a game session update was sent to the OSS
	float LastGameSessionUpdateSeconds;

	// Minimum time between sending a redundant (when there are no differences in the session settings) game session reports to the backend in SendGameSessionUpdate
	float RedundantGameBackendReportInterval;

	// Last time a game session report was sent
	float LastGameSessionReportSeconds;

#if USE_GAMELIFT_SERVER_SDK
	// Were we open to matchmaking last update? Used to save on redudant updates to GameLift
	bool bLastGameSessionOpenForMatchmaking;
#endif

	//////////////////////////////////////////////////
	// Lobby beacon
public:
	/** @return Lobby beacon state instance, if hosting or connected to a lobby beacon. */
	AILLLobbyBeaconState* GetLobbyBeaconState() const;

	/** @return World for containing lobby beacon actors. */
	FORCEINLINE UWorld* GetLobbyBeaconWorld() const { return LobbyBeaconWorld; }

	/** @return World for lobby beacon actors. */
	FORCEINLINE UWorld* GetOrCreateLobbyBeaconWorld()
	{
		if (!LobbyBeaconWorld)
		{
			static FName NAME_LobbyBeaconWorld(TEXT("LobbyBeaconWorld"));
			LobbyBeaconWorld = SpawnBeaconWorld(NAME_LobbyBeaconWorld);
		}
		return LobbyBeaconWorld;
	}

	/**
	 * NOTE: These actors live the BeaconWorld! TWeakObjectPtr is recommended.
	 * @return Current lobby beacon member list.
	 */
	FORCEINLINE const TArray<AILLLobbyBeaconMemberState*>& GetLobbyMemberList() const { return LobbyMemberList; }

	/** @return Number of accepted lobby beacon member states. */
	int32 GetNumAcceptedLobbyMembers() const;

	/** @return true if we are locally hosting a lobby beacon. */
	virtual bool IsHostingLobbyBeacon() const;

	/** @return true if we are connected to a lobby beacon.*/
	virtual bool IsInLobbyBeacon() const;

	/** @return Lobby member delegates. */
	FORCEINLINE FILLOnLobbyMemberAddedOrRemoved& OnLobbyMemberAdded() { return LobbyMemberAdded; }
	FORCEINLINE FILLOnLobbyMemberAddedOrRemoved& OnLobbyMemberRemoved() { return LobbyMemberRemoved; }

protected:
	/** Helper function for when you start a lobby beacon or join one successfully and have it replicated to you. */
	virtual void OnLobbyBeaconEstablished();

	/** Callback for AILLLobbyBeaconState::OnBeaconMemberAdded. */
	virtual void OnLobbyBeaconMemberAdded(AILLSessionBeaconMemberState* MemberState);

	/** Callback for AILLLobbyBeaconState::OnBeaconMemberRemoved. */
	virtual void OnLobbyBeaconMemberRemoved(AILLSessionBeaconMemberState* MemberState);

	// World our lobby beacon actors live in
	UPROPERTY(Transient)
	UWorld* LobbyBeaconWorld;

	// Lobby member delegates
	FILLOnLobbyMemberAddedOrRemoved LobbyMemberAdded;
	FILLOnLobbyMemberAddedOrRemoved LobbyMemberRemoved;

	// Cached lobby member list
	UPROPERTY()
	TArray<AILLLobbyBeaconMemberState*> LobbyMemberList;

	//////////////////////////////////////////////////
	// Lobby beacon host
public:
	/**
	 * Called from the AILLGameMode::PreLogin authorized a connecting player.
	 *
	 * @param Options Options the user sent for the login.
	 * @param Address Connection address of the user.
	 * @param UniqueId Platform identifier for the user.
	 * @param [out] ErrorMessage Output error message for denied approvals. Empty means allowed.
	 */
	virtual void FullApproveLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage);

	/** @return Lobby beacon host actor instance. */
	FORCEINLINE AILLLobbyBeaconHostObject* GetLobbyHostBeaconObject() const { return LobbyHostBeaconObject; }

	/** @return Lobby beacon listener actor instance. */
	FORCEINLINE AILLLobbyBeaconHost* GetLobbyBeaconHost() const { return LobbyBeaconHost; }

	/** @return Listen port for LobbyBeaconHost, or 0 for invalid. */
	int32 GetLobbyBeaconHostListenPort() const;

	/**
	 * Allows external control of lobby host beacon accessibility.
	 * By default the lobby host beacon does not allow new requests, so you must call this to allow connections
	 *
	 * @param bPause Should we stop allowing new connections to the lobby host beacon?
	 */
	virtual void PauseLobbyHostBeaconRequests(const bool bPause);

	/**
	 * Spawns the lobby beacon host.
	 *
	 * @param SessionSettings Session settings for the relevant session.
	 * @return true if the lobby host beacon was successfully initialized.
	 */
	virtual bool SpawnLobbyBeaconHost(const FOnlineSessionSettings& SessionSettings);

	/** Helper function for tearing down the lobby beacon host actors. */
	virtual void TeardownLobbyBeaconHost();

protected:
	// Lobby beacon host actor instance
	UPROPERTY(Transient)
	AILLLobbyBeaconHostObject* LobbyHostBeaconObject;

	// Lobby beacon listener actor instance
	UPROPERTY(Transient)
	AILLLobbyBeaconHost* LobbyBeaconHost;

	// Class to use for LobbyHostBeaconObject
	UPROPERTY()
	TSubclassOf<AILLLobbyBeaconHostObject> LobbyHostObjectClass;

	// Class to use for LobbyBeaconHost
	UPROPERTY()
	TSubclassOf<AILLLobbyBeaconHost> LobbyBeaconHostClass;

	//////////////////////////////////////////////////
	// Lobby beacon client
public:
	/** @return Lobby beacon client actor instance. */
	FORCEINLINE AILLLobbyBeaconClient* GetLobbyClientBeacon() const { return LobbyClientBeacon; }

	/**
	 * Creates a lobby beacon client then requests a lobby session from ConnectionString.
	 *
	 * @param ConnectionString IpAddress:Port to connect to.
	 * @param PlayerSessionId Player session identifier to send to the server for authentication.
	 * @param ReceivedDelegate Delegate to fire when the session is received.
	 * @param CompletionDelegate Delegate to fire when the session is joined or join has failed.
	 */
	virtual void BeginRequestLobbySession(const FString& ConnectionString, const FString& PlayerSessionId, FILLOnLobbyBeaconSessionReceived ReceivedDelegate, FILLOnLobbyBeaconSessionRequestComplete CompletionDelegate);

	/**
	 * Creates a lobby beacon client then requests a lobby session for the local player found in PlayerSessions, storing them off for the rest of the party (if any).
	 *
	 * @param PlayerSessions Array of player sessions to distribute amongst our party. Calling it with one entry for the local player is also allowed.
	 * @param ReceivedDelegate Delegate to fire when the session is received.
	 * @param CompletionDelegate Delegate to fire when the session is joined or join has failed.
	 * @return true if PlayerSessions is valid (contains the local player).
	 */
	virtual bool BeginRequestPartyLobbySession(const TArray<FILLMatchmadePlayerSession>& PlayerSessions, FILLOnLobbyBeaconSessionReceived ReceivedDelegate, FILLOnLobbyBeaconSessionRequestComplete CompletionDelegate);

protected:
	/** Called before OnLobbyClientSessionRequestComplete should be called to give us the session details for our party members. */
	virtual void OnLobbyClientSessionReceived(const FString& SessionId);

	/** Lobby session request completion callback. */
	virtual void OnLobbyClientSessionRequestComplete(const EILLLobbyBeaconSessionResult Response);

	/** Lobby host connection failure delegate. */
	virtual void OnLobbyClientHostConnectionFailure();

	/** Lobby host kicked us. */
	virtual void OnLobbyClientKicked(const FText& KickReason);

	/** Reservation request acceptance delegate. */
	virtual void OnLobbyClientReservationAccepted();

	/** Reservation request failure delegate. */
	virtual void OnLobbyClientReservationRequestFailed(const EILLSessionBeaconReservationResult::Type Response);

	/** Spawns our LobbyClientBeacon. */
	virtual void SpawnLobbyBeaconClient();

	/** Helper function for tearing down the lobby client beacon. */
	virtual void TeardownLobbyBeaconClient();

	// Lobby beacon client actor instance
	UPROPERTY(Transient)
	AILLLobbyBeaconClient* LobbyClientBeacon;

	// Class to use for LobbyBeaconClient
	UPROPERTY()
	TSubclassOf<AILLLobbyBeaconClient> LobbyClientClass;

	// Player sessions received to be distributed amongst our party members while we join the lobby session
	TArray<FILLMatchmadePlayerSession> PartyPlayerSessions;

	// Data for BeginRequestLobbySession
	FILLOnLobbyBeaconSessionReceived LobbyBeaconSessionReceivedDelegate;
	FILLOnLobbyBeaconSessionRequestComplete LobbyBeaconSessionRequestCompleteDelegate;

	//////////////////////////////////////////////////
	// Party session support
public:
	/** @return World for containing party beacon actors. */
	FORCEINLINE UWorld* GetPartyBeaconWorld() const { return PartyBeaconWorld; }

	/** @return World for party beacon actors. */
	FORCEINLINE UWorld* GetOrCreatePartyBeaconWorld()
	{
		if (!PartyBeaconWorld)
		{
			static FName NAME_PartyBeaconWorld(TEXT("PartyBeaconWorld"));
			PartyBeaconWorld = SpawnBeaconWorld(NAME_PartyBeaconWorld);
		}
		return PartyBeaconWorld;
	}

	/** @return true if a party can be created at this time. */
	virtual bool CanCreateParty();

	/** @return true if we can invite people to our party. */
	virtual bool CanInviteToParty();

	/**
	 * Attempts to host a party.
	 * Will attempt to create the party session, if one does not exist, and creates the party host beacon actors.
	 * If a party session already exists then it's beacon port will simply be updated.
	 *
	 * @return false if already hosting a party or a party member.
	 */
	virtual bool CreateParty();

	/**
	 * Attempts to destroy a party we are hosting.
	 *
	 * @return false if there isn't one.
	 */
	virtual bool DestroyPartySession();

	/**
	 * Attempts to destroy a party we are hosting and returns to the main menu.
	 * Return to the main menu after hitting OnPartyNetworkFailurePreTravel with NetworkFailureReason.
	 *
	 * @param NetworkFailureReason Error message to display after returning to the main menu.
	 * @return false if there isn't one.
	 */
	virtual bool DestroyPartyForMainMenu(FText NetworkFailureReason);

	/**
	 * Leaves a party if a member and stays in whatever current NAME_GameSession we have alone.
	 * Triggers LeavePartyComplete when complete.
	 *
	 * @return true if the request to destroy the session was started.
	 */
	virtual bool LeavePartySession();

	/**
	 * Leaves a party if a member.
	 * Return to the main menu after hitting OnPartyNetworkFailurePreTravel with NetworkFailureReason.
	 *
	 * @param NetworkFailureReason Error message to display after returning to the main menu.
	 * @return true if the request to destroy the session was started.
	 */
	virtual bool LeavePartyForMainMenu(FText NetworkFailureReason);

	/** @return Last known size of our party. */
	int32 GetPartySize(const bool bEvenWhenNotInOne = true) const;

	/** @return Party beacon state instance, if leading or a member of a party. */
	AILLPartyBeaconState* GetPartyBeaconState() const;

	/** @return Party beacon host actor instance. */
	FORCEINLINE AILLPartyBeaconHostObject* GetPartyHostBeaconObject() const { return PartyHostBeaconObject; }

	/** @return Party beacon listener actor instance. */
	FORCEINLINE AILLPartyBeaconHost* GetPartyBeaconHost() const { return PartyBeaconHost; }

	/** @return Party beacon client actor instance. */
	FORCEINLINE AILLPartyBeaconClient* GetPartyClientBeacon() const { return PartyClientBeacon; }

	/** @return true if we are a party leader who has party members that are still pending. */
	bool HasPendingPartyMembers() const;

	/** @return true if we are creating a party. */
	bool IsCreatingParty() const; // FIXME: pjackson: This also returns true AFTER the party has been created, which is annoying.

	/** @return true if we are in a party as the leader. */
	bool IsInPartyAsLeader() const;

	/** @return true if we are in a party as a member. */
	bool IsInPartyAsMember() const;

	/** @return true if we are joining a party. */
	bool IsJoiningParty() const;

	/** Delegate fired when CreateParty completes. */
	FORCEINLINE FILLOnCreatePartyComplete& OnCreatePartyComplete() { return CreatePartyComplete; }

	/** Delegate fired when DestroyParty completes. */
	FORCEINLINE FILLOnDestroyPartySessionComplete& OnDestroyPartySessionComplete() { return DestroyPartySessionComplete; }

	/** Delegate fired when DestroyParty completes. */
	FORCEINLINE FILLOnDestroyPartySessionComplete& OnLeavePartyComplete() { return LeavePartyComplete; }

protected:
	/** Helper function for broadcasting a party notification. */
	virtual void LocalizedPartyNotification(UILLPartyLocalMessage::EMessage Message, AILLPartyBeaconMemberState* PartyMemberState = nullptr);

	/** Called after LeavePartySession finishes it's disconnection handshake. */
	virtual void FinishLeavingPartySession();

	/** Called after LeavePartyForMainMenu finishes it's disconnection handshake. */
	virtual void FinishLeavingPartyForMainMenu();

	/** Called for the Initializing state when our heartbeat request has completed. */
	virtual void OnCreatePartyBackendPlayerHeartbeat(UILLBackendPlayer* BackendPlayer, EILLBackendArbiterHeartbeatResult HeartbeatResult);

	/** Delegate fired when a session create request has completed. */
	virtual void OnCreatePartySessionComplete(FName SessionName, bool bWasSuccessful);

	/** Delegate fired when a session destroy request has completed. */
	virtual void OnDestroyPartySessionComplete(FName SessionName, bool bWasSuccessful);

	/** Delegate fired when a session leave (destroy) request has completed. */
	virtual void OnLeavePartySessionComplete(FName SessionName, bool bWasSuccessful);

	/** Party host connection failure callback. */
	virtual void OnPartyClientHostConnectionFailure();

	/** Party host kicked us. */
	virtual void OnPartyClientKicked(const FText& KickReason);

	/** Party host shutdown callback. */
	virtual void OnPartyClientHostShutdown();

	/** Party host acknowledged us leaving callback. */
	virtual void OnPartyClientLeaveAcked();

	/** Reservation request acceptance callback. */
	virtual void OnPartyClientReservationAccepted();

	/** Reservation request failure callback. */
	virtual void OnPartyClientReservationRequestFailed(const EILLSessionBeaconReservationResult::Type Response);

	/**
	 * Notification for when we are searching for the party leader's game session.
	 * This may fire several times due to retries.
	 */
	virtual void OnPartyClientFindingLeaderGameSession();

	/** Notification for when we could not find the party leader's game session. */
	virtual void OnPartyClientJoinLeaderGameSessionFailed();

	/** Party client travel callback. */
	virtual bool OnPartyClientTravelRequested(const FOnlineSessionSearchResult& Session, FOnJoinSessionCompleteDelegate Delegate);

	/** Party host and client call for when the party has finished being created successfully or joined successfully respectively. */
	virtual void OnPartyBeaconEstablished();

	/** Callback for AILLPartyBeaconState::OnBeaconMemberAdded. */
	virtual void OnPartyBeaconMemberAdded(AILLSessionBeaconMemberState* MemberState);

	/** Callback for AILLPartyBeaconState::OnBeaconMemberRemoved. */
	virtual void OnPartyBeaconMemberRemoved(AILLSessionBeaconMemberState* MemberState);

	/** Party shutdown full acknowledgment callback. */
	virtual void OnPartyHostShutdownAcked();

	/** Party travel to game session coordination completion callback. */
	virtual void OnPartyHostTravelCoordinationComplete();

	/** Spawns our PartyClientBeacon. */
	virtual void SpawnPartyBeaconClient(const EILLPartyClientState ConnectingState);

	/** Helper function for tearing down the party beacon client actors. */
	virtual void TeardownPartyBeaconClient();

	/** Helper function for tearing down the party beacon host actors. */
	virtual void TeardownPartyBeaconHost();

	// World our party beacon actors live in
	UPROPERTY(Transient)
	UWorld* PartyBeaconWorld;

	// Maximum number of party reservations
	UPROPERTY(Config)
	int32 PartyMaxMembers;

	// Localized message class to use for notifications
	UPROPERTY()
	TSubclassOf<UILLPartyLocalMessage> PartyMessageClass;

	// Party beacon host actor instance
	UPROPERTY(Transient)
	AILLPartyBeaconClient* PartyClientBeacon;

	// Class to use for PartyBeaconClient
	UPROPERTY()
	TSubclassOf<AILLPartyBeaconClient> PartyClientClass;

	// Party beacon host actor instance
	UPROPERTY(Transient)
	AILLPartyBeaconHostObject* PartyHostBeaconObject;

	// Party beacon listener actor instance
	UPROPERTY(Transient)
	AILLPartyBeaconHost* PartyBeaconHost;

	// Class to use for PartyHostBeaconObject
	UPROPERTY()
	TSubclassOf<AILLPartyBeaconHostObject> PartyHostObjectClass;

	// Class to use for PartyBeaconHost
	UPROPERTY()
	TSubclassOf<AILLPartyBeaconHost> PartyBeaconHostClass;

	// Delegate fired when CreateParty completes
	UPROPERTY()
	FILLOnCreatePartyComplete CreatePartyComplete;

	// Delegate fired when DestroyParty completes
	UPROPERTY()
	FILLOnDestroyPartySessionComplete DestroyPartySessionComplete;

	// Delegate fired when LeaveParty completes
	UPROPERTY()
	FILLOnDestroyPartySessionComplete LeavePartyComplete;

	// Delegate for creating a new party session
	FOnCreateSessionCompleteDelegate OnCreatePartySessionCompleteDelegate;

	// Delegate for destroying our party session
	FOnDestroySessionCompleteDelegate OnDestroyPartySessionCompleteDelegate;

	// Delegate for leaving (destroying) our party client session
	FOnDestroySessionCompleteDelegate OnLeavePartySessionCompleteDelegate;

	// Current state of our party client
	EILLPartyClientState PartyClientState;

	// Party session creation delegate handle
	FDelegateHandle OnCreatePartySessionCompleteDelegateHandle;

	// Party session destruction delegate handle
	FDelegateHandle OnDestroyPartySessionCompleteDelegateHandle;

	// Party session leaving (destruction) delegate handle
	FDelegateHandle OnLeavePartySessionCompleteDelegateHandle;

	// Has the party beacon been fully established? Host and client
	bool bPartyBeaconEstablished;

private:
#if !UE_BUILD_SHIPPING
	// Debug commands
	void CmdPartyCreate();
	IConsoleCommand* PartyCreateCommand;

	void CmdPartyDestroy();
	IConsoleCommand* PartyDestroyCommand;

	void CmdPartyInvite();
	IConsoleCommand* PartyInviteCommand;

	void CmdPartyLeave();
	IConsoleCommand* PartyLeaveCommand;
#endif
	
	//////////////////////////////////////////////////
	// PS4 PlayTogether handling
public:
	/**
	 * Creates a party, which automatically calls to this to call SendPlayTogetherInvites for the party.
	 * @return true if there was PlayTogether info to process.
	 */
	virtual bool CheckPlayTogetherParty();
	virtual void AttemptFlushPlayTogetherParty() { CheckPlayTogetherParty(); }

protected:
	/** Resets Play Together PS4 system event info after it's been handled. */
	void ResetPlayTogetherInfo() { PlayTogetherInfo = FILLPlayTogetherInfo(); }

	/** Send all invites for the current game session if we've created it because Play Together on PS4 was initiated. */
	void SendPlayTogetherInvites(FName InSessionName);

	// Play Together on PS4 system event info
	FILLPlayTogetherInfo PlayTogetherInfo;

	// Automatically kick into a random QuickPlay match after sending the Play Together invites out?
	UPROPERTY(Config)
	bool bPlayTogetherAutoHostQuickPlay;

	//////////////////////////////////////////////////
	// Console commands
private:
#if !UE_BUILD_SHIPPING
	void CmdRequestLobbySession(const TArray<FString>& Args);
	IConsoleCommand* RequestLobbySessionCommand;
#endif
};
