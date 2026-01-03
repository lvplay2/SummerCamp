// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLOnlineMatchmakingClient.h"

#include "OnlineSubsystemSessionSettings.h"
#include "OnlineSubsystemUtils.h"

#include "ILLBackendPlayer.h"
#include "ILLBuildDefines.h"
#include "ILLGameBlueprintLibrary.h"
#include "ILLGameInstance.h"
#include "ILLGameMode.h"
#include "ILLGameOnlineBlueprintLibrary.h"
#include "ILLGameSession.h"
#include "ILLLobbyBeaconMemberState.h"
#include "ILLLobbyBeaconState.h"
#include "ILLLocalPlayer.h"
#include "ILLOnlineSessionClient.h"
#include "ILLPartyBeaconMemberState.h"
#include "ILLPartyBeaconState.h"
#include "ILLUDPPing.h"

#if PLATFORM_XBOXONE
using namespace Windows::Xbox::Networking;

extern void LogAssociationStateChange(SecureDeviceAssociation^ Association, SecureDeviceAssociationStateChangedEventArgs^ EventArgs);
#endif

#define LOCTEXT_NAMESPACE "ILLFonic.IGF.ILLOnlineMatchmakingClient"

#if !UE_BUILD_SHIPPING
TAutoConsoleVariable<int32> CVarMatchmakingDebug(
	TEXT("ill.MatchmakingDebug"),
	0,
	TEXT("Enables on screen displays matchmaking information.\n")
	TEXT(" 0: Off\n")
	TEXT(" 1: On\n")
);

TAutoConsoleVariable<int32> CVarMatchmakingTestBan(
	TEXT("ill.MatchmakingTestBan"),
	0,
	TEXT("Forces the response from the database to either put you into low-priority matchmaking, or to receive a 30s ban.\n")
	TEXT(" 0: Normal behavior\n")
	TEXT(" 1: Low priority matchmaking\n")
	TEXT(" 2: Banned from matchmaking for 30s")
);
#endif

FText GetLocalizedDescription(const EILLMatchmakingState MatchmakingState)
{
	switch (MatchmakingState)
	{
	case EILLMatchmakingState::Initializing: return LOCTEXT("MatchmakingState_Initializing", "Initializing");
	case EILLMatchmakingState::NotRunning: return LOCTEXT("MatchmakingState_NotRunning", "Not running");
	case EILLMatchmakingState::SearchingHTTP: // Fall through
	case EILLMatchmakingState::SearchingDedicated: return LOCTEXT("MatchmakingState_SearchingDedicated", "Searching for dedicated servers");
	case EILLMatchmakingState::SearchingSessions: return LOCTEXT("MatchmakingState_SearchingSessions", "Searching for sessions");
	case EILLMatchmakingState::SearchingLAN: return LOCTEXT("MatchmakingState_SearchingLAN", "Searching for LAN sessions");
	case EILLMatchmakingState::Cooldown: return LOCTEXT("MatchmakingState_Cooldown", "Search cooldown");
	case EILLMatchmakingState::Joining: return LOCTEXT("MatchmakingState_Joining", "Checking session for room");
	case EILLMatchmakingState::Canceling: return LOCTEXT("MatchmakingState_Canceling", "Canceling");
	case EILLMatchmakingState::Canceled: return LOCTEXT("MatchmakingState_Canceled", "Canceled");
	case EILLMatchmakingState::TimeRestart: return FText(); // Should never been displayed, processed immediately in ContinueMatchmaking
	}

	return FText::FromString(TEXT("UNKNOWN"));
}

UILLOnlineMatchmakingClient::UILLOnlineMatchmakingClient(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	MatchmakingBetterPingThreshold = 50;
	MatchmakingRetryDelayMin = 1.5f;
	MatchmakingRetryDelayMax = 2.5f;
	MatchmakingMaxConsoleJoinFailures = 6;
	MatchmakingMaxDesktopJoinFailures = 0;
	MatchmakingStartRealTime = -1.f;
#if MATCHMAKING_SUPPORTS_HTTP_API
	MatchmakingHTTPMinimumTime = 180.f;
	MatchmakingHTTPGiveUpTime = 240.f;
	ReplacementGiveUpTime = 120.f;
	ReplacementStartRealTime = -1.f;
#endif
}

UWorld* UILLOnlineMatchmakingClient::GetWorld() const
{
	return GetOuterUILLGameInstance()->GetWorld();
}

UILLOnlineSessionClient* UILLOnlineMatchmakingClient::GetOnlineSession() const
{
	return CastChecked<UILLOnlineSessionClient>(GetOuterUILLGameInstance()->GetOnlineSession());
}

IOnlineSessionPtr UILLOnlineMatchmakingClient::GetSessionInt() const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogOnline, Warning, TEXT("%s: Called with NULL world."), ANSI_TO_TCHAR(__FUNCTION__));
		return nullptr;
	}

	return Online::GetSessionInterface(World);
}

void UILLOnlineMatchmakingClient::RegisterOnlineDelegates()
{
#if MATCHMAKING_USES_MATCHMAKING_API
	MatchmakingSearchDelegate = FOnMatchmakingCompleteDelegate::CreateUObject(this, &ThisClass::OnMatchmakingSearchCompleted);
	MatchmakingCancelDelegate = FOnCancelMatchmakingCompleteDelegate::CreateUObject(this, &ThisClass::OnMatchmakingCancelComplete);
#elif MATCHMAKING_USES_FINDSESSIONS_API
	MatchmakingSearchDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnMatchmakingSearchCompleted);
#else
# error
#endif

#if MATCHMAKING_SUPPORTS_HTTP_API
	MatchmakingLobbyBeaconSessionReceivedDelegate = FILLOnLobbyBeaconSessionReceived::CreateUObject(this, &ThisClass::OnMatchmakingLobbyClientSessionReceived);
	MatchmakingLobbyBeaconSessionRequestCompleteDelegate = FILLOnLobbyBeaconSessionRequestComplete::CreateUObject(this, &ThisClass::OnMatchmakingRequestSessionComplete);
#endif
	MatchmakingJoinSessionDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnMatchmakingJoinSessionCompleted);
	DestroyGameSessionForMatchmakingJoinCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroyGameSessionForMatchmakingJoinComplete);
	DestroyGameSessionForMatchmakingCancelCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroyGameSessionForMatchmakingCancelComplete);

#if !UE_BUILD_SHIPPING
	IConsoleManager& ConsoleManager = IConsoleManager::Get();
	QuickPlayCreateLANCommand = ConsoleManager.RegisterConsoleCommand(TEXT("ill.QuickPlayCreateLAN"), TEXT(""), FConsoleCommandDelegate::CreateUObject(this, &ThisClass::CmdQuickPlayCreateLAN));
	QuickPlayCreateListenCommand = ConsoleManager.RegisterConsoleCommand(TEXT("ill.QuickPlayCreateListen"), TEXT(""), FConsoleCommandDelegate::CreateUObject(this, &ThisClass::CmdQuickPlayCreateListen));
	TestUDPEchoCommand = ConsoleManager.RegisterConsoleCommand(TEXT("ill.TestUDPEcho"), TEXT(""), FConsoleCommandWithArgsDelegate::CreateUObject(this, &ThisClass::CmdTestUDPEcho));
#endif
}

void UILLOnlineMatchmakingClient::ClearOnlineDelegates()
{
#if !UE_BUILD_SHIPPING
	IConsoleManager& ConsoleManager = IConsoleManager::Get();
	ConsoleManager.UnregisterConsoleObject(QuickPlayCreateLANCommand);
	ConsoleManager.UnregisterConsoleObject(QuickPlayCreateListenCommand);
#endif
}

void UILLOnlineMatchmakingClient::RequestPartyMatchmakingStatus()
{
	// Fake success when endpoint is not configured
	UILLBackendRequestManager* BRM = GetOuterUILLGameInstance()->GetBackendRequestManager();
	if (!BRM->ProfileAuthority_MatchmakingStatus)
	{
		UE_LOG(LogOnlineGame, Error, TEXT("%s: Endpoint not configured, faking success!"), ANSI_TO_TCHAR(__FUNCTION__));
		AggregateMatchmakingStatusDelegate.Broadcast(EILLBackendMatchmakingStatusType::Ok, -1.f);
		AggregateMatchmakingStatusDelegate.Clear();
		return;
	}
	if (bPendingPartyMatchmakingStatus)
	{
		// Already pending results, wait for the previous request
		// FIXME: pjackson: Cancel the previous HTTP request and send a new one? In case the party size changes between requests
		return;
	}

	// Generate an account identifier list
	UWorld* World = GetWorld();
	UILLOnlineSessionClient* OSC = GetOnlineSession();
	UILLLocalPlayer* FirstLocalPlayer = CastChecked<UILLLocalPlayer>(World->GetFirstLocalPlayerFromController());
	UILLBackendPlayer* BackendPlayer = FirstLocalPlayer->GetBackendPlayer();
	TArray<FILLBackendPlayerIdentifier> AccountIdList;
	if (AILLPartyBeaconState* PartyBeaconState = OSC->GetPartyBeaconState())
	{
		AccountIdList = PartyBeaconState->GenerateAccountIdList();
	}
	else
	{
		AccountIdList.Add(BackendPlayer->GetAccountId());
	}

	// Send a request to the backend to get matchmaking status
	if (BRM->RequestMatchmakingStatus(BackendPlayer, AccountIdList, FOnILLBackendMatchmakingStatusDelegate::CreateUObject(this, &ThisClass::OnMatchmakingStatusResponse)))
	{
		bPendingPartyMatchmakingStatus = true;
	}
	else
	{
		AggregateMatchmakingStatusDelegate.Broadcast(EILLBackendMatchmakingStatusType::Error, -1.f);
		AggregateMatchmakingStatusDelegate.Clear();
	}
}

void UILLOnlineMatchmakingClient::OnMatchmakingStatusResponse(const TArray<FILLBackendMatchmakingStatus>& MatchmakingStatuses)
{
	UWorld* World = GetWorld();

	// Find the aggregate (worst) status
	EILLBackendMatchmakingStatusType AggregateStatus = EILLBackendMatchmakingStatusType::Unknown;
	float AggregateBanSeconds = -1.f;
	for (const FILLBackendMatchmakingStatus& MemberStatus : MatchmakingStatuses)
	{
		switch (MemberStatus.MatchmakingStatus)
		{
		case EILLBackendMatchmakingStatusType::Error: // Fall-through
		case EILLBackendMatchmakingStatusType::Ok:
			if (AggregateStatus == EILLBackendMatchmakingStatusType::Unknown)
				AggregateStatus = EILLBackendMatchmakingStatusType::Ok;
			break;

		case EILLBackendMatchmakingStatusType::Unknown: // Fall-through
		case EILLBackendMatchmakingStatusType::LowPriority:
			if (AggregateStatus != EILLBackendMatchmakingStatusType::Banned)
			{
				AggregateStatus = EILLBackendMatchmakingStatusType::LowPriority;
			}
			break;

		case EILLBackendMatchmakingStatusType::Banned:
			AggregateStatus = EILLBackendMatchmakingStatusType::Banned;
			AggregateBanSeconds = FMath::Max(AggregateBanSeconds, MemberStatus.MatchmakingBanSeconds);
			break;
		}
	}

#if !UE_BUILD_SHIPPING
	// Debug stuff
	if (CVarMatchmakingTestBan.GetValueOnGameThread() == 1)
	{
		AggregateStatus = EILLBackendMatchmakingStatusType::LowPriority;
	}
	else if (CVarMatchmakingTestBan.GetValueOnGameThread() == 2)
	{
		AggregateStatus = EILLBackendMatchmakingStatusType::Banned;
		AggregateBanSeconds = 30.f;
	}

	if (CVarMatchmakingDebug.GetValueOnGameThread() == 1)
	{
		UKismetSystemLibrary::PrintString(World, FString::Printf(TEXT("Matchmaking status: %s (%f)"), ToString(AggregateStatus), AggregateBanSeconds), true, true, FLinearColor::White, 5.f);
	}
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: Status: %s (%f)"), ANSI_TO_TCHAR(__FUNCTION__), ToString(AggregateStatus), AggregateBanSeconds);
#endif

	// Adjust AggregateBanSeconds and cache the results
	if (AggregateStatus == EILLBackendMatchmakingStatusType::Banned)
	{
		check(AggregateBanSeconds > 0.f);
		AggregateBanSeconds = World->GetRealTimeSeconds() + AggregateBanSeconds;
	}
	LastAggregateMatchmakingStatus = AggregateStatus;
	LastAggregateMatchmakingUnbanRealTime = AggregateBanSeconds;
	bPendingPartyMatchmakingStatus = false;

	// Broadcast
	AggregateMatchmakingStatusDelegate.Broadcast(AggregateStatus, AggregateBanSeconds);
	AggregateMatchmakingStatusDelegate.Clear();
}

bool UILLOnlineMatchmakingClient::BeginMatchmaking(const bool bForLAN)
{
	if (IsMatchmaking())
	{
		return false;
	}

	// Wait for party creation to complete
	UILLOnlineSessionClient* OSC = GetOnlineSession();
	if (OSC->IsCreatingParty() && !OSC->IsInPartyAsLeader())
	{
		return false;
	}

	UWorld* World = GetWorld();

#if PLATFORM_DESKTOP // FIXME: pjackson
	// Force garbage collection to run now
	// Ensures any dangling SteamP2P sockets are flushed before creating new ones, there's a bug allow ones pending kill to be used!
	GEngine->PerformGarbageCollectionAndCleanupActors();
#endif

	const bool bIsPartyMemberOrJoining = (OSC->IsJoiningParty() || OSC->IsInPartyAsMember());

	// Cleanup any previous state
	MatchmakingSteps.Empty();
	MatchmakingStepIndex = 0;
	MatchmakingJoinCount = 0;
	MatchmakingJoinFailures = 0;
	bMatchmakingLAN = bForLAN;
	MatchmakingBlacklist.Empty();

	// Generate steps
	if (bIsPartyMemberOrJoining)
	{
		MatchmakingSteps.Empty();
	}
	else
	{
		BuildMatchmakingSearchSteps();
		check(MatchmakingSteps.Num() > 0);
	}

	// Verify integrity of MatchmakingSteps
	for (const FILLMatchmakingStep& Step : MatchmakingSteps)
	{
		check(Step.IsValid());
		if (Step.IsSearchStep())
		{
			check(Step.IsValidSearchStep());
		}
		else
		{
			check(Step.IsCooldownStep() || Step.IsInitializationStep() || Step.MatchmakingState == EILLMatchmakingState::TimeRestart);
		}
	}

	auto GetNumCooldownSteps = [this]() -> int32 { int32 Result = 0; for (const auto& Step: MatchmakingSteps) { if (Step.IsCooldownStep()) ++Result; }; return Result; };
	auto GetNumSearchingSteps = [this]() -> int32 { int32 Result = 0; for (const auto& Step: MatchmakingSteps) { if (Step.IsValidSearchStep()) ++Result; }; return Result; };
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: NumSteps: %i (%i searching, %i cooldowns)"), ANSI_TO_TCHAR(__FUNCTION__), MatchmakingSteps.Num(), GetNumSearchingSteps(), GetNumCooldownSteps());

	if (!bIsPartyMemberOrJoining)
	{
		// Listen for suspend
		MatchmakingApplicationEnterBackgroundDelegateHandle = FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this, &ThisClass::OnMatchmakingApplicationEnterBackground);
	}

	// Notify party members
	if (OSC->IsInPartyAsLeader())
	{
		check(!bForLAN); // This wouldn't make sense..
		OSC->GetPartyHostBeaconObject()->NotifyPartyMatchmakingStarted();
	}

	// Track when we started
	MatchmakingStartRealTime = GetWorld()->GetRealTimeSeconds();

	// Start processing MatchmakingSteps
	if (!bIsPartyMemberOrJoining && MatchmakingSteps.Num() > 0)
		ContinueMatchmaking(false);

	return true;
}

