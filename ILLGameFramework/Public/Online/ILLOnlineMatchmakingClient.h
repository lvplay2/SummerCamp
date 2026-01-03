// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "OnlineSubsystem.h"
#include "OnlineEngineInterface.h"
#include "OnlineSessionSettings.h"
#include "OnlineSessionInterface.h"

#include "ILLBackendRequestManager.h"
#include "ILLLobbyBeaconClient.h"
#include "ILLOnlineSessionSearch.h"
#include "ILLProjectSettings.h"
#include "ILLOnlineMatchmakingClient.generated.h"

class UILLOnlineSessionClient;

struct FIcmpEchoResult;

// HTTP matchmaking support
// Used as the first method for matching, other methods below (MATCHMAKING_USES_*) are typically the platform-specific fallback
#ifndef MATCHMAKING_SUPPORTS_HTTP_API
# define MATCHMAKING_SUPPORTS_HTTP_API			0
#endif

// Continue delay between processing matchmaking steps, to ease off of the platform services
#ifndef MATCHMAKING_CONTINUE_DELAY
# define MATCHMAKING_CONTINUE_DELAY				.5f
#endif

// Here so UILLBackendPutXBLSessionHTTPRequestTask can use it
#define XBOXONE_QUICKPLAY_SESSIONTEMPLATE		FString(TEXT("QuickPlay"))

#if PLATFORM_PS4
// PS4 grants you the use of up to 6 ints (last 2 are reserved)
# define SETTING_BUILDNUMBER					SETTING_CUSTOMSEARCHINT1
# define SETTING_PLAYERCOUNT					SETTING_CUSTOMSEARCHINT2
# define SETTING_OPENFORMM						SETTING_CUSTOMSEARCHINT3
# define SETTING_PRIORITY						SETTING_CUSTOMSEARCHINT4
#elif PLATFORM_DESKTOP
// Steam don't give a fuck, use what you want
# define SETTING_BUILDNUMBER					FName(TEXT("BUILD"))
# define SETTING_PLAYERCOUNT					FName(TEXT("PCNT"))
# define SETTING_OPENFORMM						FName(TEXT("OPN"))
# define SETTING_PRIORITY						FName(TEXT("PRI"))
#elif PLATFORM_XBOXONE
// Short-bus XB1 gives you 1 fucking string
# define XBOXONE_QUICKPLAY_HOPPER_STANDARD		FString(TEXT("Matchmaking"))
# define XBOXONE_QUICKPLAY_HOPPER_LOWPRIORITY	FString(TEXT("LowPriority"))
# define XBOXONE_QUICKPLAY_MATCHING_TIMEOUT		float(90.f)

# define XBOXONE_PRIVATE_SESSIONTEMPLATE		FString(TEXT("PrivateMatch"))
# define XBOXONE_PARTY_SESSIONTEMPLATE			FString(TEXT("Party"))

# define XBOXONE_QUICKPLAY_KEYWORDS_FORMAT		TEXT("Game_%i")
# define XBOXONE_PARTY_KEYWORDS					FString(TEXT("Party"))
#endif

// SETTING_PRIORITY values
//#if PLATFORM_PS4 || PLATFORM_DESKTOP
# define SESSION_PRIORITY_OK					1
# define SESSION_PRIORITY_LOW					0
//#endif

// Use SmartMatch matchmaking on XboxOne, FindSessions is far too slow
#ifndef MATCHMAKING_USES_MATCHMAKING_API
# define MATCHMAKING_USES_MATCHMAKING_API		(PLATFORM_XBOXONE)
#endif

// FindSessions is the fall back API for matchmaking
#ifndef MATCHMAKING_USES_FINDSESSIONS_API
# define MATCHMAKING_USES_FINDSESSIONS_API		(!MATCHMAKING_USES_MATCHMAKING_API)
#endif

// Error checking
#if (MATCHMAKING_USES_MATCHMAKING_API+MATCHMAKING_USES_FINDSESSIONS_API) > 1
# error "You can not mix matchmaking API uses!"
#endif

/**
 * @enum EILLMatchmakingState
 */
enum class EILLMatchmakingState : uint8
{
	NotRunning,
	Initializing, // Verifying internet connectivity
	SearchingHTTP, // Server-side matching for dedicated servers to join
	SearchingDedicated, // Searching for dedicated servers to join
	SearchingSessions, // Searching for peer-to-peer sessions to join
	SearchingLAN, // Searching for LAN sessions
	TimeRestart, // Restart after the first Initializing step if we have not been searching long enough
	Cooldown, // Waiting to do it all again
	Joining, // Joining a found session/server
	Canceling, // Matchmaking is canceling
	Canceled, // Matchmaking has finished canceling
};

/** @return Localized description of MatchmakingState. */
FText ILLGAMEFRAMEWORK_API GetLocalizedDescription(const EILLMatchmakingState MatchmakingState);