bool UILLOnlineMatchmakingClient::CancelMatchmaking()
{
	if (!IsMatchmaking())
	{
		return false;
	}

	UILLOnlineSessionClient* OSC = GetOnlineSession();
	if (OSC->IsJoiningParty() || OSC->IsInPartyAsMember())
	{
		return false;
	}
	
	UWorld* World = GetWorld();

	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: MatchmakingState: %s"), ANSI_TO_TCHAR(__FUNCTION__), ToString(MatchmakingState));

	bool bFinishedCanceling = false;
	switch (MatchmakingState)
	{
	case EILLMatchmakingState::NotRunning:
		// Do nothing, but check our state
		if (!ensure(!TimerHandle_CancelMatchmaking.IsValid()))
		{
			World->GetTimerManager().ClearTimer(TimerHandle_CancelMatchmaking);
			TimerHandle_CancelMatchmaking.Invalidate();
		}
		if (!ensure(!TimerHandle_ContinueMatchmaking.IsValid()))
		{
			World->GetTimerManager().ClearTimer(TimerHandle_ContinueMatchmaking);
			TimerHandle_ContinueMatchmaking.Invalidate();
		}
		break;

	case EILLMatchmakingState::SearchingHTTP:
#if MATCHMAKING_SUPPORTS_HTTP_API
		if (MatchmakingDescribeHTTPRequest)
		{
			MatchmakingDescribeHTTPRequest->CancelRequest();
		}
		if (TimerHandle_SendMatchmakingDescribe.IsValid())
		{
			World->GetTimerManager().ClearTimer(TimerHandle_SendMatchmakingDescribe);
			TimerHandle_SendMatchmakingDescribe.Invalidate();
		}

		SendMatchmakingCancelRequest(true);
#else
		check(0);
#endif
		break;

	case EILLMatchmakingState::SearchingDedicated:
	case EILLMatchmakingState::SearchingSessions:
	case EILLMatchmakingState::SearchingLAN:
		{
			IOnlineSessionPtr Sessions = GetSessionInt();

#if MATCHMAKING_USES_MATCHMAKING_API
			// If we are already in a game session, then a match was found and we are performing QoS
			// Wait for that join to fail instead of attempting to leave while joining, otherwise a party member may finish joining before they receive the cancellation notification and bad shit happens
			FNamedOnlineSession* ExistingGameSession = Sessions->GetNamedSession(NAME_GameSession);
			if (ExistingGameSession && ExistingGameSession->SessionState != EOnlineSessionState::Destroying)
			{
				SetMatchmakingState(EILLMatchmakingState::Canceling);
				UE_LOG(LogOnlineGame, Log, TEXT("%s: Game session exists, waiting for join completion or failure"), ANSI_TO_TCHAR(__FUNCTION__));
				break;
			}
#endif

			if (MatchmakingSearchDelegateHandle.IsValid())
			{

#if MATCHMAKING_USES_MATCHMAKING_API
				// Update our matchmaking state BEFORE canceling, because the OSS may fire the delegate in the CancelMatchmaking call
				SetMatchmakingState(EILLMatchmakingState::Canceling);

				// No longer listen for matchmaking completion
				// Do this prior to calling CancelMatchmaking because CancelMatchmaking may fire it
				check(MatchmakingSearchDelegateHandle.IsValid());
				Sessions->ClearOnMatchmakingCompleteDelegate_Handle(MatchmakingSearchDelegateHandle);

				// Cancel matchmaking, waiting for a delegate to tell us we are done canceling
				UILLLocalPlayer* FirstLocalPlayer = Cast<UILLLocalPlayer>(World->GetFirstLocalPlayerFromController());
				MatchmakingCancelDelegateHandle = Sessions->AddOnCancelMatchmakingCompleteDelegate_Handle(MatchmakingCancelDelegate);
				Sessions->CancelMatchmaking(FirstLocalPlayer->GetControllerId(), OSC->IsInPartyAsLeader() ? NAME_PartySession : NAME_GameSession); // NOTE: Always returns true as far as I can tell!
#elif MATCHMAKING_USES_FINDSESSIONS_API
				if (Sessions->CancelFindSessions())
				{
					// Ensure we don't retry matchmaking again and cancel
					if (MatchmakingSearchDelegateHandle.IsValid())
					{
						Sessions->ClearOnFindSessionsCompleteDelegate_Handle(MatchmakingSearchDelegateHandle);
					}
					bFinishedCanceling = true;
				}
				else
				{
					// We are waiting for search results
					// Flag pending cancellation, so that it will cancel when the search completes
					SetMatchmakingState(EILLMatchmakingState::Canceling);
				}
#else
# error
#endif
			}
			else
			{
				// Not actually searching?
				bFinishedCanceling = true;
			}
		}
		break;

	case EILLMatchmakingState::Canceling:
	case EILLMatchmakingState::Canceled:
		// Wait until the current session search or join completes
		UE_LOG(LogOnlineGame, Log, TEXT("%s: %s state already pending!"), ANSI_TO_TCHAR(__FUNCTION__), ToString(MatchmakingState));
		break;

	case EILLMatchmakingState::Joining:
		// Flag for cancellation if the join fails
		SetMatchmakingState(EILLMatchmakingState::Canceling);
		break;

	case EILLMatchmakingState::Initializing:
		// Wait for the request response to finish canceling
		SetMatchmakingState(EILLMatchmakingState::Canceling);
		break;
	case EILLMatchmakingState::Cooldown:
		// Ensure we don't retry matchmaking again and cancel
		bFinishedCanceling = true;
		break;

	default:
		check(0);
		break;
	}

	if (bFinishedCanceling)
	{
		World->GetTimerManager().ClearTimer(TimerHandle_ContinueMatchmaking);
		MatchmakingCancelingComplete();
	}

#if MATCHMAKING_USES_MATCHMAKING_API
	// Always flush this on cancellation, so stale instance replacement data is not used way too late
	PendingMatchmakingPlayerSessions.Empty();
#endif

	return true;
}

bool UILLOnlineMatchmakingClient::SupportsRegionalizedMatchmaking() const
{
	return (MATCHMAKING_SUPPORTS_HTTP_API != 0) ? true : false;
}

FILLMatchmakingStep UILLOnlineMatchmakingClient::GetCurrentMatchmakingStep() const
{
	return MatchmakingSteps.IsValidIndex(MatchmakingStepIndex) ? MatchmakingSteps[MatchmakingStepIndex] : FILLMatchmakingStep();
}

void UILLOnlineMatchmakingClient::BuildMatchmakingSearchSteps()
{
	if (bMatchmakingLAN)
	{
		// LAN simply searches a handful of times before finishing
		const int32 LAN_SEARCH_ATTEMPTS = 5;
		for (int32 AttemptIndex = 0; AttemptIndex < LAN_SEARCH_ATTEMPTS; ++AttemptIndex)
		{
			MatchmakingSteps.Add(FILLMatchmakingStep(EILLMatchmakingState::SearchingLAN));
		}
		return;
	}

	// Always push the Initialize step first for ONLINE searches
	MatchmakingSteps.Add(FILLMatchmakingStep(EILLMatchmakingState::Initializing));

	// Determine our "player size range", handling our party size
	// Right now we grab the player limit from whatever GameSession we have currently
	UWorld* World = GetWorld();
	UILLOnlineSessionClient* OSC = GetOnlineSession();
	AILLGameSession* GameSession = Cast<AILLGameSession>(World->GetAuthGameMode()->GameSession);
	const int32 PartySize = OSC->GetPartySize();
	const int32 MaxPlayers = GameSession->GetMaxPlayers();
	const int32 LargestGameSize = MaxPlayers - PartySize;

	// Now plot this out into MatchmakingSteps
	// Monitor your step count to keep this to a reasonable amount of time
	// Try and end with a cooldown so that there's a buffer between the search completion and any cancellation/failure or game creation, also gives the player's connection a chance to relax so pings don't continue to climb

#if MATCHMAKING_SUPPORTS_HTTP_API
	{
		const int32 RestartIndex = MatchmakingSteps.Num();

		// HTTP matchmaking should be attempted first since it should ideally be the most reliable
		MatchmakingSteps.Add(FILLMatchmakingStep(EILLMatchmakingState::SearchingHTTP).SetPlayerSlots(PartySize));

		// Retry if it fails too quickly
		MatchmakingSteps.Add(FILLMatchmakingStep(EILLMatchmakingState::TimeRestart).SetTimeRestart(MatchmakingHTTPMinimumTime, RestartIndex));
	}
#endif

#if MATCHMAKING_USES_MATCHMAKING_API
	if (LargestGameSize > 0)
	{
		const int32 RestartIndex = MatchmakingSteps.Num();

		// Matching tickets search path
		// Obviously one of the simplest, since it just waits out the XBOXONE_QUICKPLAY_MATCHING_TIMEOUT
		MatchmakingSteps.Add(FILLMatchmakingStep(EILLMatchmakingState::SearchingSessions));

		// Retry if it fails too quickly (finds a session that was open to matchmaking but that changed while QoS checks were running)
		MatchmakingSteps.Add(FILLMatchmakingStep(EILLMatchmakingState::TimeRestart).SetTimeRestart(60.f, RestartIndex));
	}
#elif MATCHMAKING_USES_FINDSESSIONS_API
	/**
	 * Shitty matchmaking "scouring" path that will hopefully be deprecated in the near future.
	 * The primary goal with these steps should be to scope-down the results as much as possible, server-side as often as possible.
	 * Getting back too many results slows this process down as the OSS has to ping each result it gets back.
	 */

	/**
	 * Ping limits serve to reject results which are too laggy for use.
	 * Take care not to make these too low, or players on crappier connections may waste a bunch of time pinging results they will just ignore.
	 * Also keep in mind that we search in slices, so ignoring a given slice for not yielding good results is not always bad.
	 */
	typedef TPairInitializer<int32, float> TPingLimitInitializer;
	TArray<TPair<int32, float>> PingLimits;
# if PLATFORM_DESKTOP
	// Add an additional ping pass on desktop since QoS times are sub-second, and to spend more time looking for a better ping server than not
	PingLimits.Add(TPingLimitInitializer(100, 30.f));
# endif
	PingLimits.Add(TPingLimitInitializer(150, 45.f));
	PingLimits.Add(TPingLimitInitializer(200, 60.f));
# if PLATFORM_PS4
	PingLimits.Add(TPingLimitInitializer(250, 75.f));
	PingLimits.Add(TPingLimitInitializer(300, 90.f));
# endif

# if PLATFORM_DESKTOP
	/**
	 * Dedicated and session search passes will iterate through PlayerRequirements one at a time, with the intent of scoping down the number of results we get back server-side based on player count.
	 * Servers that are full and empty (which are the majority of servers) will automatically be ruled out by this. Big optimization.
	 * Serves as a client-side and server-side optimization, throttling the number of results returned, as we walk from the smallest available game to the largest.
	 */
	TArray<int32> PlayerRequirements;
	for (int32 MinPlayers = 1; MinPlayers <= LargestGameSize; ++MinPlayers)
		PlayerRequirements.Add(MinPlayers);

	// Skip Steam dedicated server searches when HTTP matchmaking is enabled
	// Even though there's the bypass path (@see PerformMatchmakingBypass)
#  if !MATCHMAKING_SUPPORTS_HTTP_API
	// PC matchmaking code path
	// Only proceed if there are any PlayerRequirements, otherwise a full party will just add a few cool downs
	if (PlayerRequirements.Num() > 0)
	{
		/**
		 * Iterate through ResultLimits, so that we expand our radius after a PingLimits pass.
		 * Values that are too low may restrict the number of servers received from Steam, which can give us the same crappy server(s) over and over.
		 * Values that are too high will make this whole process take longer, as the OSS is pinging more servers/sessions before evaluating them.
		 * The amount of time it takes to search and get responses from 4 servers seems to be roughly the same as requesting 1 server, probably due to the wait time for server listings from Steam.
		 */
		TArray<int32> ResultLimits;
		ResultLimits.Add(1);
		ResultLimits.Add(2);
		ResultLimits.Add(3);

		// Ping limiting
		for (int32 PingIndex = 0; PingIndex < PingLimits.Num(); ++PingIndex)
		{
			const int32 RestartIndex = MatchmakingSteps.Num();

			// Player count slicing
			for (int32 MaxResults : ResultLimits)
			{
				for (int32 NumPlayers : PlayerRequirements)
					MatchmakingSteps.Add(FILLMatchmakingStep(EILLMatchmakingState::SearchingDedicated).SetMaxResults(MaxResults).SetNumPlayers(NumPlayers).SetPingLimit(PingLimits[PingIndex].Key));
			}

			// Keep retrying this phase until the matchmaking time is reached
			MatchmakingSteps.Add(FILLMatchmakingStep(EILLMatchmakingState::TimeRestart).SetTimeRestart(PingLimits[PingIndex].Value, RestartIndex));

			// Cooldown once after each phase
			MatchmakingSteps.Add(FILLMatchmakingStep(EILLMatchmakingState::Cooldown));
		}
	}

	// Special-case for PC: Search for an empty dedicated server to occupy, skipping the last of the PingLimits
	const int32 SteamOccupyDedicatedAttempts = 2;
	for (int32 Attempt = 0; Attempt < SteamOccupyDedicatedAttempts; ++Attempt)
	{
		for (int32 PingIndex = 0; PingIndex < PingLimits.Num()-1; ++PingIndex)
			MatchmakingSteps.Add(FILLMatchmakingStep(EILLMatchmakingState::SearchingDedicated).SetMaxResults(1).SetNumPlayers(0).SetPingLimit(PingLimits[PingIndex].Key));
	}
#  endif // !MATCHMAKING_SUPPORTS_HTTP_API

	{
		// Since we perform dedicated server searches on PC, to bias towards those, only P2P search every now and then, and definitely after searching for an empty dedicated server to occupy
		// We do not get pings back for SteamP2P searches right now (@see UILLOnlineMatchmakingClient::OnMatchmakingSearchCompleted) so do not filter by that, supposedly we get them back with the best first from Steam in that case
		const int32 RestartIndex = MatchmakingSteps.Num();
#  ifdef SEARCH_PLAYERSLOTS
		const int32 SteamP2PMatchmakingSearchAttempts = 3;
		for (int32 Attempt = 0; Attempt < SteamP2PMatchmakingSearchAttempts; ++Attempt)
			MatchmakingSteps.Add(FILLMatchmakingStep(EILLMatchmakingState::SearchingSessions).SetMaxResults(1).SetPlayerSlots(PartySize));
#  else
		for (int32 NumPlayers : PlayerRequirements)
			MatchmakingSteps.Add(FILLMatchmakingStep(EILLMatchmakingState::SearchingSessions).SetMaxResults(1).SetNumPlayers(NumPlayers));
#  endif
		MatchmakingSteps.Add(FILLMatchmakingStep(EILLMatchmakingState::TimeRestart).SetTimeRestart(90.f, RestartIndex));
	}
# elif PLATFORM_PS4
	// Console matchmaking code path
	// This always limits to 1 max result per pass, otherwise it would perform QoS and NAT punching on multiple sessions, wasting a lot of time due to how long that takes
	if (LargestGameSize > 0)
	{
		// Ping limiting
		for (int32 PingIndex = 0; PingIndex < PingLimits.Num(); ++PingIndex)
		{
			const int32 RestartIndex = MatchmakingSteps.Num();

			// Player count scoping
			MatchmakingSteps.Add(FILLMatchmakingStep(EILLMatchmakingState::SearchingSessions).SetMaxResults(1).SetMaxPlayers(LargestGameSize).SetPingLimit(PingLimits[PingIndex].Key));

			// Keep retrying this phase until the matchmaking time is reached
			MatchmakingSteps.Add(FILLMatchmakingStep(EILLMatchmakingState::TimeRestart).SetTimeRestart(PingLimits[PingIndex].Value, RestartIndex));

			// Cooldown once after each phase
			MatchmakingSteps.Add(FILLMatchmakingStep(EILLMatchmakingState::Cooldown));
		}
	}
# else
#  error
# endif
#else
# error
#endif
}