/** @return String name for MatchmakingState. */
FORCEINLINE const TCHAR* ToString(const EILLMatchmakingState MatchmakingState)
{
	switch (MatchmakingState)
	{
	case EILLMatchmakingState::Initializing: return TEXT("Initializing");
	case EILLMatchmakingState::NotRunning: return TEXT("NotRunning");
	case EILLMatchmakingState::SearchingHTTP: return TEXT("SearchingHTTP");
	case EILLMatchmakingState::SearchingDedicated: return TEXT("SearchingDedicated");
	case EILLMatchmakingState::SearchingSessions: return TEXT("SearchingSessions");
	case EILLMatchmakingState::SearchingLAN: return TEXT("SearchingLAN");
	case EILLMatchmakingState::Cooldown: return TEXT("Cooldown");
	case EILLMatchmakingState::Joining: return TEXT("Joining");
	case EILLMatchmakingState::Canceling: return TEXT("Canceling");
	case EILLMatchmakingState::Canceled: return TEXT("Canceled");
	case EILLMatchmakingState::TimeRestart: return TEXT("TimeRestart");
	}

	return TEXT("UNKNOWN");
}

/**
 * @struct FILLMatchmakingStep
 */
USTRUCT()
struct ILLGAMEFRAMEWORK_API FILLMatchmakingStep
{
	GENERATED_USTRUCT_BODY()

	// Matchmaking state for this step
	EILLMatchmakingState MatchmakingState;

#if MATCHMAKING_USES_FINDSESSIONS_API
	// Maximum number of search results to fetch
	int32 MaxResults;

# if PLATFORM_PS4
	// Maximum number of players allowed in this session
	int32 MaxPlayers;
# else
	// Number of players required to be in this session and enabled filtering for non-empty games server-side, or <=0 for no requirement
	int32 NumPlayers;
# endif

	// Maximum ping allowed during this step, or MAX_QUERY_PING for no limit
	int32 PingLimit;
#endif // MATCHMAKING_USES_FINDSESSIONS_API

#if MATCHMAKING_SUPPORTS_HTTP_API || defined(SEARCH_PLAYERSLOTS)
	// Number of player slots that need to be available, for SearchingSessions
	int32 PlayerSlots;
#endif

	// Time requirement to skip over a EILLMatchmakingState::TimeRestart step
	float MinTime;

	// Step to restart into, automatically skips over Initializing steps
	int32 RestartIndex;

	FILLMatchmakingStep(EILLMatchmakingState InMatchmakingState = EILLMatchmakingState::NotRunning)
	: MatchmakingState(InMatchmakingState)
#if MATCHMAKING_USES_FINDSESSIONS_API
	, MaxResults(0)
# if PLATFORM_PS4
	, MaxPlayers(-1)
# else
	, NumPlayers(-1)
# endif // MATCHMAKING_USES_FINDSESSIONS_API
	, PingLimit(MAX_QUERY_PING)
#endif
#if MATCHMAKING_SUPPORTS_HTTP_API || defined(SEARCH_PLAYERSLOTS)
	, PlayerSlots(-1)
#endif
	, MinTime(0.f)
	, RestartIndex(0)
	{}

	/** @return true if this is a cooldown step. */
	FORCEINLINE bool IsCooldownStep() const { return (MatchmakingState == EILLMatchmakingState::Cooldown); }

	/** @return true if this is the initialization step. */
	FORCEINLINE bool IsInitializationStep() const { return (MatchmakingState == EILLMatchmakingState::Initializing); }

	/** @return true if this a search step. */
	FORCEINLINE bool IsSearchStep() const { return (MatchmakingState == EILLMatchmakingState::SearchingHTTP || MatchmakingState == EILLMatchmakingState::SearchingDedicated || MatchmakingState == EILLMatchmakingState::SearchingSessions || MatchmakingState == EILLMatchmakingState::SearchingLAN); }

	/** @return true if this is a valid step. */
	FORCEINLINE bool IsValid() const { return (MatchmakingState != EILLMatchmakingState::NotRunning); }

	/** @return true if this is a valid search step. */
	bool IsValidSearchStep() const
	{
		if (IsValid() && IsSearchStep())
		{
#if MATCHMAKING_USES_FINDSESSIONS_API
			// Ensure we have result-throttling
			if (MaxResults < 0)
				return false;
#endif

#if MATCHMAKING_SUPPORTS_HTTP_API
			// Verify the PlayerSlots is sane
			if (MatchmakingState == EILLMatchmakingState::SearchingHTTP && PlayerSlots < 1)
				return false;
#endif

#if MATCHMAKING_USES_FINDSESSIONS_API && defined(SEARCH_PLAYERSLOTS)
			// Special case for SteamP2P searches
			if (MatchmakingState == EILLMatchmakingState::SearchingSessions && (PlayerSlots < 1 || NumPlayers != -1))
				return false;
#endif

			return true;
		}

		return false;
	}