#if MATCHMAKING_USES_MATCHMAKING_API || MATCHMAKING_USES_FINDSESSIONS_API
void UILLOnlineMatchmakingClient::BuildMatchmakingSearchObject()
{
	const FILLMatchmakingStep& CurrentStep = GetCurrentMatchmakingStep();

	// Clear our existing search
	if (MatchmakingSearchObject.IsValid())
	{
		MatchmakingSearchObject.Reset();
	}

	MatchmakingSearchObject = MakeShareable(new FILLOnlineSessionSearch);
	MatchmakingSearchObject->bIsLanQuery = (CurrentStep.MatchmakingState == EILLMatchmakingState::SearchingLAN);
#if MATCHMAKING_USES_FINDSESSIONS_API
	MatchmakingSearchObject->MaxSearchResults = CurrentStep.MaxResults;
#else
	MatchmakingSearchObject->MaxSearchResults = 1; // SmartMatch only fetches one at a time
#endif
	
#if PLATFORM_XBOXONE
	MatchmakingSearchObject->QuerySettings.Set(SEARCH_KEYWORDS, FString::Printf(XBOXONE_QUICKPLAY_KEYWORDS_FORMAT, ILLBUILD_BUILD_NUMBER), EOnlineComparisonOp::Equals);
#endif

#if MATCHMAKING_USES_MATCHMAKING_API
# if PLATFORM_XBOXONE
	MatchmakingSearchObject->QuerySettings.Set(SETTING_GAMEMODE, XBOXONE_QUICKPLAY_SESSIONTEMPLATE, EOnlineComparisonOp::Equals);
# endif
#elif MATCHMAKING_USES_FINDSESSIONS_API
	MatchmakingSearchObject->QuerySettings.Set(SETTING_BUILDNUMBER, ILLBUILD_BUILD_NUMBER, EOnlineComparisonOp::Equals);
	MatchmakingSearchObject->QuerySettings.Set(SETTING_OPENFORMM, 1, EOnlineComparisonOp::Equals);
	MatchmakingSearchObject->QuerySettings.Set(SETTING_PRIORITY, (LastAggregateMatchmakingStatus == EILLBackendMatchmakingStatusType::Ok) ? SESSION_PRIORITY_OK : SESSION_PRIORITY_LOW, EOnlineComparisonOp::Equals);

# if PLATFORM_PS4
	// Special case for PS4 searches: Look for matches with fewer than MaxPlayers
	MatchmakingSearchObject->QuerySettings.Set(SETTING_PLAYERCOUNT, CurrentStep.MaxPlayers, EOnlineComparisonOp::LessThanEquals);
# else
#  ifdef SEARCH_PLAYERSLOTS
	if (CurrentStep.MatchmakingState == EILLMatchmakingState::SearchingSessions)
	{
		// Special case for SteamP2P searches: Use SEARCH_PLAYERSLOTS instead
		MatchmakingSearchObject->QuerySettings.Set(SEARCH_PLAYERSLOTS, CurrentStep.PlayerSlots, EOnlineComparisonOp::Equals);
	}
	else
#  endif
	{
		// <=0 means no requirement
		// ==0 means empty only
		// >0 means this many people or fewer
		if (CurrentStep.NumPlayers >= 0)
			MatchmakingSearchObject->QuerySettings.Set(SETTING_PLAYERCOUNT, CurrentStep.NumPlayers, EOnlineComparisonOp::Equals);
	}
# endif
#endif // MATCHMAKING_USES_FINDSESSIONS_API

	if (CurrentStep.MatchmakingState == EILLMatchmakingState::SearchingDedicated)
	{
#if PLATFORM_DESKTOP
		// Only search for VAC-secured servers
		MatchmakingSearchObject->QuerySettings.Set(SEARCH_SECURE_SERVERS_ONLY, 1, EOnlineComparisonOp::Equals);

		// Optimizations that only affect the Steam OSS dedicated server searches
		MatchmakingSearchObject->QuerySettings.Set(SEARCH_DEDICATED_ONLY, 1, EOnlineComparisonOp::Equals);

# if MATCHMAKING_USES_FINDSESSIONS_API
		// Ignore or require non-empty servers based on the NumPlayers step setting
		MatchmakingSearchObject->QuerySettings.Set(SEARCH_EMPTY_SERVERS_ONLY, (CurrentStep.NumPlayers == 0) ? 1 : 0, EOnlineComparisonOp::Equals);
		MatchmakingSearchObject->QuerySettings.Set(SEARCH_NONEMPTY_SERVERS_ONLY, (CurrentStep.NumPlayers > 0) ? 1 : 0, EOnlineComparisonOp::Equals);
# endif // MATCHMAKING_USES_FINDSESSIONS_API

		// Always ignore full servers
		MatchmakingSearchObject->QuerySettings.Set(SEARCH_NONFULL_SERVERS_ONLY, 1, EOnlineComparisonOp::Equals);
#else
		check(0);
#endif
	}
	else if (CurrentStep.MatchmakingState == EILLMatchmakingState::SearchingSessions)
	{
		// This only affects the Steam OSS: Search for P2P sessions
		MatchmakingSearchObject->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
	}
}
#endif // MATCHMAKING_USES_MATCHMAKING_API || MATCHMAKING_USES_FINDSESSIONS_API

void UILLOnlineMatchmakingClient::ContinueMatchmaking(bool bNextStep/* = true*/)
{
	UWorld* World = GetWorld();

	// Invalidate ANY timer to retry this
	World->GetTimerManager().ClearTimer(TimerHandle_ContinueMatchmaking);
	TimerHandle_ContinueMatchmaking.Invalidate();

	// Check for cancellation
	if (MatchmakingState == EILLMatchmakingState::Canceling)
	{
		UE_LOG(LogOnlineGame, Verbose, TEXT("%s: Cancelled, ending matchmaking"), ANSI_TO_TCHAR(__FUNCTION__));
		MatchmakingCancelingComplete();
		return;
	}

	// Move to the next step if desired
	if (bNextStep)
	{
		// Check if we have hit our join failure limit
		const int32 MaxFailures = PLATFORM_DESKTOP ? MatchmakingMaxDesktopJoinFailures : MatchmakingMaxConsoleJoinFailures;
		if (MaxFailures > 0 && MatchmakingJoinFailures >= MaxFailures)
		{
			MatchmakingFinished();
			return;
		}

		// Attempt to progress
		if (MatchmakingSteps.IsValidIndex(MatchmakingStepIndex+1))
		{
			++MatchmakingStepIndex;

			// Handle matchmaking status, skipping dedicated servers or any searching
			// CLEANUP: pjackson: This would be a lot better in BuildMatchmakingSearchStep, if Initializing was not a state and instead a process ran before it
			if (LastAggregateMatchmakingStatus != EILLBackendMatchmakingStatusType::Ok)
			{
				check(LastAggregateMatchmakingStatus == EILLBackendMatchmakingStatusType::LowPriority);
				if(MatchmakingSteps[MatchmakingStepIndex].MatchmakingState == EILLMatchmakingState::SearchingHTTP
				|| MatchmakingSteps[MatchmakingStepIndex].MatchmakingState == EILLMatchmakingState::SearchingDedicated)
				{
					do
					{
						check(MatchmakingSteps.IsValidIndex(MatchmakingStepIndex+1));
						++MatchmakingStepIndex;
					} while (MatchmakingSteps[MatchmakingStepIndex].MatchmakingState == EILLMatchmakingState::SearchingHTTP
						  || MatchmakingSteps[MatchmakingStepIndex].MatchmakingState == EILLMatchmakingState::SearchingDedicated
						  || MatchmakingSteps[MatchmakingStepIndex].MatchmakingState == EILLMatchmakingState::TimeRestart);
				}
			}

#if MATCHMAKING_SUPPORTS_HTTP_API
			// Handle MatchmakingEndpoints being empty correctly... sigh
			if (MatchmakingSteps[MatchmakingStepIndex].MatchmakingState == EILLMatchmakingState::SearchingHTTP
			&& MatchmakingEndpoints.Num() <= 0)
			{
				UE_LOG(LogOnlineGame, Warning, TEXT("%s: No endpoints received, stripping HTTP matchmaking steps!"), ANSI_TO_TCHAR(__FUNCTION__));
				do
				{
					check(MatchmakingSteps.IsValidIndex(MatchmakingStepIndex+1));
					++MatchmakingStepIndex;
				} while (MatchmakingSteps[MatchmakingStepIndex].MatchmakingState == EILLMatchmakingState::SearchingHTTP
					  || MatchmakingSteps[MatchmakingStepIndex].MatchmakingState == EILLMatchmakingState::TimeRestart);
			}
#endif
		}
		else
		{
			// We have ran out of steps
			MatchmakingFinished();
			return;
		}
	}

	// Verify there is a step
	if (!MatchmakingSteps.IsValidIndex(MatchmakingStepIndex))
	{
		MatchmakingFinished();
		return;
	}

	// Handle TimeRestart
	if (MatchmakingSteps[MatchmakingStepIndex].MatchmakingState == EILLMatchmakingState::TimeRestart)
	{
		const float MatchmakingTime = GetWorld()->GetRealTimeSeconds() - MatchmakingStartRealTime;
		if (MatchmakingTime >= MatchmakingSteps[MatchmakingStepIndex].MinTime)
		{
			// Skip the restart, we have hit the time requirement
			if (MatchmakingSteps.IsValidIndex(MatchmakingStepIndex+1))
			{
				++MatchmakingStepIndex;
			}
			else
			{
				// Looks like we're done
				MatchmakingFinished();
				return;
			}
		}
		else
		{
			UE_LOG(LogOnlineGame, Verbose, TEXT("%s: Restarting matchmaking due to MatchmakingTime (%3.2f) being less than MinTime (%3.2f)"), ANSI_TO_TCHAR(__FUNCTION__), MatchmakingTime, MatchmakingSteps[MatchmakingStepIndex].MinTime);

			// Correct the RestartIndex, skipping Initializing steps
			int32 CorrectedRestartIndex = MatchmakingSteps[MatchmakingStepIndex].RestartIndex;
			while (CorrectedRestartIndex < 0 || MatchmakingSteps[CorrectedRestartIndex].MatchmakingState == EILLMatchmakingState::Initializing)
				++CorrectedRestartIndex;

			// Store this as the MatchmakingStepIndex so we resume from there
			MatchmakingStepIndex = CorrectedRestartIndex;
		}
	}

	// Should not hit this while joining a game!
	if (!ensure(MatchmakingState != EILLMatchmakingState::Joining))
	{
		SetMatchmakingState(EILLMatchmakingState::NotRunning);
		return;
	}

	// Check if we can process this step
	const FILLMatchmakingStep& CurrentStep = GetCurrentMatchmakingStep();
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: Step: %s"), ANSI_TO_TCHAR(__FUNCTION__), *CurrentStep.ToDebugString());
	if (!CurrentStep.IsValid())
	{
		// Stop matchmaking, invalid state
		MatchmakingCanceledComplete();
		return;
	}

	UILLLocalPlayer* FirstLocalPlayer = CastChecked<UILLLocalPlayer>(World->GetFirstLocalPlayerFromController());

	// Check if we should be searching
	bool bSearching = CurrentStep.IsValidSearchStep();
	if (!bSearching)
	{
		switch (CurrentStep.MatchmakingState)
		{
		case EILLMatchmakingState::Initializing:
			{
				// Send an arbiter heartbeat (or wait for an existing pending one to finish)
				if (FirstLocalPlayer->GetBackendPlayer() && FirstLocalPlayer->GetBackendPlayer()->IsLoggedIn())
				{
					// Wait for the heartbeat to complete before continuing matchmaking
					FirstLocalPlayer->GetBackendPlayer()->OnBackendPlayerArbiterHeartbeatComplete().AddUObject(this, &ThisClass::OnMatchmakingBackendPlayerHeartbeat);
					FirstLocalPlayer->GetBackendPlayer()->SendArbiterHeartbeat();
					SetMatchmakingState(EILLMatchmakingState::Initializing);
				}
				else
				{
					// Not logged in, fake a failure
					OnMatchmakingBackendPlayerHeartbeat(nullptr, EILLBackendArbiterHeartbeatResult::UnknownError);
				}
			}
			return; // Do not search or cooldown!

		case EILLMatchmakingState::Cooldown:
			break;

		default:
			UE_LOG(LogOnlineGame, Warning, TEXT("%s: Unhandled state %s hit!"), ANSI_TO_TCHAR(__FUNCTION__), ToString(CurrentStep.MatchmakingState));
			check(0);
			break;
		}
	}

	if (bSearching)
	{
		if (CurrentStep.MatchmakingState == EILLMatchmakingState::SearchingHTTP)
		{
#if MATCHMAKING_SUPPORTS_HTTP_API
			if (PendingMatchmakingPlayerSessions.Num() > 0)
			{
				if (GetOnlineSession()->BeginRequestPartyLobbySession(PendingMatchmakingPlayerSessions, MatchmakingLobbyBeaconSessionReceivedDelegate, MatchmakingLobbyBeaconSessionRequestCompleteDelegate))
				{
					// Update matchmaking state and do not continue, we're going to attempt travel
					SetMatchmakingState(EILLMatchmakingState::Joining);
					PendingMatchmakingPlayerSessions.Empty();
					return;
				}

				// Invalid PendingMatchmakingPlayerSessions if we hit here, fall through to starting a new queue
				PendingMatchmakingPlayerSessions.Empty();
			}

			// Now start searching
			bSearching = SendMatchmakingQueueRequest();
#endif
		}
		else
		{
			// Build MatchmakingSearchObject
			BuildMatchmakingSearchObject();

			// Now start searching
			IOnlineSessionPtr Sessions = GetSessionInt();
			if (Sessions.IsValid())
			{
				// Now start searching
#if MATCHMAKING_USES_MATCHMAKING_API
				TArray<TSharedRef<const FUniqueNetId>> LocalPlayers;
				LocalPlayers.Add(FirstLocalPlayer->GetCachedUniqueNetId()->AsShared());

				const int32 MaxPlayers = World->GetAuthGameMode()->GetGameSessionClass()->GetDefaultObject<AILLGameSession>()->MaxPlayers;
				FILLGameSessionSettings NewSessionSettings(false, MaxPlayers, true, false);
# if PLATFORM_XBOXONE
				NewSessionSettings.Set(SETTING_MATCHING_TIMEOUT, XBOXONE_QUICKPLAY_MATCHING_TIMEOUT, EOnlineDataAdvertisementType::DontAdvertise);
				NewSessionSettings.Set(SETTING_SESSION_TEMPLATE_NAME, XBOXONE_QUICKPLAY_SESSIONTEMPLATE, EOnlineDataAdvertisementType::DontAdvertise);
				NewSessionSettings.Set(SEARCH_KEYWORDS, FString::Printf(XBOXONE_QUICKPLAY_KEYWORDS_FORMAT, ILLBUILD_BUILD_NUMBER), EOnlineDataAdvertisementType::ViaOnlineService);
				NewSessionSettings.Set(SETTING_MATCHING_ATTRIBUTES, FString::Printf(TEXT("{ \"BuildNumber\": %i }"), ILLBUILD_BUILD_NUMBER), EOnlineDataAdvertisementType::DontAdvertise);
				if (LastAggregateMatchmakingStatus == EILLBackendMatchmakingStatusType::Ok)
					NewSessionSettings.Set(SEARCH_XBOX_LIVE_HOPPER_NAME, XBOXONE_QUICKPLAY_HOPPER_STANDARD, EOnlineDataAdvertisementType::DontAdvertise);
				else
					NewSessionSettings.Set(SEARCH_XBOX_LIVE_HOPPER_NAME, XBOXONE_QUICKPLAY_HOPPER_LOWPRIORITY, EOnlineDataAdvertisementType::DontAdvertise);
# endif

				TSharedRef<FOnlineSessionSearch> SearchSettings = MatchmakingSearchObject.ToSharedRef();
# if PLATFORM_XBOXONE
				SearchSettings->QuerySettings.Set(SETTING_MATCHING_PRESERVE_SESSION, false, EOnlineComparisonOp::Equals);
				SearchSettings->QuerySettings.Set(SETTING_SESSION_TEMPLATE_NAME, XBOXONE_QUICKPLAY_SESSIONTEMPLATE, EOnlineComparisonOp::Equals);
				SearchSettings->QuerySettings.Set(SEARCH_XBOX_LIVE_SESSION_TEMPLATE_NAME, XBOXONE_QUICKPLAY_SESSIONTEMPLATE, EOnlineComparisonOp::Equals);
				if (LastAggregateMatchmakingStatus == EILLBackendMatchmakingStatusType::Ok)
					SearchSettings->QuerySettings.Set(SEARCH_XBOX_LIVE_HOPPER_NAME, XBOXONE_QUICKPLAY_HOPPER_STANDARD, EOnlineComparisonOp::Equals);
				else
					SearchSettings->QuerySettings.Set(SEARCH_XBOX_LIVE_HOPPER_NAME, XBOXONE_QUICKPLAY_HOPPER_LOWPRIORITY, EOnlineComparisonOp::Equals);
				SearchSettings->TimeoutInSeconds = XBOXONE_QUICKPLAY_MATCHING_TIMEOUT;
# endif

				if (!MatchmakingSearchDelegateHandle.IsValid())
					MatchmakingSearchDelegateHandle = Sessions->AddOnMatchmakingCompleteDelegate_Handle(MatchmakingSearchDelegate);
				UILLOnlineSessionClient* OSC = GetOnlineSession();
				if (Sessions->StartMatchmaking(LocalPlayers, OSC->IsInPartyAsLeader() ? NAME_PartySession : NAME_GameSession, NewSessionSettings, SearchSettings))
				{
					bSearching = true;
				}
				else
				{
					bSearching = false;

					// Attempt to cancel our previous search now
					check(MatchmakingSearchDelegateHandle.IsValid());
					Sessions->ClearOnMatchmakingCompleteDelegate_Handle(MatchmakingSearchDelegateHandle);
				}
#elif MATCHMAKING_USES_FINDSESSIONS_API
				MatchmakingSearchDelegateHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(MatchmakingSearchDelegate);
				if (Sessions->FindSessions(FirstLocalPlayer->GetControllerId(), MatchmakingSearchObject.ToSharedRef()))
				{
					bSearching = true;
				}
				else
				{
					bSearching = false;

					// Attempt to cancel our previous search now
					if (MatchmakingSearchDelegateHandle.IsValid())
					{
						Sessions->ClearOnFindSessionsCompleteDelegate_Handle(MatchmakingSearchDelegateHandle);
					}
					if (!Sessions->CancelFindSessions())
					{
						// Couldn't cancel!
					}
				}
#else
# error
#endif
			}
		}
	}

	if (bSearching)
	{
		SetMatchmakingState(CurrentStep.MatchmakingState);
	}
	else
	{
		PerformMatchmakingCooldown();
	}
}

const FOnlineSessionSearchResult* UILLOnlineMatchmakingClient::FindBestSessionResult(const TArray<FOnlineSessionSearchResult>& FromList) const
{
	if (FromList.Num() <= 0)
	{
		return nullptr;
	}

	IOnlineSessionPtr Sessions = GetSessionInt();
	FNamedOnlineSession* CurrentSession = Sessions.IsValid() ? Sessions->GetNamedSession(NAME_GameSession) : nullptr;

	UILLOnlineSessionClient* OSC = GetOnlineSession();
	const int32 PartySize = OSC->GetPartySize();

	// Find the largest game with room
	int32 BestPing = INT_MAX;
	int BestNumPlayers = -1;
	const FOnlineSessionSearchResult* BestResult = nullptr;
	for (const FOnlineSessionSearchResult& Result : FromList)
	{
		FBlueprintSessionResult BPResult;
		BPResult.OnlineResult = Result;

		// Grab the active player count
		int32 NumPlayers = UILLGameOnlineBlueprintLibrary::GetNumPlayers(BPResult);
			
		// Deduct a player for "us" if this is the one we are currently on
		// This will make any session we are not on look bigger and so more valuable, and will allow this server to "have enough room" (below) to be returned as the best server if it still is
		if (CurrentSession && CurrentSession->OwningUserId.IsValid() && *CurrentSession->OwningUserId == *Result.Session.OwningUserId)
		{
			NumPlayers -= PartySize;
		}

		// Check for room!
		const int32 AvailableSlots = UILLGameOnlineBlueprintLibrary::GetMaxPlayers(BPResult) - NumPlayers;
		if (AvailableSlots < PartySize)
		{
			continue;
		}

		// Now compare player counts
		if (NumPlayers > BestNumPlayers)
		{
			// Larger match, always accept...
		}
		else if (NumPlayers == BestNumPlayers)
		{
			// Same-sized match, only accept if the ping is better
			if (Result.PingInMs >= BestPing)
				continue;
		}
		else if (NumPlayers > 0)
		{
			// Non-empty smaller sized match
			// Only accept if we have a noticeably better ping
			if (Result.PingInMs > BestPing-MatchmakingBetterPingThreshold)
				continue;
		}

		BestNumPlayers = NumPlayers;
		BestPing = Result.PingInMs;
		BestResult = &Result;
	}

	return BestResult;
}

void UILLOnlineMatchmakingClient::SetMatchmakingState(const EILLMatchmakingState NewState)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: NewState: %s"), ANSI_TO_TCHAR(__FUNCTION__), ToString(NewState));

	if (MatchmakingState == NewState)
	{
		return;
	}

	UILLOnlineSessionClient* OSC = GetOnlineSession();
	const bool bIsPartyMemberOrJoining = (OSC->IsJoiningParty() || OSC->IsInPartyAsMember());

	// Developer checks to ensure that we get states in the expected order
	if (!bIsPartyMemberOrJoining)
	{
		if (MatchmakingState == EILLMatchmakingState::NotRunning)
		{
			checkf(NewState != EILLMatchmakingState::Canceling, TEXT("Unexpected matchmaking state Canceling when NotRunning"));
			checkf(NewState != EILLMatchmakingState::Canceled, TEXT("Unexpected matchmaking state Canceled when NotRunning"));
		}
		else if (MatchmakingState == EILLMatchmakingState::Canceling)
		{
			checkf(NewState == EILLMatchmakingState::Canceled || NewState == EILLMatchmakingState::NotRunning, TEXT("Unexpected matchmaking state %s when Canceling"), ToString(NewState));
		}
		else if (MatchmakingState == EILLMatchmakingState::Canceled)
		{
			checkf(NewState == EILLMatchmakingState::NotRunning, TEXT("Unexpected matchmaking state %s when Canceled"), ToString(NewState));
		}
	}

	// Take the new state
	MatchmakingState = NewState;

	if (MatchmakingState == EILLMatchmakingState::NotRunning)
	{
		// Update time
		MatchmakingStartRealTime = -1.f;

		if (!bIsPartyMemberOrJoining)
		{
			// No longer listen for suspend
			FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(MatchmakingApplicationEnterBackgroundDelegateHandle);

			// Discard the arbitration handshake callback
			UWorld* World = GetWorld();
			if (UILLLocalPlayer* FirstLocalPlayer = Cast<UILLLocalPlayer>(World->GetFirstLocalPlayerFromController()))
			{
				if (FirstLocalPlayer->GetBackendPlayer())
					FirstLocalPlayer->GetBackendPlayer()->OnBackendPlayerArbiterHeartbeatComplete().RemoveAll(this);
			}
		}
	}

	// Update our party leader status to everyone
	if (OSC->IsInPartyAsLeader())
	{
		AILLPartyBeaconState* PartyBeaconState = OSC->GetPartyBeaconState();
		if (MatchmakingState == EILLMatchmakingState::NotRunning)
		{
			if (PartyBeaconState->GetLeaderStateName() == ILLPartyLeaderState::Matchmaking)
				PartyBeaconState->SetLeaderState(ILLPartyLeaderState::Idle);
		}
		else if (MatchmakingState == EILLMatchmakingState::Joining)
		{
			PartyBeaconState->SetLeaderState(ILLPartyLeaderState::JoiningGame);
		}
		else
		{
			PartyBeaconState->SetLeaderState(ILLPartyLeaderState::Matchmaking);
		}

		OSC->GetPartyHostBeaconObject()->NotifyPartyMatchmakingState(MatchmakingState);
	}

	// Broadcast the event
	MatchmakingStateChangeDelegate.Broadcast(MatchmakingState);
}

void UILLOnlineMatchmakingClient::OnMatchmakingApplicationEnterBackground()
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Force matchmaking to end now
	CancelMatchmaking();
}

void UILLOnlineMatchmakingClient::OnMatchmakingBackendPlayerHeartbeat(UILLBackendPlayer* BackendPlayer, EILLBackendArbiterHeartbeatResult HeartbeatResult)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: %i"), ANSI_TO_TCHAR(__FUNCTION__), static_cast<uint8>(HeartbeatResult));

	// Discard the arbitration handshake callback
	UWorld* World = GetWorld();
	if (UILLLocalPlayer* FirstLocalPlayer = Cast<UILLLocalPlayer>(World->GetFirstLocalPlayerFromController()))
	{
		if (FirstLocalPlayer->GetBackendPlayer())
			FirstLocalPlayer->GetBackendPlayer()->OnBackendPlayerArbiterHeartbeatComplete().RemoveAll(this);
	}

	if (HeartbeatResult == EILLBackendArbiterHeartbeatResult::Success)
	{
		if (MatchmakingState == EILLMatchmakingState::Canceling)
		{
			MatchmakingCancelingComplete();
		}
		else
		{
			// Request our matchmaking status before proceeding
			GetAggregateMatchmakingStatusDelegate().AddUObject(this, &ThisClass::OnMatchmakingPartyStatusResponse);
			RequestPartyMatchmakingStatus();
		}
	}
	else
	{
		// We will have already forced to sign out and disconnect
		// Force matchmaking to end
		SetMatchmakingState(EILLMatchmakingState::NotRunning);
	}
}

void UILLOnlineMatchmakingClient::OnMatchmakingPartyStatusResponse(EILLBackendMatchmakingStatusType AggregateStatus, float UnBanRealTimeSeconds)
{
	UWorld* World = GetWorld();
	if (AggregateStatus == EILLBackendMatchmakingStatusType::Banned)
	{
		GetOuterUILLGameInstance()->HandleLocalizedError(LOCTEXT("MachmakingBannedHeader", "Matchmaking denied!"), LOCTEXT("MatchmakingBannedMessage", "You are currently banned from matchmaking."), false);
		SetMatchmakingState(EILLMatchmakingState::NotRunning);
	}
	else
	{
		if (MatchmakingState == EILLMatchmakingState::Canceling)
		{
			MatchmakingCancelingComplete();
		}
		else
		{
#if MATCHMAKING_SUPPORTS_HTTP_API
			// Request a region listing to perform QoS against if we have not done so very recently
			if (MatchmakingEndpoints.Num() <= 0
			|| LastRegionRequestRealTime == 0.f || World->GetRealTimeSeconds()-LastRegionRequestRealTime >= HTTP_MATCHMAKING_QOS_MAXAGE)
			{
				SendMatchmakingRegionsRequest();
			}
			else
#endif
			{
				// Wait then process the next step
				auto ContinueDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::ContinueMatchmaking, true);
				World->GetTimerManager().SetTimer(TimerHandle_ContinueMatchmaking, ContinueDelegate, .1f, false);
			}
		}
	}
}

void UILLOnlineMatchmakingClient::PerformMatchmakingCooldown()
{
	const float RetryDelay = FMath::RandRange(MatchmakingRetryDelayMin, MatchmakingRetryDelayMax);
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: Continuing in %.1f seconds..."), ANSI_TO_TCHAR(__FUNCTION__), RetryDelay);

	// Wait then process the next step
	SetMatchmakingState(EILLMatchmakingState::Cooldown);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_ContinueMatchmaking, FTimerDelegate::CreateUObject(this, &ThisClass::ContinueMatchmaking, true), RetryDelay, false);
}

void UILLOnlineMatchmakingClient::MatchmakingCancelingComplete()
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	SetMatchmakingState(EILLMatchmakingState::Canceled);

	// Wait a second before wrapping up, in case OnMatchmakingSearchCompleted hits again (looking at you Steam LAN OSS)
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_CancelMatchmaking, this, &ThisClass::MatchmakingCanceledComplete, 1.f);
}

void UILLOnlineMatchmakingClient::MatchmakingCanceledComplete()
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	UWorld* World = GetWorld();
	IOnlineSessionPtr Sessions = GetSessionInt();
	UILLOnlineSessionClient* OSC = GetOnlineSession();

	// Cleanup our timer
	World->GetTimerManager().ClearTimer(TimerHandle_CancelMatchmaking);
	TimerHandle_CancelMatchmaking.Invalidate();

#if MATCHMAKING_SUPPORTS_HTTP_API
	// Cleanup requests
	MatchmakingRequestId.Empty();
	if (TimerHandle_SendMatchmakingDescribe.IsValid())
	{
		World->GetTimerManager().ClearTimer(TimerHandle_SendMatchmakingDescribe);
		TimerHandle_SendMatchmakingDescribe.Invalidate();
	}
#endif

	// Cleanup our matchmaking search delegate
	if (MatchmakingSearchDelegateHandle.IsValid())
	{
#if MATCHMAKING_USES_MATCHMAKING_API
		Sessions->ClearOnMatchmakingCompleteDelegate_Handle(MatchmakingSearchDelegateHandle);
#elif MATCHMAKING_USES_FINDSESSIONS_API
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(MatchmakingSearchDelegateHandle);
#else
#  error
#endif
	}

	// Sometimes SmartMatch will create a game session after we've canceled...
	if (FNamedOnlineSession* CurrentSession = Sessions->GetNamedSession(NAME_GameSession))
	{
		if (CurrentSession->SessionState != EOnlineSessionState::Destroying)
		{
			UE_LOG(LogOnlineGame, Warning, TEXT("%s: Dangling Game session detected, destroying it"), ANSI_TO_TCHAR(__FUNCTION__));
			OSC->DestroyExistingSession_Impl(DestroyGameSessionForMatchmakingCancelCompleteDelegateHandle, NAME_GameSession, DestroyGameSessionForMatchmakingCancelCompleteDelegate);
		}
	}

	// Cleanup any loading screen, in case we displayed one for joining
	GetOuterUILLGameInstance()->OnMatchmakingCanceledComplete();

	if (OSC->IsInPartyAsLeader())
	{
		// Notify our party members
		OSC->GetPartyHostBeaconObject()->NotifyPartyMatchmakingStopped();
	}

	// Cancel matchmaking
	SetMatchmakingState(EILLMatchmakingState::NotRunning);

	// Check if we need to flush an invite
	OSC->QueueFlushDeferredInvite();
}