	// Inline setters
#if MATCHMAKING_USES_FINDSESSIONS_API
	FORCEINLINE FILLMatchmakingStep& SetMaxResults(const int32 InMaxResults)
	{
		MaxResults = InMaxResults;
		return *this;
	}
# if PLATFORM_PS4
	FORCEINLINE FILLMatchmakingStep& SetMaxPlayers(const int32 InMaxPlayers)
	{
		MaxPlayers = InMaxPlayers;
		return *this;
	}
# else
	FORCEINLINE FILLMatchmakingStep& SetNumPlayers(const int32 InNumPlayers)
	{
		NumPlayers = InNumPlayers;
		return *this;
	}
# endif
	FORCEINLINE FILLMatchmakingStep& SetPingLimit(const int32 InPingLimit)
	{
		PingLimit = InPingLimit;
		return *this;
	}
#endif // MATCHMAKING_USES_FINDSESSIONS_API

#if MATCHMAKING_SUPPORTS_HTTP_API || defined(SEARCH_PLAYERSLOTS)
	FORCEINLINE FILLMatchmakingStep& SetPlayerSlots(const int32 InPlayerSlots)
	{
		PlayerSlots = InPlayerSlots;
		return *this;
	}
#endif

	FORCEINLINE FILLMatchmakingStep& SetTimeRestart(const float InMinTime, const int32 InRestartIndex = 0)
	{
		check(MatchmakingState == EILLMatchmakingState::TimeRestart);
		MinTime = InMinTime;
		RestartIndex = InRestartIndex;
		return *this;
	}

	/** @return Debug string for this step. */
	FString ToDebugString() const
	{
		if (IsValidSearchStep())
		{
			FString Result = ToString(MatchmakingState);
			Result += TEXT(":");

#if MATCHMAKING_SUPPORTS_HTTP_API || defined(SEARCH_PLAYERSLOTS)
			Result += FString::Printf(TEXT(" PlayerSlots: %i"), PlayerSlots);
#endif

#if MATCHMAKING_USES_FINDSESSIONS_API
			Result += FString::Printf(TEXT(" MaxResults: %i PingLimit: %i"), MaxResults, PingLimit);
# if PLATFORM_PS4
			Result += FString::Printf(TEXT(" MaxPlayers: %i"), MaxPlayers);
# else
			Result += FString::Printf(TEXT(" NumPlayers: %i"), NumPlayers);
# endif
#endif
			return Result;
		}
		else if (IsCooldownStep())
		{
			return FString(TEXT("Cooldown"));
		}
		else if (IsInitializationStep())
		{
			return FString(TEXT("Initializing"));
		}
		else if (MatchmakingState == EILLMatchmakingState::TimeRestart)
		{
			return FString::Printf(TEXT("%s: MinTime: %f RestartIndex: %i"), ToString(MatchmakingState), MinTime, RestartIndex);
		}

		return FString::Printf(TEXT("INVALID (%s)"), ToString(MatchmakingState));
	}
};

/**
 * @struct FILLHTTPMatchmakingEndpoint
 */
USTRUCT()
struct FILLHTTPMatchmakingEndpoint
{
	GENERATED_USTRUCT_BODY()

	// System name of the endpoint
	FString Name;

	// Address to ping
	FString Address;

	// Number of timeouts we have suppressed
	int32 SuppressedTimeouts;

	// Round trip time for each QoS pass
	TArray<float> PingBucket;

	// Average ping result
	float Latency;

	FILLHTTPMatchmakingEndpoint()
	: SuppressedTimeouts(0)
	, Latency(MAX_QUERY_PING) {}
};

#if MATCHMAKING_SUPPORTS_HTTP_API

# ifndef HTTP_MATCHMAKING_QOS_MAXAGE
#  define HTTP_MATCHMAKING_QOS_MAXAGE				10.f
# endif

# ifndef HTTP_MATCHMAKING_QOS_PASSES
#  define HTTP_MATCHMAKING_QOS_PASSES				4
# endif

# ifndef HTTP_MATCHMAKING_QOS_TIMEOUT_DURATION
#  define HTTP_MATCHMAKING_QOS_TIMEOUT_DURATION		.5f
# endif

# ifndef HTTP_MATCHMAKING_QOS_TIMEOUTS_IGNORED
#  define HTTP_MATCHMAKING_QOS_TIMEOUTS_IGNORED		2
# endif

/**
 * Delegate fired for the matchmaking region requests, after QoS has also been performed on all of the Endpoints.
 *
 * @param bSuccess Successful?
 * @param Endpoints List of server endpoints and their QoS results for matchmaking.
 */
DECLARE_DELEGATE_TwoParams(FILLOnMatchmakingRegionResponseDelegate, bool/* bSuccess*/, const TArray<FILLHTTPMatchmakingEndpoint>&/* Endpoints*/);

/**
 * Delegate fired for the matchmaking queue requests.
 *
 * @param bSuccess Successful?
 * @param RequestId Request identifier if successful.
 * @param RequestDelay Delay that should be used for subsequent describe requests.
 */
DECLARE_DELEGATE_ThreeParams(FILLOnMatchmakingQueueResponseDelegate, bool/* bSuccess*/, const FString&/* RequestId*/, float/* RequestDelay*/);

/**
 * Delegate fired for matchmaking describe requests.
 *
 * @param bFinished Have we received a valid response? PlayerSessions will be non-empty in this case.
 * @param PlayerSessions List of player session results for the AccountIdList in the initial queue request.
 */