void UILLOnlineMatchmakingClient::MatchmakingFinished()
{
	UILLOnlineSessionClient* OSC = GetOnlineSession();
	if (OSC->IsJoiningParty() || OSC->IsInPartyAsMember())
		return;

	UE_LOG(LogOnlineGame, Log, TEXT("%s: Duration: %3.2f, JoinCount: %i, JoinFail: %i"), ANSI_TO_TCHAR(__FUNCTION__), GetWorld()->GetRealTimeSeconds()-MatchmakingStartRealTime, MatchmakingJoinCount, MatchmakingJoinFailures);

	if (OSC->IsInPartyAsLeader())
	{
		// Update party leader status
		OSC->GetPartyBeaconState()->SetLeaderState(ILLPartyLeaderState::CreatingPublicMatch);
	}

	// Cancel matchmaking
	SetMatchmakingState(EILLMatchmakingState::NotRunning);

	// Load the server map in listen mode on this platform
	FString Options = TEXT("?listen");
	if (bMatchmakingLAN)
		Options += TEXT("?bIsLANMatch");
	UILLGameBlueprintLibrary::ShowLoadingAndTravel(this, GetOuterUILLGameInstance()->PickRandomQuickPlayMap(), true, Options);
}

#if MATCHMAKING_USES_MATCHMAKING_API
void UILLOnlineMatchmakingClient::OnMatchmakingSearchCompleted(FName InSessionName, bool bSuccess)
#elif MATCHMAKING_USES_FINDSESSIONS_API
void UILLOnlineMatchmakingClient::OnMatchmakingSearchCompleted(bool bSuccess)
#endif
{
	UILLOnlineSessionClient* OSC = GetOnlineSession();
#if MATCHMAKING_USES_MATCHMAKING_API
	if (!OSC->IsInPartyAsMember())
#endif
	{
		// State checks
		// Transition from Canceling to Canceled if we are allowed to on this platform
#if MATCHMAKING_USES_MATCHMAKING_API
		if (!OSC->IsCreatingParty() && !OSC->IsInPartyAsLeader() && !OSC->IsJoiningParty())
#endif
		if (MatchmakingState == EILLMatchmakingState::Canceling)
		{
			UE_LOG(LogOnlineGame, Verbose, TEXT("%s: Cancelled, skipping"), ANSI_TO_TCHAR(__FUNCTION__));

			// Since we may be waiting on the search to stop to finish canceling, finish it up now
			MatchmakingCancelingComplete();
			return;
		}

		// Discard bogus duplicate broadcasts
		if (MatchmakingState == EILLMatchmakingState::Canceled || MatchmakingState == EILLMatchmakingState::NotRunning)
		{
			UE_LOG(LogOnlineGame, Warning, TEXT("%s: Cancelled, ignoring!"), ANSI_TO_TCHAR(__FUNCTION__));
			return;
		}
		if (!ensure(MatchmakingState != EILLMatchmakingState::Joining))
		{
			UE_LOG(LogOnlineGame, Warning, TEXT("%s: Already joining a game session, ignoring!"), ANSI_TO_TCHAR(__FUNCTION__));
			return;
		}
	}
	if (!MatchmakingSearchDelegateHandle.IsValid())
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("%s: Supressing redudant call!"), ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}

	IOnlineSessionPtr Sessions = GetSessionInt();

#if MATCHMAKING_USES_MATCHMAKING_API
	check(MatchmakingSearchDelegateHandle.IsValid());
	Sessions->ClearOnMatchmakingCompleteDelegate_Handle(MatchmakingSearchDelegateHandle);

	if (bSuccess)
	{
		if (FNamedOnlineSession* CurrentSession = Sessions->GetNamedSession(NAME_GameSession))
		{
			// The XB1 OSS "took the wheel" creating and joining a game session for us
			if (CurrentSession->bHosting)
			{
				// Stop the matchmaking state
				SetMatchmakingState(EILLMatchmakingState::NotRunning);

				const int32 MaxPlayers = GetWorld()->GetAuthGameMode()->GetGameSessionClass()->GetDefaultObject<AILLGameSession>()->MaxPlayers;
				FOnlineSessionSettings NewSettings = FILLGameSessionSettings(false, MaxPlayers, true, false);
# if PLATFORM_XBOXONE
				NewSettings.Set(SETTING_MATCHING_TIMEOUT, XBOXONE_QUICKPLAY_MATCHING_TIMEOUT, EOnlineDataAdvertisementType::DontAdvertise);
				NewSettings.Set(SETTING_SESSION_TEMPLATE_NAME, XBOXONE_QUICKPLAY_SESSIONTEMPLATE, EOnlineDataAdvertisementType::DontAdvertise);
				NewSettings.Set(SEARCH_KEYWORDS, FString::Printf(XBOXONE_QUICKPLAY_KEYWORDS_FORMAT, ILLBUILD_BUILD_NUMBER), EOnlineDataAdvertisementType::ViaOnlineService);
				NewSettings.Set(SETTING_MATCHING_ATTRIBUTES, FString::Printf(TEXT("{ \"BuildNumber\": %i }"), ILLBUILD_BUILD_NUMBER), EOnlineDataAdvertisementType::DontAdvertise);
				if (LastAggregateMatchmakingStatus == EILLBackendMatchmakingStatusType::Ok)
					NewSettings.Set(SEARCH_XBOX_LIVE_HOPPER_NAME, XBOXONE_QUICKPLAY_HOPPER_STANDARD, EOnlineDataAdvertisementType::DontAdvertise);
				else
					NewSettings.Set(SEARCH_XBOX_LIVE_HOPPER_NAME, XBOXONE_QUICKPLAY_HOPPER_LOWPRIORITY, EOnlineDataAdvertisementType::DontAdvertise);
# endif
				// Local player is hosting
				// Spawn a lobby beacon immediately
				// This will hold user reservations in pending until the server finishes loading the lobby
				if (OSC->SpawnLobbyBeaconHost(NewSettings))
				{
					// Update our settings immediately so that connected clients can know where to place a reservation sooner
					NewSettings.Set(SETTING_BEACONPORT, OSC->GetLobbyBeaconHostListenPort(), EOnlineDataAdvertisementType::ViaOnlineService);
					Sessions->UpdateSession(NAME_GameSession, NewSettings, true);

					// Travel to a hosted lobby
					// Can't call MatchmakingFinished because then party members won't travel...
					UILLGameBlueprintLibrary::ShowLoadingAndTravel(this, GetOuterUILLGameInstance()->PickRandomQuickPlayMap(), true, TEXT("?listen"));
				}
				else
				{
					// Run through the same failure path
					OSC->FakeJoinCompletion(MatchmakingJoinSessionDelegate, EOnJoinSessionCompleteResult::CouldNotRetrieveAddress);
				}
			}
			else
			{
				// Local player is a client
				// Start attempting to connect, and note that SETTING_BEACONPORT may have no propagated yet because the session was not created through normal flow
				SetMatchmakingState(EILLMatchmakingState::Joining);

				// Now we gotta do the beacon stuff
				OSC->FakeJoinCompletion(MatchmakingJoinSessionDelegate, EOnJoinSessionCompleteResult::Success);
			}
			return;
		}
	}
	else
	{
		// Complete the failure normally, so our game session is destroyed and so we continue matchmaking
		OnMatchmakingJoinSessionCompleted(NAME_GameSession, EOnJoinSessionCompleteResult::CouldNotRetrieveAddress);
		return;
	}
#elif MATCHMAKING_USES_FINDSESSIONS_API
	check(MatchmakingSearchDelegateHandle.IsValid());
	Sessions->ClearOnFindSessionsCompleteDelegate_Handle(MatchmakingSearchDelegateHandle);

	if (FNamedOnlineSession* CurrentSession = Sessions->GetNamedSession(NAME_GameSession))
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("%s: Ignoring results because we already are in a game session!"), ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}

	if (bSuccess && MatchmakingSearchObject.IsValid())
	{
		// Assume failure until we send a join request
		bSuccess = false;

		// Filter down results
		// Filter by result validity
		TArray<FOnlineSessionSearchResult> FilteredResults = MatchmakingSearchObject->SearchResults;
		FilteredResults = FilteredResults.FilterByPredicate([&](const FOnlineSessionSearchResult& SearchResult) -> bool
			{
				// Refuse owner-less sessions, matchmaking relies on it
				// NOTE: Dedicated servers should have their Steam ID in this, as well as clients on any platform obviously, so this is mostly being done to mitigate host-less sessions on XB1 which is somehow a thing (probably half created when it was found)
				return SearchResult.Session.OwningUserId.IsValid();
			});
		const int32 FilterPostBad = FilteredResults.Num();
		const int32 FilteredByBad = MatchmakingSearchObject->SearchResults.Num() - FilterPostBad;

		// Filter by blacklist
		FilteredResults = FilteredResults.FilterByPredicate([&](const FOnlineSessionSearchResult& SearchResult) -> bool
			{
				// Skip blacklisted entries
				for (TSharedPtr<const FUniqueNetIdString> BlacklistEntry : MatchmakingBlacklist)
				{
					if (BlacklistEntry->ToString() == SearchResult.Session.OwningUserId->ToString())
					{
						return false;
					}
				}
				return true;
			});
		const int32 FilterPostBlacklist = FilteredResults.Num();
		const int32 FilteredByBlacklist = FilterPostBad - FilterPostBlacklist;

		// Filter by ping
		const FILLMatchmakingStep& CurrentStep = GetCurrentMatchmakingStep();
		FilteredResults = FilteredResults.FilterByPredicate([&](const FOnlineSessionSearchResult& SearchResult) -> bool
			{
		
# if PLATFORM_DESKTOP
				// FIXME: pjackson: SteamP2P sessions return MAX_QUERY_PING right now... Perform a matchmaking QoS pass in that case? http://www.aclockworkberry.com/ping-9999-issue-on-unreal-engine-steam-session-results/
				// (@see UILLOnlineMatchmakingClient::BuildMatchmakingSearchSteps)
				if (CurrentStep.MatchmakingState != EILLMatchmakingState::SearchingSessions || SearchResult.PingInMs != MAX_QUERY_PING)
# endif
				{
					// Check for an acceptable ping
					if (CurrentStep.PingLimit != MAX_QUERY_PING && SearchResult.PingInMs > CurrentStep.PingLimit)
					{
						return false;
					}
				}
				return true;
			});
		const int32 FilterPostPing = FilteredResults.Num();
		const int32 FilteredByPing = FilterPostBlacklist - FilterPostPing;

		UE_LOG(LogOnlineGame, Verbose, TEXT("%s: SearchResults: %i FilteredResults: %i (%i bad, %i blacklisted, %i filtered by ping)"), ANSI_TO_TCHAR(__FUNCTION__), MatchmakingSearchObject->SearchResults.Num(), FilteredResults.Num(), FilteredByBad, FilteredByBlacklist, FilteredByPing);
# if PLATFORM_DESKTOP
		for (const FOnlineSessionSearchResult& SearchResult : MatchmakingSearchObject->SearchResults)
		{
			UE_LOG(LogOnlineGame, VeryVerbose, TEXT("... %i/%i %4ims \"%s\""), SearchResult.Session.SessionSettings.NumPublicConnections-SearchResult.Session.NumOpenPublicConnections, SearchResult.Session.SessionSettings.NumPublicConnections, SearchResult.PingInMs, *SearchResult.Session.OwningUserName);
		}
# endif

		// Find the best session search result
		// We only bother attempting one session from this result set because in the time you could try to join a session, fail, and then attempt the next would be relatively long in high contention scenarios
		if (const FOnlineSessionSearchResult* BestSearchResult = FindBestSessionResult(FilteredResults))
		{
			++MatchmakingJoinCount;
			if (OSC->JoinSession(NAME_GameSession, *BestSearchResult, MatchmakingJoinSessionDelegate))
			{
				// Session join in progress...
				SetMatchmakingState(EILLMatchmakingState::Joining);
				bSuccess = true;
			}
			else
			{
				bSuccess = false;
			}
		}
	}
	else
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("%s: Search failed"), ANSI_TO_TCHAR(__FUNCTION__));
	}
#endif

	if (!bSuccess)
	{
		// No session found or join attempt failed
		// Move to the next step, but delay it a bit so the OSS can cleanup CurrentSessionSearch... yes really
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_ContinueMatchmaking, FTimerDelegate::CreateUObject(this, &ThisClass::ContinueMatchmaking, true), MATCHMAKING_CONTINUE_DELAY, false);
	}
}

#if MATCHMAKING_USES_MATCHMAKING_API
void UILLOnlineMatchmakingClient::OnMatchmakingCancelComplete(FName InSessionName, bool bWasSuccessful)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	IOnlineSessionPtr Sessions = GetSessionInt();
	check(MatchmakingCancelDelegateHandle.IsValid());
	Sessions->ClearOnCancelMatchmakingCompleteDelegate_Handle(MatchmakingCancelDelegateHandle);

	MatchmakingCancelingComplete();
}
#endif // MATCHMAKING_USES_MATCHMAKING_API

#if MATCHMAKING_SUPPORTS_HTTP_API
UILLBackendSimpleHTTPRequestTask* UILLOnlineMatchmakingClient::BuildMatchmakingRegionsRequest(FILLOnMatchmakingRegionResponseDelegate& Delegate)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Grab the (optional) BackendPlayer, dedicated servers will not have one obviously
	UWorld* World = GetWorld();
	UILLLocalPlayer* FirstLocalPlayer = Cast<UILLLocalPlayer>(World->GetFirstLocalPlayerFromController());
	UILLBackendPlayer* BackendPlayer = FirstLocalPlayer ? FirstLocalPlayer->GetBackendPlayer() : nullptr;

	// Build a request
	OnMatchmakingRegionResponse = Delegate;
	FOnILLBackendSimpleRequestDelegate RequestDelegate = FOnILLBackendSimpleRequestDelegate::CreateUObject(this, &ThisClass::MatchmakingRegionsResponse);
	UILLBackendRequestManager* BRM = GetOuterUILLGameInstance()->GetBackendRequestManager();
	return BRM->CreateSimpleRequest(BRM->MatchmakingService_Regions, RequestDelegate, FString(), BackendPlayer);
}

void UILLOnlineMatchmakingClient::MatchmakingRegionsResponse(int32 ResponseCode, const FString& ResponseContent)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Parse the endpoints
	MatchmakingEndpoints.Empty();
	if (ResponseCode == 200 && !ResponseContent.IsEmpty())
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ResponseContent);
		if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
		{
			for (auto It = JsonObject->Values.CreateConstIterator(); It; ++It)
			{
				FILLHTTPMatchmakingEndpoint Entry;
				Entry.Name = It.Key();
				if (It.Value()->TryGetString(Entry.Address))
					MatchmakingEndpoints.Add(Entry);
			}
		}
	}

	// Send UDP echo requests to all of them
	UE_LOG(LogOnlineGame, Log, TEXT("%s: ResponseCode: %i, Endpoints: %i"), ANSI_TO_TCHAR(__FUNCTION__), ResponseCode, MatchmakingEndpoints.Num());
	if (MatchmakingEndpoints.Num() > 0)
	{
		// Grab our XB1 SDA template to open the port
#if PLATFORM_XBOXONE
		if (!QoSTemplateName.IsEmpty())
		{
			try
			{
				QoSPeerTemplate = SecureDeviceAssociationTemplate::GetTemplateByName(ref new Platform::String(*QoSTemplateName));

				// Listen for Secure Association incoming
				auto AssociationIncomingEvent = ref new TypedEventHandler<SecureDeviceAssociationTemplate^, SecureDeviceAssociationIncomingEventArgs^>(
					[] (Platform::Object^, SecureDeviceAssociationIncomingEventArgs^ EventArgs)
				{
					if (EventArgs->Association)
					{
						UE_LOG(LogBeacon, Log, TEXT("Received association, state is %d."), EventArgs->Association->State);

						auto StateChangedEvent = ref new TypedEventHandler<SecureDeviceAssociation^, SecureDeviceAssociationStateChangedEventArgs^>(&LogAssociationStateChange);
						EventArgs->Association->StateChanged += StateChangedEvent;
					}
				});

				QoSAssociationIncoming = QoSPeerTemplate->AssociationIncoming += AssociationIncomingEvent;
			}
			catch(Platform::COMException^ Ex)
			{
				UE_LOG(LogOnlineGame, Error, TEXT("Couldn't find secure device association template named %s. Check the app manifest."), *QoSTemplateName);
			}
		}
#endif

		for (FILLHTTPMatchmakingEndpoint& Entry : MatchmakingEndpoints)
		{
			SendMatchmakingRegionUDP(Entry);
		}
	}
	else
	{
		OnMatchmakingRegionResponse.Execute(false, MatchmakingEndpoints);
	}
}

void UILLOnlineMatchmakingClient::SendMatchmakingRegionUDP(FILLHTTPMatchmakingEndpoint& EndpointEntry)
{
	ILLUDPEcho(EndpointEntry.Address, HTTP_MATCHMAKING_QOS_TIMEOUT_DURATION, [this, &EndpointEntry](FIcmpEchoResult Result)
	{
		OnMatchmakingRegionQoSResponse(EndpointEntry, Result);
	});
}

void UILLOnlineMatchmakingClient::OnMatchmakingRegionQoSResponse(FILLHTTPMatchmakingEndpoint& EndpointEntry, const FIcmpEchoResult& Result)
{
	// Store the latency result
	if (Result.Status == EIcmpResponseStatus::Success)
	{
		// Convert back to milliseconds
		EndpointEntry.PingBucket.Add(Result.Time * 1000.f);
	}
	else if (Result.Status == EIcmpResponseStatus::Timeout)
	{
		if (EndpointEntry.SuppressedTimeouts >= HTTP_MATCHMAKING_QOS_TIMEOUTS_IGNORED)
		{
			// Store the timeout duration as the latency when we timeout, as they are less serious than other failures
			EndpointEntry.PingBucket.Add(HTTP_MATCHMAKING_QOS_TIMEOUT_DURATION * 1000.f);
		}
		else
		{
			// Suppress this timeout
			++EndpointEntry.SuppressedTimeouts;
		}
	}
	else
	{
		// Any other failure is very serious, fill the PingBuckets with horrible values so this is unused
		while (EndpointEntry.PingBucket.Num() < HTTP_MATCHMAKING_QOS_PASSES)
			EndpointEntry.PingBucket.Add(MAX_QUERY_PING);
	}

	// Check if we need to go again or if we are done
	if (EndpointEntry.PingBucket.Num() < HTTP_MATCHMAKING_QOS_PASSES)
	{
		SendMatchmakingRegionUDP(EndpointEntry);
	}
	else
	{
		// We have completed evaluating this specific endpoint
		// Calculate average latency based on the PingBuckets
		EndpointEntry.Latency = 0.f;
		for (float Ping : EndpointEntry.PingBucket)
			EndpointEntry.Latency += Ping;
		EndpointEntry.Latency *= 1.f / float(EndpointEntry.PingBucket.Num());

		// Check if all endpoints have completed and broadcast
		const bool bCompleted = !MatchmakingEndpoints.ContainsByPredicate([](const FILLHTTPMatchmakingEndpoint& Entry) -> bool { return (Entry.PingBucket.Num() < HTTP_MATCHMAKING_QOS_PASSES); });
		if (bCompleted)
		{
			FString ServerRegion;
			if (IsRunningDedicatedServer())
				FParse::Value(FCommandLine::Get(), TEXT("ServerRegion="), ServerRegion);

			// Cull out bad entries with no good QoS results, and report metrics
			UE_LOG(LogOnlineGame, Log, TEXT("%s: Completed, dumping metrics..."), ANSI_TO_TCHAR(__FUNCTION__));
			for (int32 EndpointIndex = 0; EndpointIndex < MatchmakingEndpoints.Num(); )
			{
				if (!ServerRegion.IsEmpty() && MatchmakingEndpoints[EndpointIndex].Name == ServerRegion)
				{
					// Always accept our local server region as best
					// This will become irrelevant once servers start using player QoS results for replacement lookup
					MatchmakingEndpoints[EndpointIndex].PingBucket.Empty(1);
					MatchmakingEndpoints[EndpointIndex].PingBucket.Add(0.f);
					MatchmakingEndpoints[EndpointIndex].Latency = 0.f;
					++EndpointIndex;
				}
				else if (!MatchmakingEndpoints[EndpointIndex].PingBucket.ContainsByPredicate([](const float Time) -> bool { return (Time < MAX_QUERY_PING); }))
				{
					UE_LOG(LogOnlineGame, Log, TEXT("... %s removed for exceeding MAX_QUERY_PING"), *MatchmakingEndpoints[EndpointIndex].Name);
					MatchmakingEndpoints.RemoveAt(EndpointIndex);
				}
				else
				{
					++EndpointIndex;
				}
			}
			for (const FILLHTTPMatchmakingEndpoint& Endpoint : MatchmakingEndpoints)
			{
				UE_LOG(LogOnlineGame, Log, TEXT("... %s: %.1fms"), *Endpoint.Name, Endpoint.Latency);
			}

			// Skip frequent requests
			LastRegionRequestRealTime = GetWorld()->GetRealTimeSeconds();

			// Cleanup our XB1 SDA
#if PLATFORM_XBOXONE
			if (QoSPeerTemplate)
			{
				QoSPeerTemplate->AssociationIncoming -= QoSAssociationIncoming;
			}
#endif

			// Remember, success depends on getting back valid endpoints!
			OnMatchmakingRegionResponse.Execute(MatchmakingEndpoints.Num() > 0, MatchmakingEndpoints);
		}
	}
}

void UILLOnlineMatchmakingClient::MatchmakingRegionsHandler(bool bSuccess, const TArray<FILLHTTPMatchmakingEndpoint>& Endpoints)
{
	check(MatchmakingRegionsHTTPRequest);
	MatchmakingRegionsHTTPRequest = nullptr;

	// State checks
	if (MatchmakingState == EILLMatchmakingState::Canceling)
	{
		UE_LOG(LogOnlineGame, Verbose, TEXT("%s: Cancelled, skipping"), ANSI_TO_TCHAR(__FUNCTION__));
		MatchmakingCancelingComplete();
		return;
	}
	if (MatchmakingState == EILLMatchmakingState::Canceled || MatchmakingState == EILLMatchmakingState::NotRunning)
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("%s: Cancelled, ignoring!"), ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}

	// Clear the MatchmakingEndpoints on failure, HTTP matchmaking will only proceed if this is non-empty
	if (!bSuccess)
	{
		MatchmakingEndpoints.Empty();
	}

	// Continue matchmaking
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_ContinueMatchmaking, FTimerDelegate::CreateUObject(this, &ThisClass::ContinueMatchmaking, true), MATCHMAKING_CONTINUE_DELAY, false);
}

void UILLOnlineMatchmakingClient::SendMatchmakingRegionsRequest()
{
	check(!MatchmakingRegionsHTTPRequest);

	// Build and send the request
	FILLOnMatchmakingRegionResponseDelegate Delegate = FILLOnMatchmakingRegionResponseDelegate::CreateUObject(this, &ThisClass::MatchmakingRegionsHandler);
	MatchmakingRegionsHTTPRequest = BuildMatchmakingRegionsRequest(Delegate);
	MatchmakingRegionsHTTPRequest->QueueRequest();
}

UILLBackendSimpleHTTPRequestTask* UILLOnlineMatchmakingClient::BuildMatchmakingQueueRequest(const TArray<FILLBackendPlayerIdentifier>& AccountIdList, const TArray<FILLHTTPMatchmakingEndpoint>& EndpointList, const bool bSkipWaitForPlayersDelay, FILLOnMatchmakingQueueResponseDelegate& Delegate)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: Accounts: %i, bSkipWaitForPlayersDelay: %s"), ANSI_TO_TCHAR(__FUNCTION__), AccountIdList.Num(), bSkipWaitForPlayersDelay ? TEXT("true") : TEXT("false"));

	// Grab the (optional) BackendPlayer, dedicated servers will not have one obviously
	UWorld* World = GetWorld();
	UILLLocalPlayer* FirstLocalPlayer = Cast<UILLLocalPlayer>(World->GetFirstLocalPlayerFromController());
	UILLBackendPlayer* BackendPlayer = FirstLocalPlayer ? FirstLocalPlayer->GetBackendPlayer() : nullptr;

	// Build request contents
	FString RequestContents;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> JsonWriter = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&RequestContents);
	JsonWriter->WriteObjectStart();
	JsonWriter->WriteArrayStart(TEXT("PlayerIDs"));
	for (const FILLBackendPlayerIdentifier& AccountId : AccountIdList)
	{
		JsonWriter->WriteValue(AccountId.GetIdentifier());
	}
	JsonWriter->WriteArrayEnd();
	JsonWriter->WriteObjectStart(TEXT("LatencyData"));
	for (const FILLHTTPMatchmakingEndpoint& Region : EndpointList)
	{
		JsonWriter->WriteValue(Region.Name, Region.Latency);
	}
	JsonWriter->WriteObjectEnd();
	JsonWriter->WriteValue(TEXT("GameSessionID"), TEXT(""));
	JsonWriter->WriteValue(TEXT("SkipWaitForPlayersDelay"), bSkipWaitForPlayersDelay);
	JsonWriter->WriteValue(TEXT("Criteria"), TEXT(""));
	JsonWriter->WriteObjectEnd();
	JsonWriter->Close();

	// Build a request
	OnMatchmakingQueueResponse = Delegate;
	FOnILLBackendSimpleRequestDelegate RequestDelegate = FOnILLBackendSimpleRequestDelegate::CreateUObject(this, &ThisClass::MatchmakingQueueResponse);
	UILLBackendRequestManager* BRM = GetOuterUILLGameInstance()->GetBackendRequestManager();
	UILLBackendSimpleHTTPRequestTask* Result = BRM->CreateSimpleRequest(BRM->MatchmakingService_Queue, RequestDelegate, FString(), BackendPlayer);
	Result->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Result->SetContentAsString(RequestContents);

	return Result;
}

void UILLOnlineMatchmakingClient::MatchmakingQueueResponse(int32 ResponseCode, const FString& ResponseContent)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: ResponseCode: %i"), ANSI_TO_TCHAR(__FUNCTION__), ResponseCode);

	// Parse the response to see if we have started
	bool bSuccess = false;
	FString RequestId;
	double RequestDelay = 5.f;
	if (ResponseCode == 200 && !ResponseContent.IsEmpty())
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ResponseContent);
		if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
		{
			if (JsonObject->TryGetStringField(TEXT("RequestID"), RequestId) && !RequestId.IsEmpty())
			{
				JsonObject->TryGetNumberField(TEXT("RequestDelay"), RequestDelay);
				bSuccess = true;
			}
		}
	}

	// Notify our initiator
	OnMatchmakingQueueResponse.Execute(bSuccess, RequestId, float(RequestDelay));
}

bool UILLOnlineMatchmakingClient::SendMatchmakingQueueRequest()
{
	MatchmakingRequestId.Empty();
	check(!MatchmakingQueueHTTPRequest);
	check(!TimerHandle_SendMatchmakingDescribe.IsValid());

	UWorld* World = GetWorld();
	UILLOnlineSessionClient* OSC = GetOnlineSession();
	UILLLocalPlayer* FirstLocalPlayer = CastChecked<UILLLocalPlayer>(World->GetFirstLocalPlayerFromController());
	UILLBackendPlayer* BackendPlayer = FirstLocalPlayer->GetBackendPlayer();
	if (BackendPlayer && BackendPlayer->IsLoggedIn())
	{
		// Build a list of users to queue
		TArray<FILLBackendPlayerIdentifier> AccountIdList;
		if (AILLPartyBeaconState* PartyBeaconState = OSC->GetPartyBeaconState())
		{
			AccountIdList = PartyBeaconState->GenerateAccountIdList();
		}
		else
		{
			AccountIdList.Add(BackendPlayer->GetAccountId());
		}

		// Build a delegate and send the request
		MatchmakingQueueStartRealTime = World->GetRealTimeSeconds();
		FILLOnMatchmakingQueueResponseDelegate Delegate = FILLOnMatchmakingQueueResponseDelegate::CreateUObject(this, &ThisClass::MatchmakingQueueHandler);
		MatchmakingQueueHTTPRequest = BuildMatchmakingQueueRequest(AccountIdList, MatchmakingEndpoints, false, Delegate);
		MatchmakingQueueHTTPRequest->QueueRequest();
		return true;
	}

	return false;
}

void UILLOnlineMatchmakingClient::MatchmakingQueueHandler(bool bSuccess, const FString& RequestId, float RequestDelay)
{
	// State checks
	if (MatchmakingState == EILLMatchmakingState::Canceling)
	{
		UE_LOG(LogOnlineGame, Verbose, TEXT("%s: Cancelled, skipping"), ANSI_TO_TCHAR(__FUNCTION__));
		// Since we may be waiting on the search to stop to finish canceling, finish it up now
		MatchmakingCancelingComplete();
		return;
	}
	if (MatchmakingState == EILLMatchmakingState::Canceled || MatchmakingState == EILLMatchmakingState::NotRunning)
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("%s: Cancelled, ignoring!"), ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}
	if (!ensure(MatchmakingState != EILLMatchmakingState::Joining))
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("%s: Already joining a game session, ignoring!"), ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}

	check(MatchmakingQueueHTTPRequest);
	MatchmakingQueueHTTPRequest = nullptr;

	check(MatchmakingRequestId.IsEmpty());
	check(!TimerHandle_SendMatchmakingDescribe.IsValid());

	if (bSuccess && ensure(!RequestId.IsEmpty()))
	{
		// Set a timer to describe
		MatchmakingRequestId = RequestId;
		MatchmakingDescribeDelay = RequestDelay;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_SendMatchmakingDescribe, this, &ThisClass::SendMatchmakingDescribeRequest, MatchmakingDescribeDelay);
	}
	else
	{
		// No session found or join attempt failed
		SendMatchmakingCancelRequest(false);

		// Set a timer to continue with the fall back matchmaking path
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_ContinueMatchmaking, FTimerDelegate::CreateUObject(this, &ThisClass::ContinueMatchmaking, true), MATCHMAKING_CONTINUE_DELAY, false);
	}
}