DECLARE_DELEGATE_TwoParams(FILLOnMatchmakingDescribeResponseDelegate, bool/* bFinished*/, const TArray<FILLMatchmadePlayerSession>&/* PlayerSessions*/);
#endif

/**
 * Delegate fired for party matchmaking status updates.
 *
 * @param AggregateStatus Worst-possible status found for this party.
 * @param UnBanRealTimeSeconds Real time seconds when we will be unbanned from matchmaking, when AggregateStatus is EILLBackendMatchmakingStatusType::Banned.
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FILLOnAggregateMatchmakingStatusDelegate, EILLBackendMatchmakingStatusType /*AggregateStatus*/, float /*UnBanRealTimeSeconds*/);

/**
 * Delegate fired for matchmaking state changes.
 *
 * @param State New matchmaking state.
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FILLMatchmakingStateChangeDelegate, EILLMatchmakingState /*State*/);

/**
 * @class UILLOnlineMatchmakingClient
 */
UCLASS(Transient, Config=Game, Within=ILLGameInstance)
class ILLGAMEFRAMEWORK_API UILLOnlineMatchmakingClient
: public UObject
{
	GENERATED_UCLASS_BODY()

	// Begin UObject interface
	virtual UWorld* GetWorld() const override;
	// End UObject interface

	/** @return Online session client instance from our outer game instance. */
	UILLOnlineSessionClient* GetOnlineSession() const;

	/** @return Session interface from the online subsystem. */
	IOnlineSessionPtr GetSessionInt() const;

	/** Called after this is spawned to allow binding to online subsystem delegates. */
	virtual void RegisterOnlineDelegates();

	/** Called during game instance shutdown. */
	virtual void ClearOnlineDelegates();

	//////////////////////////////////////////////////
	// Matchmaking status
public:
	/** @return Delegate to bind to for RequestPartyMatchmakingStatus results. */
	FORCEINLINE FILLOnAggregateMatchmakingStatusDelegate& GetAggregateMatchmakingStatusDelegate() { return AggregateMatchmakingStatusDelegate; }

	/** @return LastAggregateMatchmakingStatus for our party. Will never be EILLBackendMatchmakingStatusType::Error, that gets converted to Ok. */
	FORCEINLINE EILLBackendMatchmakingStatusType GetLastAggregateMatchmakingStatus() const { return IsRunningDedicatedServer() ? EILLBackendMatchmakingStatusType::Ok : LastAggregateMatchmakingStatus; }

	/** @return LastAggregateMatchmakingUnbanRealTime for our party. */
	FORCEINLINE float GetLastAggregateMatchmakingUnbanRealTime() const { return LastAggregateMatchmakingUnbanRealTime; }

	/** Clears the LastAggregateMatchmakingStatus so that it must be requested again. */
	FORCEINLINE void InvalidateMatchmakingStatus() { LastAggregateMatchmakingStatus = EILLBackendMatchmakingStatusType::Unknown; }

	/**
	 * Requests a matchmaking status update for the local party.
	 * Note that you will never receive EILLBackendMatchmakingStatusType::Error as an aggregate status. It would not be nice to punish people for database failures.
	 */
	virtual void RequestPartyMatchmakingStatus();

protected:
	/** Callback triggered for the response to RequestPartyMatchmakingStatus. */
	virtual void OnMatchmakingStatusResponse(const TArray<FILLBackendMatchmakingStatus>& MatchmakingStatuses);

	// Delegate to fire when RequestPartyMatchmakingStatus completes
	FILLOnAggregateMatchmakingStatusDelegate AggregateMatchmakingStatusDelegate;

	// Aggregate matchmaking status from OnMatchmakingPartyStatusResponse
	EILLBackendMatchmakingStatusType LastAggregateMatchmakingStatus;

	// Aggregate matchmaking ban time, real time seconds when the ban will be lifted
	float LastAggregateMatchmakingUnbanRealTime;

	// Are we waiting on a response for RequestPartyMatchmakingStatus?
	bool bPendingPartyMatchmakingStatus;

	//////////////////////////////////////////////////
	// Matchmaking
public:
	/**
	 * Begins the matchmaking process.
	 *
	 * @param bForLAN Is this a LAN query?
	 *
	 * @return true if matchmaking was started.
	 */
	virtual bool BeginMatchmaking(const bool bForLAN);

	/**
	 * Cancels a previous matchmaking requests.
	 *
	 * @return true if matchmaking was running and stopped.
	 */
	virtual bool CancelMatchmaking();

	/** @return true if we are currently matchmaking. */
	FORCEINLINE bool IsMatchmaking() const { return (MatchmakingState != EILLMatchmakingState::NotRunning); }

	/** @return Current matchmaking step, or an invalid step if not matchmaking. */
	FILLMatchmakingStep GetCurrentMatchmakingStep() const;

	/** @return Real time that matchmaking was started, or <0 if it's not running. */
	FORCEINLINE float GetMatchmakingStartRealTime() const { return MatchmakingStartRealTime; }

	/** @return Current matchmaking state. */
	FORCEINLINE EILLMatchmakingState GetMatchmakingState() const { return MatchmakingState; }

	/** @return Matchmaking state change delegate for binding. */
	FILLMatchmakingStateChangeDelegate& GetMatchmakingStateChangeDelegate() { return MatchmakingStateChangeDelegate; }

	/** @return true if we support regionalized matchmaking. */
	virtual bool SupportsRegionalizedMatchmaking() const;

protected:
	/** Builds the matchmaking search steps. */
	virtual void BuildMatchmakingSearchSteps();

#if MATCHMAKING_USES_MATCHMAKING_API || MATCHMAKING_USES_FINDSESSIONS_API
	/** Builds the matchmaking search object. */
	virtual void BuildMatchmakingSearchObject();
#endif

	/** Called for the first attempt, or the previous matchmaking attempt failed and we are trying again. Searches for sessions, which are handled in OnMatchmakingSearchCompleted. */
	virtual void ContinueMatchmaking(bool bNextStep = true);

	/**
	 * Allows games to implement their matchmaking behavior.
	 *
	 * @return Largest session in FromList that has room for GetPartySize.
	 */
	virtual const FOnlineSessionSearchResult* FindBestSessionResult(const TArray<FOnlineSessionSearchResult>& FromList) const;

	/** Changes the matchmaking state. */
	virtual void SetMatchmakingState(const EILLMatchmakingState NewState);

	/** Delegate fired when the application is suspended while matchmaking! What a weirdo! */
	virtual void OnMatchmakingApplicationEnterBackground();

	/** Called for the Initializing state when our heartbeat request has completed. */
	virtual void OnMatchmakingBackendPlayerHeartbeat(UILLBackendPlayer* BackendPlayer, EILLBackendArbiterHeartbeatResult HeartbeatResult);

	/** Called for the Initializing state when our matchmaking aggregate status request has completed.*/
	virtual void OnMatchmakingPartyStatusResponse(EILLBackendMatchmakingStatusType AggregateStatus, float UnBanRealTimeSeconds);

	/** Called to perform a matchmaking cooldown. */
	virtual void PerformMatchmakingCooldown();

	/** Called when the Canceling state completes. Starts a timer for MatchmakingCanceledComplete. */
	virtual void MatchmakingCancelingComplete();

	/** Called when the Canceling state cool down timer completes. */
	virtual void MatchmakingCanceledComplete();

	/** Called when we have exhausted all of the MatchmakingSteps. */
	virtual void MatchmakingFinished();

#if MATCHMAKING_SUPPORTS_HTTP_API
	/**
	 * Builds an HTTP request to collect all of the regions for matchmaking so we can perform QoS on them.
	 *
	 * @param Delegate Completion delegate.
	 */
	virtual UILLBackendSimpleHTTPRequestTask* BuildMatchmakingRegionsRequest(FILLOnMatchmakingRegionResponseDelegate& Delegate);

	/** Response parser for BuildMatchmakingRegionsRequest. */
	virtual void MatchmakingRegionsResponse(int32 ResponseCode, const FString& ResponseContent);

	/** Sends a matchmaking QoS UDP request. */
	virtual void SendMatchmakingRegionUDP(FILLHTTPMatchmakingEndpoint& EndpointEntry);

	/** Called every time a matchmaking endpoint UDP completes to check if we are all done or need to do another pass. */
	virtual void OnMatchmakingRegionQoSResponse(FILLHTTPMatchmakingEndpoint& EndpointEntry, const FIcmpEchoResult& Result);

# if PLATFORM_XBOXONE
	// Name of the SDA for the QoS connection
	FString QoSTemplateName;

	// SDA for the QoS connection
	Windows::Xbox::Networking::SecureDeviceAssociationTemplate^ QoSPeerTemplate;
	Windows::Foundation::EventRegistrationToken QoSAssociationIncoming;
# endif

	// List of matchmaking endpoints
	TArray<FILLHTTPMatchmakingEndpoint> MatchmakingEndpoints;

	// Delegate fired from MatchmakingRegionsResponse
	FILLOnMatchmakingRegionResponseDelegate OnMatchmakingRegionResponse;

	/** Response handler for SendMatchmakingRegionsRequest. */
	virtual void MatchmakingRegionsHandler(bool bSuccess, const TArray<FILLHTTPMatchmakingEndpoint>& Endpoints);

	/** Called during the matchmaking initialization process to collect regions and perform QoS. */
	virtual void SendMatchmakingRegionsRequest();

	// Last real time that we called SendMatchmakingRegionsRequest
	float LastRegionRequestRealTime;

	/**
	 * Builds an HTTP request to queue AccountIdList for matchmaking.
	 *
	 * @param AccountIdList List of player database accounts that are queuing.
	 * @param EndpointList List of latency results for matchmaking regions.
	 * @param bSkipWaitForPlayersDelay Skip the wait time for other players to occupy the game session? Can lead to fragmentation, only use this when speed is more important than quality match sizing.
	 * @param Delegate Delegate to trigger for the response.
	 */
	virtual UILLBackendSimpleHTTPRequestTask* BuildMatchmakingQueueRequest(const TArray<FILLBackendPlayerIdentifier>& AccountIdList, const TArray<FILLHTTPMatchmakingEndpoint>& EndpointList,const bool bSkipWaitForPlayersDelay, FILLOnMatchmakingQueueResponseDelegate& Delegate);

	/** Response parser for BuildMatchmakingQueueRequest. */
	virtual void MatchmakingQueueResponse(int32 ResponseCode, const FString& ResponseContent);

	// Delegate fired from MatchmakingQueueResponse
	FILLOnMatchmakingQueueResponseDelegate OnMatchmakingQueueResponse;

	/** Requests to start HTTP matchmaking. */
	virtual bool SendMatchmakingQueueRequest();

	/** Response handler for SendMatchmakingQueueRequest. */
	virtual void MatchmakingQueueHandler(bool bSuccess, const FString& RequestId, float RequestDelay);

	/**
	 * Builds an HTTP request to describe a previously-started queue request, to see if we're ready to join.
	 *
	 * @param RequestId Previous matchmaking queue request identifier received.
	 * @param Delegate Delegate to trigger for the response.
	 */
	virtual UILLBackendSimpleHTTPRequestTask* BuildMatchmakingDescribeRequest(const FString& RequestId, FILLOnMatchmakingDescribeResponseDelegate& Delegate);

	/** Response parser for BuildMatchmakingDescribeRequest. */
	virtual void MatchmakingDescribeResponse(int32 ResponseCode, const FString& ResponseContent);

	// Delegate fired from MatchmakingDescribeResponse
	FILLOnMatchmakingDescribeResponseDelegate OnMatchmakingDescribeResponseDelegate;

	/** Requests a describe on a previously started HTTP MatchmakingRequestId. */
	virtual void SendMatchmakingDescribeRequest();

	/** Response handler for SendMatchmakingDescribeRequest. */
	virtual void MatchmakingDescribeHandler(bool bFinished, const TArray<FILLMatchmadePlayerSession>& PlayerSessions);

	/**
	 * Builds an HTTP request to cancel a previously-started queue request.
	 *
	 * @param RequestId Previous matchmaking queue request identifier received.
	 * @param Delegate Delegate to trigger for the response.
	 */
	virtual UILLBackendSimpleHTTPRequestTask* BuildMatchmakingCancelRequest(const FString& RequestId, FOnILLBackendSimpleRequestDelegate& Delegate);

	/** Requests a cancellation on a previously started HTTP MatchmakingRequestId. */
	virtual void SendMatchmakingCancelRequest(const bool bForStateChange);

	/** Callback for when a matchmaking HTTP cancel completes. */
	virtual void MatchmakingCancelResponse(int32 ResponseCode, const FString& ResponseContent);

	/** Callback for when a matchmaking HTTP cancel completes. */
	virtual void MatchmakingCancelResponseChangeState(int32 ResponseCode, const FString& ResponseContent);

	/** Delegate fired when a matchmaking lobby session request has received the session. */
	virtual void OnMatchmakingLobbyClientSessionReceived(const FString& SessionId);

	/** Delegate fired when a matchmaking lobby session request completes. */
	virtual void OnMatchmakingRequestSessionComplete(const EILLLobbyBeaconSessionResult Response);

	// Delegates for OnMatchmakingLobbyClientSessionReceived
	FILLOnLobbyBeaconSessionReceived MatchmakingLobbyBeaconSessionReceivedDelegate;
	FDelegateHandle MatchmakingLobbyBeaconSessionReceivedDelegateHandle;

	// Delegates for OnMatchmakingRequestSessionComplete
	FILLOnLobbyBeaconSessionRequestComplete MatchmakingLobbyBeaconSessionRequestCompleteDelegate;
	FDelegateHandle MatchmakingLobbyBeaconSessionRequestCompleteDelegateHandle;
#endif

#if MATCHMAKING_USES_MATCHMAKING_API
	/** Callback for when a matchmaking search completes. */
	virtual void OnMatchmakingSearchCompleted(FName InSessionName, bool bSuccess);

	/** Callback for when matchmaking cancellation with the online subsystem completes. */
	virtual void OnMatchmakingCancelComplete(FName InSessionName, bool bWasSuccessful);
#elif MATCHMAKING_USES_FINDSESSIONS_API
	/** Callback for when a matchmaking FindSession search completes. */
	virtual void OnMatchmakingSearchCompleted(bool bSuccess);
#else
# error
#endif

	/** Callback when the matchmaking session join completes. */
	virtual void OnMatchmakingJoinSessionCompleted(FName InSessionName, EOnJoinSessionCompleteResult::Type Result);

	/** Delegate fired when a destroying an online session has completed. */
	virtual void OnDestroyGameSessionForMatchmakingJoinComplete(FName InSessionName, bool bWasSuccessful);

	/** Delegate fired when a destroying an online session has completed. */
	virtual void OnDestroyGameSessionForMatchmakingCancelComplete(FName InSessionName, bool bWasSuccessful);

	// How much better a given non-empty session's ping must be in order to be taken simply on that
	UPROPERTY(Config)
	int32 MatchmakingBetterPingThreshold;

	// Minimum time to wait between matchmaking failures retries
	UPROPERTY(Config)
	float MatchmakingRetryDelayMin;

	// The most time to wait between matchmaking failures retries
	UPROPERTY(Config)
	float MatchmakingRetryDelayMax;

	// How many matchmaking join attempts have failed
	UPROPERTY(Config)
	int32 MatchmakingMaxConsoleJoinFailures;

	// How many matchmaking join attempts have failed
	UPROPERTY(Config)
	int32 MatchmakingMaxDesktopJoinFailures;

	// Steps to perform for matchmaking
	TArray<FILLMatchmakingStep> MatchmakingSteps;

	// Matchmaking state change delegate
	FILLMatchmakingStateChangeDelegate MatchmakingStateChangeDelegate;

#if MATCHMAKING_USES_MATCHMAKING_API || MATCHMAKING_USES_FINDSESSIONS_API
	// Search object used for matchmaking
	TSharedPtr<FILLOnlineSessionSearch> MatchmakingSearchObject;
#endif

	// How many matchmaking search attempts we have made
	int32 MatchmakingStepIndex;

	// How many matchmaking join attempts we have made
	int32 MatchmakingJoinCount;

	// How many matchmaking join attempts have failed
	int32 MatchmakingJoinFailures;

	// Matchmaking join delegate
	FOnJoinSessionCompleteDelegate MatchmakingJoinSessionDelegate;

	// Delegate for OnDestroyGameSessionForMatchmakingJoinComplete
	FOnDestroySessionCompleteDelegate DestroyGameSessionForMatchmakingJoinCompleteDelegate;
	FDelegateHandle DestroyGameSessionForMatchmakingJoinCompleteDelegateHandle;

	// Delegate for OnDestroyGameSessionForMatchmakingCancelComplete
	FOnDestroySessionCompleteDelegate DestroyGameSessionForMatchmakingCancelCompleteDelegate;
	FDelegateHandle DestroyGameSessionForMatchmakingCancelCompleteDelegateHandle;

	// Matchmaking black list, stores previous matchmaking fail session owners
	TArray<TSharedPtr<const FUniqueNetIdString>> MatchmakingBlacklist;

#if MATCHMAKING_SUPPORTS_HTTP_API
	// Matchmaking HTTP minimum time
	float MatchmakingHTTPMinimumTime;

	// Matchmaking HTTP time limit for a request
	float MatchmakingHTTPGiveUpTime;

	// Matchmaking regions request handle
	UILLBackendSimpleHTTPRequestTask* MatchmakingRegionsHTTPRequest;

	// Matchmaking queue request handle
	UILLBackendSimpleHTTPRequestTask* MatchmakingQueueHTTPRequest;

	// Time that the matchmaking queue request was sent
	float MatchmakingQueueStartRealTime;

	// Matchmaking describe request handle
	UILLBackendSimpleHTTPRequestTask* MatchmakingDescribeHTTPRequest;

	// Matchmaking cancel request handle
	UILLBackendSimpleHTTPRequestTask* MatchmakingCancelHTTPRequest;

	// Matchmaking request identifier
	FString MatchmakingRequestId;

	// Delay between matchmaking describe requests
	float MatchmakingDescribeDelay;

	// Timer handle to call SendMatchmakingDescribeRequest
	FTimerHandle TimerHandle_SendMatchmakingDescribe;
#endif

#if MATCHMAKING_USES_MATCHMAKING_API
	// Matchmaking search delegate
	FOnMatchmakingCompleteDelegate MatchmakingSearchDelegate;

	// Matchmaking cancel delegate
	FOnCancelMatchmakingCompleteDelegate MatchmakingCancelDelegate;

	// Delegate handle for OnMatchmakingCancelComplete
	FDelegateHandle MatchmakingCancelDelegateHandle;
#elif MATCHMAKING_USES_FINDSESSIONS_API
	// Matchmaking search delegate
	FOnFindSessionsCompleteDelegate MatchmakingSearchDelegate;
#else
# error
#endif

#if MATCHMAKING_USES_MATCHMAKING_API || MATCHMAKING_USES_FINDSESSIONS_API
	// Delegate handle for OnFindMatchmakingSessionsComplete
	FDelegateHandle MatchmakingSearchDelegateHandle;
#endif

	// Delegate handles for console suspend while matchmaking
	FDelegateHandle MatchmakingApplicationEnterBackgroundDelegateHandle;

	// Timer handle for canceling matchmaking
	FTimerHandle TimerHandle_CancelMatchmaking;

	// Timer handle for retrying matchmaking
	FTimerHandle TimerHandle_ContinueMatchmaking;

	// Current matchmaking state
	EILLMatchmakingState MatchmakingState;

	// Time that matchmaking started
	float MatchmakingStartRealTime;

	// LAN matchmaking search?
	bool bMatchmakingLAN;

	//////////////////////////////////////////////////
	// Instance management
public:
	/**
	 * Starts the process of requesting a replacement server for this one when a server is about to recycle.
	 *
	 * @return true if this is supported and the process has begun.
	 */
	virtual bool BeginRequestServerReplacement();

	/** @return true if we are in the process of searching for a server replacement. */
	virtual bool PendingServerReplacement() const
	{
#if MATCHMAKING_SUPPORTS_HTTP_API
		return (ReplacementStartRealTime != -1.f);
#else
		return false;
#endif
	}

	/** @return true if this platform supports requesting server replacements. */
	virtual bool SupportsServerReplacement() const;

	/**
	 * Flags this disconnect as one intended for returning to matchmaking then disconnects and tears down connections.
	 *
	 * @param World Game world context.
	 * @param NetDrive Network driver to disconnect.
	 * @param PlayerSession Optional list of player sessions to matchmake into. Handed to us from a server that told us to migrate.
	 */
	virtual bool HandleDisconnectForMatchmaking(UWorld* World, UNetDriver* NetDriver, const TArray<FILLMatchmadePlayerSession>& PlayerSessions = TArray<FILLMatchmadePlayerSession>());

	/** @return true if we should start matchmaking after loading the main menu, and flag it false because we are going to do that. */
	UFUNCTION(BlueprintPure, Category="InstanceManagement", meta=(WorldContext="WorldContextObject"))
	static bool ConsumeMatchmakeAtEntry(UObject* WorldContextObject);

	/** @return Last region used for matchmaking. */
	UFUNCTION(BlueprintPure, Category="InstanceManagement", meta=(WorldContext="WorldContextObject", DeprecatedFunction))
	static FString GetMatchmakeAtEntryRegion(UObject* WorldContextObject) { return FString(); }

protected:
#if MATCHMAKING_SUPPORTS_HTTP_API
	/** Response handler for SendReplacementRegionsRequest. */
	virtual void ReplacementRegionsHandler(bool bSuccess, const TArray<FILLHTTPMatchmakingEndpoint>& Endpoints);

	/** Called during the replacement initialization process to collect regions and perform QoS. */
	virtual void SendReplacementRegionsRequest();

	/** Response handler for BeginRequestServerReplacement. */
	virtual void ReplacementQueueHandler(bool bSuccess, const FString& RequestId, float RequestDelay);

	/** Requests a describe on a previously started HTTP ReplacementRequestId. */
	virtual void SendReplacementDescribeRequest();

	/** Response handler for SendReplacementDescribeRequest. */
	virtual void ReplacementDescribeHandler(bool bFinished, const TArray<FILLMatchmadePlayerSession>& PlayerSessions);

	// Replacement time limit
	float ReplacementGiveUpTime;

	// Time that replacement searching started
	float ReplacementStartRealTime;

	// Replacement regions request handle
	UILLBackendSimpleHTTPRequestTask* ReplacementRegionsHTTPRequest;

	// List of matchmaking endpoints for replacing this server
	TArray<FILLHTTPMatchmakingEndpoint> ReplacementEndpoints;

	// Replacement queue request handle
	UILLBackendSimpleHTTPRequestTask* ReplacementQueueHTTPRequest;

	// Replacement request identifier
	FString ReplacementRequestId;

	// Delay between replacement describe requests
	float ReplacementDescribeDelay;

	// Timer handle to call SendReplacementDescribeRequest
	FTimerHandle TimerHandle_SendReplacementDescribe;

	// Replacement describe request handle
	UILLBackendSimpleHTTPRequestTask* ReplacementDescribeHTTPRequest;

	// Player sessions ready for use by HTTP matchmaking
	TArray<FILLMatchmadePlayerSession> PendingMatchmakingPlayerSessions;
#endif

	// Are we going to attempt to matchmake after returning to the main menu?
	bool bMatchmakeAfterReturnToMenu;

	//////////////////////////////////////////////////
	// Party matchmaking
public:
	/** Called from UILLOnlineSessionClient::FinishLeavingPartySession. */
	virtual void OnFinishedLeavingPartySession();

	/** Called from UILLOnlineSessionClient::FinishLeavingPartyForMainMenu. */
	virtual void OnFinishedLeavingPartyForMainMenu();

	/** Notification for when our party leader has started matchmaking. */
	virtual void OnPartyMatchmakingStarted();

	/** Notification for when our party leader has updated their matchmaking status. */
	virtual void OnPartyMatchmakingUpdated(const EILLMatchmakingState NewState);

	/** Notification for when our party leader has stopped matchmaking. */
	virtual void OnPartyMatchmakingStopped();

	//////////////////////////////////////////////////
	// Console commands
private:
#if !UE_BUILD_SHIPPING
	void CmdQuickPlayCreateLAN();
	IConsoleCommand* QuickPlayCreateLANCommand;

	void CmdQuickPlayCreateListen();
	IConsoleCommand* QuickPlayCreateListenCommand;

	void CmdTestUDPEcho(const TArray<FString>& Args);
	IConsoleCommand* TestUDPEchoCommand;
#endif
};