UILLBackendSimpleHTTPRequestTask* UILLOnlineMatchmakingClient::BuildMatchmakingDescribeRequest(const FString& RequestId, FILLOnMatchmakingDescribeResponseDelegate& Delegate)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: RequestId: %s"), ANSI_TO_TCHAR(__FUNCTION__), *RequestId);

	// Grab the (optional) BackendPlayer, dedicated servers will not have one obviously
	UWorld* World = GetWorld();
	UILLLocalPlayer* FirstLocalPlayer = Cast<UILLLocalPlayer>(World->GetFirstLocalPlayerFromController());
	UILLBackendPlayer* BackendPlayer = FirstLocalPlayer ? FirstLocalPlayer->GetBackendPlayer() : nullptr;

	// Build a request
	OnMatchmakingDescribeResponseDelegate = Delegate;
	FOnILLBackendSimpleRequestDelegate RequestDelegate = FOnILLBackendSimpleRequestDelegate::CreateUObject(this, &ThisClass::MatchmakingDescribeResponse);
	UILLBackendRequestManager* BRM = GetOuterUILLGameInstance()->GetBackendRequestManager();
	return BRM->CreateSimpleRequest(BRM->MatchmakingService_Describe, RequestDelegate, FString::Printf(TEXT("?RequestID=%s"), *RequestId), BackendPlayer);
}

void UILLOnlineMatchmakingClient::MatchmakingDescribeResponse(int32 ResponseCode, const FString& ResponseContent)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: ResponseCode: %i"), ANSI_TO_TCHAR(__FUNCTION__), ResponseCode);

	bool bFinished = false;
	TArray<FILLMatchmadePlayerSession> PlayerSessions;

	if (ResponseCode == 200 && !ResponseContent.IsEmpty())
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ResponseContent);
		if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
		{
			const TArray<TSharedPtr<FJsonValue>>* PlayerSessionsObjects = nullptr;
			if (JsonObject->TryGetArrayField(TEXT("PlayerSessions"), PlayerSessionsObjects))
			{
				PlayerSessions.Empty(PlayerSessionsObjects->Num());
				for (TSharedPtr<FJsonValue> PlayerSessionValue : *PlayerSessionsObjects)
				{
					const TSharedPtr<FJsonObject>* PlayerSessionObject = nullptr;
					if (PlayerSessionValue->TryGetObject(PlayerSessionObject))
					{
						FILLMatchmadePlayerSession Entry;
						FString IpAddress;
						int32 Port = 0;
						FString PlayerId;
						if((*PlayerSessionObject)->TryGetStringField(TEXT("IpAddress"), IpAddress)
						&& (*PlayerSessionObject)->TryGetNumberField(TEXT("Port"), Port)
						&& (*PlayerSessionObject)->TryGetStringField(TEXT("PlayerId"), PlayerId)
						&& (*PlayerSessionObject)->TryGetStringField(TEXT("PlayerSessionId"), Entry.PlayerSessionId))
						{
							Entry.ConnectionString = FString::Printf(TEXT("%s:%i"), *IpAddress, Port);
							Entry.AccountId = FILLBackendPlayerIdentifier(PlayerId);
							PlayerSessions.Add(Entry);
						}
					}
				}

				if (PlayerSessions.Num() == PlayerSessionsObjects->Num())
				{
					bFinished = true;
				}
				else
				{
					// Failed to parse
					UE_LOG(LogOnlineGame, Warning, TEXT("%s: Failed to parse all reservations for the group! (%i of %i parsed)"), ANSI_TO_TCHAR(__FUNCTION__), PlayerSessions.Num(), PlayerSessionsObjects->Num());
				}
			}
			else
			{
				UE_LOG(LogOnlineGame, Warning, TEXT("%s: Failed to find PlayerSessions array!"), ANSI_TO_TCHAR(__FUNCTION__));
			}
		}
		else
		{
			UE_LOG(LogOnlineGame, Warning, TEXT("%s: Failed to parse ResponseContent as JSON!"), ANSI_TO_TCHAR(__FUNCTION__));
		}
	}
	else if (ResponseCode == 404)
	{
		// Not ready or nothing found
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ResponseContent);
		if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
		{
			FString ErrorMessage;
			if (JsonObject->TryGetStringField(TEXT("Error"), ErrorMessage))
			{
				UE_LOG(LogOnlineGame, Verbose, TEXT("%s: Got error: \"%s\""), ANSI_TO_TCHAR(__FUNCTION__), *ErrorMessage);
			}
		}
	}

	if (!bFinished)
	{
		PlayerSessions.Empty();
	}
	OnMatchmakingDescribeResponseDelegate.Execute(bFinished, PlayerSessions);
}

void UILLOnlineMatchmakingClient::SendMatchmakingDescribeRequest()
{
	check(!MatchmakingRequestId.IsEmpty());
	check(!MatchmakingQueueHTTPRequest);
	check(!MatchmakingDescribeHTTPRequest);
	check(!MatchmakingCancelHTTPRequest);
	check(TimerHandle_SendMatchmakingDescribe.IsValid());
	TimerHandle_SendMatchmakingDescribe.Invalidate();

	// Build a delegate and send the request
	FILLOnMatchmakingDescribeResponseDelegate Delegate = FILLOnMatchmakingDescribeResponseDelegate::CreateUObject(this, &ThisClass::MatchmakingDescribeHandler);
	MatchmakingDescribeHTTPRequest = BuildMatchmakingDescribeRequest(MatchmakingRequestId, Delegate);
	MatchmakingDescribeHTTPRequest->QueueRequest();
}

void UILLOnlineMatchmakingClient::MatchmakingDescribeHandler(bool bFinished, const TArray<FILLMatchmadePlayerSession>& PlayerSessions)
{
	check(MatchmakingDescribeHTTPRequest);
	MatchmakingDescribeHTTPRequest = nullptr;

	if (MatchmakingState != EILLMatchmakingState::SearchingHTTP)
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("%s: Hit while not in SearchingHTTP state!"), ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}

	UWorld* World = GetWorld();
	bool bShouldContinueMatchmaking = true;

	if (bFinished)
	{
		// OnMatchmakingRequestSessionComplete will take care of the results even if BeginRequestLobbySession returns false
		bShouldContinueMatchmaking = false;

		// Begin a party lobby session request
		if (GetOnlineSession()->BeginRequestPartyLobbySession(PlayerSessions, MatchmakingLobbyBeaconSessionReceivedDelegate, MatchmakingLobbyBeaconSessionRequestCompleteDelegate))
		{
			// Update matchmaking state and do not continue, we're going to attempt travel
			SetMatchmakingState(EILLMatchmakingState::Joining);
		}
		else
		{
			UE_LOG(LogOnlineGame, Warning, TEXT("%s: BeginRequestLobbySession failed!"), ANSI_TO_TCHAR(__FUNCTION__));
		}
	}
	else
	{
		// Check if we should try again
		const float MatchmakingTime = World->GetRealTimeSeconds() - MatchmakingQueueStartRealTime;
		if (MatchmakingHTTPGiveUpTime > 0.f && MatchmakingTime > MatchmakingHTTPGiveUpTime)
		{
			UE_LOG(LogOnlineGame, Warning, TEXT("%s: MatchmakingHTTPGiveUpTime (%3.2fs) hit! Falling back to legacy matchmaking path."), ANSI_TO_TCHAR(__FUNCTION__), MatchmakingHTTPGiveUpTime);
			bShouldContinueMatchmaking = true;
		}
		else
		{
			// Set a timer to describe again
			World->GetTimerManager().SetTimer(TimerHandle_SendMatchmakingDescribe, this, &ThisClass::SendMatchmakingDescribeRequest, MatchmakingDescribeDelay);
			bShouldContinueMatchmaking = false;
		}
	}

	if (bShouldContinueMatchmaking)
	{
		// Cleanup
		SendMatchmakingCancelRequest(false);

		// Set a timer to continue with the fall back matchmaking path
		World->GetTimerManager().SetTimer(TimerHandle_ContinueMatchmaking, FTimerDelegate::CreateUObject(this, &ThisClass::ContinueMatchmaking, true), MATCHMAKING_CONTINUE_DELAY, false);
	}
}

UILLBackendSimpleHTTPRequestTask* UILLOnlineMatchmakingClient::BuildMatchmakingCancelRequest(const FString& RequestId, FOnILLBackendSimpleRequestDelegate& Delegate)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: RequestId: %s"), ANSI_TO_TCHAR(__FUNCTION__), *RequestId);

	// Grab the (optional) BackendPlayer, dedicated servers will not have one obviously
	UWorld* World = GetWorld();
	UILLLocalPlayer* FirstLocalPlayer = Cast<UILLLocalPlayer>(World->GetFirstLocalPlayerFromController());
	UILLBackendPlayer* BackendPlayer = FirstLocalPlayer ? FirstLocalPlayer->GetBackendPlayer() : nullptr;

	// Build a request
	UILLBackendRequestManager* BRM = GetOuterUILLGameInstance()->GetBackendRequestManager();
	return BRM->CreateSimpleRequest(BRM->MatchmakingService_Cancel, Delegate, FString::Printf(TEXT("?RequestID=%s"), *RequestId), BackendPlayer);
}

void UILLOnlineMatchmakingClient::SendMatchmakingCancelRequest(const bool bForStateChange)
{
	if (MatchmakingRequestId.IsEmpty())
		return;

	check(!MatchmakingCancelHTTPRequest);

	// Build a delegate and send the request
	FOnILLBackendSimpleRequestDelegate Delegate;
	if (bForStateChange)
	{
		Delegate = FOnILLBackendSimpleRequestDelegate::CreateUObject(this, &ThisClass::MatchmakingCancelResponseChangeState);
		SetMatchmakingState(EILLMatchmakingState::Canceling);
	}
	else
	{
		Delegate = FOnILLBackendSimpleRequestDelegate::CreateUObject(this, &ThisClass::MatchmakingCancelResponse);
	}
	MatchmakingCancelHTTPRequest = BuildMatchmakingCancelRequest(MatchmakingRequestId, Delegate);
	MatchmakingCancelHTTPRequest->QueueRequest();

	// Cleanup and forbid re-entry
	MatchmakingRequestId.Empty();
}

void UILLOnlineMatchmakingClient::MatchmakingCancelResponse(int32 ResponseCode, const FString& ResponseContent)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: ResponseCode: %i"), ANSI_TO_TCHAR(__FUNCTION__), ResponseCode);

	check(MatchmakingCancelHTTPRequest);
	MatchmakingCancelHTTPRequest = nullptr;
}

void UILLOnlineMatchmakingClient::MatchmakingCancelResponseChangeState(int32 ResponseCode, const FString& ResponseContent)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: ResponseCode: %i"), ANSI_TO_TCHAR(__FUNCTION__), ResponseCode);

	check(MatchmakingCancelHTTPRequest);
	MatchmakingCancelHTTPRequest = nullptr;

	MatchmakingCancelingComplete();
}

void UILLOnlineMatchmakingClient::OnMatchmakingLobbyClientSessionReceived(const FString& SessionId)
{
}

void UILLOnlineMatchmakingClient::OnMatchmakingRequestSessionComplete(const EILLLobbyBeaconSessionResult Response)
{
	if (Response == EILLLobbyBeaconSessionResult::Success)
	{
		// We have joined a game session, kill matchmaking
		SetMatchmakingState(EILLMatchmakingState::NotRunning);
	}
	else
	{
		// Sometimes we will have partially joined the Game session and need to clean it up
		bool bCanResumeMatchmaking = true;
		IOnlineSessionPtr Sessions = GetSessionInt();
		if (FNamedOnlineSession* CurrentSession = Sessions->GetNamedSession(NAME_GameSession))
		{
			if (CurrentSession->SessionState != EOnlineSessionState::Destroying)
			{
				UE_LOG(LogOnlineGame, Warning, TEXT("%s: Dangling Game session detected, destroying it"), ANSI_TO_TCHAR(__FUNCTION__));
				GetOnlineSession()->DestroyExistingSession_Impl(DestroyGameSessionForMatchmakingJoinCompleteDelegateHandle, NAME_GameSession, DestroyGameSessionForMatchmakingJoinCompleteDelegate);
				bCanResumeMatchmaking = false;
			}
		}

		// Attempt to resume matchmaking
		if (bCanResumeMatchmaking && IsMatchmaking())
		{
			if (GetMatchmakingState() == EILLMatchmakingState::Canceling)
			{
				UE_LOG(LogOnlineGame, Verbose, TEXT("%s: Cancelled, ending matchmaking"), ANSI_TO_TCHAR(__FUNCTION__));
				MatchmakingCancelingComplete();
			}
			else
			{
				// Lay off the internet! And make sure we leave the Joining state.
				PerformMatchmakingCooldown();
			}
		}
	}
}
#endif // MATCHMAKING_SUPPORTS_HTTP_API

void UILLOnlineMatchmakingClient::OnMatchmakingJoinSessionCompleted(FName InSessionName, EOnJoinSessionCompleteResult::Type Result)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: Result: %s"), ANSI_TO_TCHAR(__FUNCTION__), *GetLocalizedDescription(InSessionName, Result).ToString());

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		// Track the fail
		++MatchmakingJoinFailures;

		// Blacklist the failed session so we don't try it again during this matchmaking pass
		IOnlineSessionPtr Sessions = GetSessionInt();
		FNamedOnlineSession* CurrentSession = Sessions.IsValid() ? Sessions->GetNamedSession(NAME_GameSession) : nullptr;
		if (CurrentSession && CurrentSession->OwningUserId.IsValid())
		{
			MatchmakingBlacklist.Add(MakeShareable(new FUniqueNetIdString(CurrentSession->OwningUserId->ToString())));
		}

		if (CurrentSession)
		{
			if (CurrentSession->SessionState != EOnlineSessionState::Destroying)
			{
				// Destroy the current session, which will ContinueMatchmaking when it's done if that's still desired
				GetOnlineSession()->DestroyExistingSession_Impl(DestroyGameSessionForMatchmakingJoinCompleteDelegateHandle, NAME_GameSession, DestroyGameSessionForMatchmakingJoinCompleteDelegate);
			}
		}
		else
		{
			// Check for cancellation
			// We only do this on failure because joining+traveling to a game session only to return to the main menu is weird
			if (MatchmakingState == EILLMatchmakingState::Canceling)
			{
				UE_LOG(LogOnlineGame, Verbose, TEXT("%s: Cancelled, destroying game session"), ANSI_TO_TCHAR(__FUNCTION__));
				MatchmakingCancelingComplete();
			}
			else
			{
				// Lay off the internet! And make sure we leave the Joining state.
				PerformMatchmakingCooldown();
			}
		}
		return;
	}

	// We have joined a game session, kill matchmaking
	SetMatchmakingState(EILLMatchmakingState::NotRunning);
}

void UILLOnlineMatchmakingClient::OnDestroyGameSessionForMatchmakingJoinComplete(FName InSessionName, bool bWasSuccessful)
{
	if (InSessionName != NAME_GameSession)
		return;

	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: %s bSuccess: %d"), ANSI_TO_TCHAR(__FUNCTION__), *InSessionName.ToString(), bWasSuccessful);

	IOnlineSessionPtr Sessions = GetSessionInt();
	check(DestroyGameSessionForMatchmakingJoinCompleteDelegateHandle.IsValid());
	Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyGameSessionForMatchmakingJoinCompleteDelegateHandle);

	// Ensure this is cleaned up
	UILLOnlineSessionClient* OSC = GetOnlineSession();
	OSC->ClearPendingGameSession();

	// Resume matchmaking
	if (IsMatchmaking())
	{
		if (GetMatchmakingState() == EILLMatchmakingState::Canceling)
		{
			UE_LOG(LogOnlineGame, Verbose, TEXT("%s: Cancelled, ending matchmaking"), ANSI_TO_TCHAR(__FUNCTION__));
			MatchmakingCancelingComplete();
		}
		else
		{
			// Lay off the internet! And make sure we leave the Joining state.
			PerformMatchmakingCooldown();
		}
	}
}

void UILLOnlineMatchmakingClient::OnDestroyGameSessionForMatchmakingCancelComplete(FName InSessionName, bool bWasSuccessful)
{
	if (InSessionName != NAME_GameSession)
		return;

	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: %s bSuccess: %d"), ANSI_TO_TCHAR(__FUNCTION__), *InSessionName.ToString(), bWasSuccessful);

	IOnlineSessionPtr Sessions = GetSessionInt();
	check(DestroyGameSessionForMatchmakingCancelCompleteDelegateHandle.IsValid());
	Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyGameSessionForMatchmakingCancelCompleteDelegateHandle);

	// Ensure this is cleaned up
	UILLOnlineSessionClient* OSC = GetOnlineSession();
	OSC->ClearPendingGameSession();

	// Resume matchmaking
	if (IsMatchmaking())
	{
		if (GetMatchmakingState() == EILLMatchmakingState::Canceling)
		{
			UE_LOG(LogOnlineGame, Verbose, TEXT("%s: Cancelled, ending matchmaking"), ANSI_TO_TCHAR(__FUNCTION__));
			MatchmakingCancelingComplete();
		}
		else
		{
			// Lay off the internet! And make sure we leave the Joining state.
			PerformMatchmakingCooldown();
		}
	}
}

bool UILLOnlineMatchmakingClient::BeginRequestServerReplacement()
{
	if (!SupportsServerReplacement())
		return false;

#if MATCHMAKING_SUPPORTS_HTTP_API
	// If we are already running a replacement sequence, short out
	if (PendingServerReplacement())
		return true;

	// Only bother with this when we have players to migrate
	UWorld* World = GetWorld();
	AILLGameMode* GameMode = World->GetAuthGameMode<AILLGameMode>();
	if (GameMode->GetNumPlayers() <= 0)
		return false;

	// Wait for replacement regions to be received
	if (!ReplacementRegionsHTTPRequest)
	{
		SendReplacementRegionsRequest();
		return true;
	}

	// We can not do this without QoS data
	if (ReplacementEndpoints.Num() <= 0)
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("%s: No endpoint QoS data to find a replacement, aborting"), ANSI_TO_TCHAR(__FUNCTION__));
		return false;
	}

	// Build an account identifier list for all lobby members
	TArray<FILLBackendPlayerIdentifier> AccountIdList;
	UILLOnlineSessionClient* OSC = GetOnlineSession();
	if (AILLLobbyBeaconState* LobbyBeaconState = OSC->GetLobbyBeaconState())
	{
		for (int32 MemberIndex = 0; MemberIndex < LobbyBeaconState->GetNumMembers(); ++MemberIndex)
		{
			if (AILLLobbyBeaconMemberState* LobbyMember = Cast<AILLLobbyBeaconMemberState>(LobbyBeaconState->GetMemberAtIndex(MemberIndex)))
			{
				AccountIdList.Add(LobbyMember->GetAccountId());
			}
		}
	}
	if (AccountIdList.Num() == 0)
	{
		UE_LOG(LogOnlineGame, Verbose, TEXT("%s: No players to find a replacement for, aborting"), ANSI_TO_TCHAR(__FUNCTION__));
		return false;
	}

	// Build a delegate and send the request
	ReplacementRequestId.Empty();
	ReplacementStartRealTime = GetWorld()->GetRealTimeSeconds();
	FILLOnMatchmakingQueueResponseDelegate Delegate = FILLOnMatchmakingQueueResponseDelegate::CreateUObject(this, &ThisClass::ReplacementQueueHandler);
	ReplacementQueueHTTPRequest = BuildMatchmakingQueueRequest(AccountIdList, ReplacementEndpoints, true, Delegate);
	ReplacementQueueHTTPRequest->QueueRequest();
	return true;
#else
	return false;
#endif
}

bool UILLOnlineMatchmakingClient::SupportsServerReplacement() const
{
#if MATCHMAKING_SUPPORTS_HTTP_API
	return IsRunningDedicatedServer();
#else
	return false;
#endif
}

bool UILLOnlineMatchmakingClient::HandleDisconnectForMatchmaking(UWorld* World, UNetDriver* NetDriver, const TArray<FILLMatchmadePlayerSession>& PlayerSessions/* = TArray<FILLMatchmadePlayerSession>()*/)
{
	UILLOnlineSessionClient* OSC = GetOnlineSession();
	OSC->HandleDisconnect(World, NetDriver);
	bMatchmakeAfterReturnToMenu = true;
#if MATCHMAKING_SUPPORTS_HTTP_API
	PendingMatchmakingPlayerSessions = PlayerSessions;
#endif
	return true;
}

bool UILLOnlineMatchmakingClient::ConsumeMatchmakeAtEntry(UObject* WorldContextObject)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (UILLGameInstance* GI = CastChecked<UILLGameInstance>(World->GetGameInstance()))
		{
			UILLOnlineMatchmakingClient* OMC = Cast<UILLOnlineMatchmakingClient>(GI->GetOnlineMatchmaking());
			if (OMC->bMatchmakeAfterReturnToMenu)
			{
				// Consume the request
				UILLOnlineSessionClient* OSC = CastChecked<UILLOnlineSessionClient>(GI->GetOnlineSession());
				OMC->bMatchmakeAfterReturnToMenu = false;

				// Party members should just chill while their leader does it
				if (OSC->IsJoiningParty() || OSC->IsInPartyAsMember())
					return false;

				return true;
			}
		}
	}

	return false;
}

#if MATCHMAKING_SUPPORTS_HTTP_API
void UILLOnlineMatchmakingClient::ReplacementRegionsHandler(bool bSuccess, const TArray<FILLHTTPMatchmakingEndpoint>& Endpoints)
{
	check(ReplacementRegionsHTTPRequest);

	if (bSuccess)
	{
		ReplacementEndpoints = Endpoints;
	}
	else
	{
		// Clear the ReplacementEndpoints on failure
		ReplacementEndpoints.Empty();
	}

	// Now attempt to find a replacement
	if (!BeginRequestServerReplacement())
	{
		// Abort instance transition, kicking all players back to matchmaking...
		AILLGameMode* GameMode = GetWorld()->GetAuthGameMode<AILLGameMode>();
		GameMode->RecycleReplacementFailed();
	}

	// NULL this out AFTER the BeginRequestServerReplacement call...
	ReplacementRegionsHTTPRequest = nullptr;
}

void UILLOnlineMatchmakingClient::SendReplacementRegionsRequest()
{
	check(!ReplacementRegionsHTTPRequest);

	// Build and send the request
	FILLOnMatchmakingRegionResponseDelegate Delegate = FILLOnMatchmakingRegionResponseDelegate::CreateUObject(this, &ThisClass::ReplacementRegionsHandler);
	ReplacementRegionsHTTPRequest = BuildMatchmakingRegionsRequest(Delegate);
	ReplacementRegionsHTTPRequest->QueueRequest();
}

void UILLOnlineMatchmakingClient::ReplacementQueueHandler(bool bSuccess, const FString& RequestId, float RequestDelay)
{
	check(ReplacementQueueHTTPRequest);
	ReplacementQueueHTTPRequest = nullptr;

	check(ReplacementRequestId.IsEmpty());
	check(!TimerHandle_SendReplacementDescribe.IsValid());

	UWorld* World = GetWorld();

	if (bSuccess && ensure(!RequestId.IsEmpty()))
	{
		// Set a timer to describe
		ReplacementRequestId = RequestId;
		ReplacementDescribeDelay = RequestDelay;
		World->GetTimerManager().SetTimer(TimerHandle_SendReplacementDescribe, this, &ThisClass::SendReplacementDescribeRequest, ReplacementDescribeDelay);
	}
	else
	{
		// No session found or join attempt failed
		ReplacementRequestId.Empty();
		ReplacementStartRealTime = -1.f;

		// Abort instance transition, kicking all players back to matchmaking...
		AILLGameMode* GameMode = World->GetAuthGameMode<AILLGameMode>();
		GameMode->RecycleReplacementFailed();
	}
}

void UILLOnlineMatchmakingClient::SendReplacementDescribeRequest()
{
	check(!ReplacementRequestId.IsEmpty());
	check(!ReplacementQueueHTTPRequest);
	check(!ReplacementDescribeHTTPRequest);
	check(TimerHandle_SendReplacementDescribe.IsValid());
	TimerHandle_SendReplacementDescribe.Invalidate();

	// Build a delegate and send the request
	FILLOnMatchmakingDescribeResponseDelegate Delegate = FILLOnMatchmakingDescribeResponseDelegate::CreateUObject(this, &ThisClass::ReplacementDescribeHandler);
	ReplacementDescribeHTTPRequest = BuildMatchmakingDescribeRequest(ReplacementRequestId, Delegate);
	ReplacementDescribeHTTPRequest->QueueRequest();
}

void UILLOnlineMatchmakingClient::ReplacementDescribeHandler(bool bFinished, const TArray<FILLMatchmadePlayerSession>& PlayerSessions)
{
	check(ReplacementDescribeHTTPRequest);
	ReplacementDescribeHTTPRequest = nullptr;

	UWorld* World = GetWorld();

	if (bFinished)
	{
		UE_LOG(LogOnlineGame, Verbose, TEXT("%s: Finished!"), ANSI_TO_TCHAR(__FUNCTION__));

		struct FILLPartiedPlayerSession
		{
			AILLLobbyBeaconMemberState* LeaderMemberState;
			TArray<FILLMatchmadePlayerSession> MemberSessions;
		};
		TArray<FILLPartiedPlayerSession> PartiedPlayerSessions;

		// Organize PlayerSessions into parties
		UILLOnlineSessionClient* OSC = GetOnlineSession();
		AILLLobbyBeaconState* BeaconState = OSC->GetLobbyBeaconState();
		for (const FILLMatchmadePlayerSession& PlayerSession : PlayerSessions)
		{
			if (AILLLobbyBeaconMemberState* MemberState = Cast<AILLLobbyBeaconMemberState>(BeaconState->FindMember(PlayerSession.AccountId)))
			{
				AILLLobbyBeaconMemberState* LeaderState = MemberState->GetPartyLeaderMemberState();
				if (LeaderState)
				{
					if (FILLPartiedPlayerSession* ExistingEntry = PartiedPlayerSessions.FindByPredicate([&](FILLPartiedPlayerSession& Existing) { return (Existing.LeaderMemberState == LeaderState); }))
					{
						ExistingEntry->MemberSessions.Add(PlayerSession);
						continue;
					}
				}
				else
				{
					LeaderState = MemberState;
				}

				FILLPartiedPlayerSession Entry;
				Entry.LeaderMemberState = LeaderState;
				Entry.MemberSessions.Add(PlayerSession);
				PartiedPlayerSessions.Add(Entry);
			}
		}

		// Now tell the parties to migrate
		for (const FILLPartiedPlayerSession& PartySession : PartiedPlayerSessions)
		{
			if (AILLLobbyBeaconClient* LobbyClient = Cast<AILLLobbyBeaconClient>(PartySession.LeaderMemberState->GetBeaconClientActor()))
			{
				LobbyClient->MigrateToReplacement(PartySession.MemberSessions);
			}
		}

		// Tell the server to exit soon
		World->GetAuthGameMode<AILLGameMode>()->DelayedRecycleExit();
	}
	else
	{
		// Check if we should try again
		const float ReplacementTime = World->GetRealTimeSeconds() - ReplacementStartRealTime;
		if (ReplacementGiveUpTime > 0.f && ReplacementTime > ReplacementGiveUpTime)
		{
			UE_LOG(LogOnlineGame, Warning, TEXT("%s: ReplacementGiveUpTime (%3.2fs) hit! Giving up."), ANSI_TO_TCHAR(__FUNCTION__), ReplacementGiveUpTime);

			// No session found or join attempt failed
			ReplacementRequestId.Empty();
			ReplacementStartRealTime = -1.f;

			// Abort instance transition, kicking all players back to matchmaking...
			AILLGameMode* GameMode = World->GetAuthGameMode<AILLGameMode>();
			GameMode->RecycleReplacementFailed();
		}
		else
		{
			// Set a timer to describe again
			World->GetTimerManager().SetTimer(TimerHandle_SendReplacementDescribe, this, &ThisClass::SendReplacementDescribeRequest, ReplacementDescribeDelay);
		}
	}
}
#endif

void UILLOnlineMatchmakingClient::OnFinishedLeavingPartySession()
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Stop simulated matchmaking
	SetMatchmakingState(EILLMatchmakingState::NotRunning);
#if MATCHMAKING_USES_MATCHMAKING_API
	if (MatchmakingSearchDelegateHandle.IsValid())
	{
		IOnlineSessionPtr Sessions = GetSessionInt();
		check(MatchmakingSearchDelegateHandle.IsValid());
		Sessions->ClearOnMatchmakingCompleteDelegate_Handle(MatchmakingSearchDelegateHandle);
	}
#endif
}

void UILLOnlineMatchmakingClient::OnFinishedLeavingPartyForMainMenu()
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Stop simulated matchmaking
	SetMatchmakingState(EILLMatchmakingState::NotRunning);
#if MATCHMAKING_USES_MATCHMAKING_API
	if (MatchmakingSearchDelegateHandle.IsValid())
	{
		IOnlineSessionPtr Sessions = GetSessionInt();
		check(MatchmakingSearchDelegateHandle.IsValid());
		Sessions->ClearOnMatchmakingCompleteDelegate_Handle(MatchmakingSearchDelegateHandle);
	}
#endif
}

void UILLOnlineMatchmakingClient::OnPartyMatchmakingStarted()
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Simulate the start of matchmaking so MatchmakingStartRealTime is set
	BeginMatchmaking(false);

#if MATCHMAKING_USES_MATCHMAKING_API
	// Listen for matchmaking completion so we follow our party leader into a found match
	if (!MatchmakingSearchDelegateHandle.IsValid())
	{
		IOnlineSessionPtr Sessions = GetSessionInt();
		MatchmakingSearchDelegateHandle = Sessions->AddOnMatchmakingCompleteDelegate_Handle(MatchmakingSearchDelegate);
	}
#endif
}

void UILLOnlineMatchmakingClient::OnPartyMatchmakingUpdated(const EILLMatchmakingState NewState)
{
	// Assume the same MatchmakingState as the leader
	SetMatchmakingState(NewState);
}

void UILLOnlineMatchmakingClient::OnPartyMatchmakingStopped()
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

#if MATCHMAKING_USES_MATCHMAKING_API
	// Cleanup for OnPartyMatchmakingStarted
	// Since this function is called when the party leader cancels, it serves as our cleanup for when we do not join a match
	if (MatchmakingSearchDelegateHandle.IsValid())
	{
		IOnlineSessionPtr Sessions = GetSessionInt();
		Sessions->ClearOnMatchmakingCompleteDelegate_Handle(MatchmakingSearchDelegateHandle);
	}
#endif
}

#if !UE_BUILD_SHIPPING
void UILLOnlineMatchmakingClient::CmdQuickPlayCreateLAN()
{
	UILLGameBlueprintLibrary::ShowLoadingAndTravel(this, GetOuterUILLGameInstance()->PickRandomQuickPlayMap(), true, TEXT("listen?bIsLANMatch"));
}

void UILLOnlineMatchmakingClient::CmdQuickPlayCreateListen()
{
	UILLGameBlueprintLibrary::ShowLoadingAndTravel(this, GetOuterUILLGameInstance()->PickRandomQuickPlayMap(), true, TEXT("listen"));
}

void UILLOnlineMatchmakingClient::CmdTestUDPEcho(const TArray<FString>& Args)
{
	ILLUDPEcho(Args[0], 1.f, [](FIcmpEchoResult Result)
	{
		UE_LOG(LogOnlineGame, Log, TEXT("UDP echo took %f"), Result.Time);
	});
}
#endif

#undef LOCTEXT_NAMESPACE
