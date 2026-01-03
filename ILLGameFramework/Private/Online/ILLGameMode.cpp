// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLGameMode.h"

#include "Net/OnlineEngineInterface.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionInterface.h"
#include "OnlineSubsystemUtils.h"

#include "ILLBackendRequestManager.h"
#include "ILLGameBlueprintLibrary.h"
#include "ILLGameEngine.h"
#include "ILLGameInstance.h"
#include "ILLGameSession.h"
#include "ILLGameState.h"
#include "ILLHUD.h"
#include "ILLLobbyBeaconHostObject.h"
#include "ILLLobbyBeaconMemberState.h"
#include "ILLLobbyBeaconState.h"
#include "ILLOnlineMatchmakingClient.h"
#include "ILLOnlineSessionClient.h"
#include "ILLPlayerController.h"
#include "ILLPlayerState.h"

AILLGameMode::AILLGameMode(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	HUDClass = AILLHUD::StaticClass();
	PlayerControllerClass = AILLPlayerController::StaticClass();
	PlayerStateClass = AILLPlayerState::StaticClass();
//	SpectatorClass = AILLSpectatorPawn::StaticClass(); // TODO: pjackson
	GameStateClass = AILLGameState::StaticClass();

	RecycleExitDelay = 60.f;
	TimeEmptyBeforeEndMatch = 1800;

	TimerDeltaSeconds = 1.f;

	SessionUpdateDelay = 30.f;
	SessionInitialUpdateDelay = 1.f;
}

void AILLGameMode::Destroyed()
{
	Super::Destroyed();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerHandle_RecycleExit);
	}

	// Pointer guard for compiling BP GameModes
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UILLOnlineSessionClient* OSC = Cast<UILLOnlineSessionClient>(GameInstance->GetOnlineSession()))
		{
			// Cleanup lobby membership delegates
			OSC->OnLobbyMemberAdded().RemoveAll(this);
			OSC->OnLobbyMemberRemoved().RemoveAll(this);
		}
	}
}

void AILLGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Accumulate DeltaSeconds until we hit a second, at which point we run the GameSecondTimer once
	TimerDeltaSeconds -= DeltaSeconds;
	if (TimerDeltaSeconds <= 0.f)
	{
		TimerDeltaSeconds = GetWorldSettings()->GetEffectiveTimeDilation();
		GameSecondTimer();
	}

	// See if we should perform a game session update
	bool bSendGameSessionUpdate = false;
	if (!bHasSentInitialSessionUpdate)
	{
		if (GetWorld()->GetTimeSeconds()-CreationTime >= SessionInitialUpdateDelay)
		{
			bSendGameSessionUpdate = true;
			bHasSentInitialSessionUpdate = true;
		}
	}
	else
	{
		SessionUpdateTimer -= DeltaSeconds;
		bSendGameSessionUpdate = (SessionUpdateTimer <= 0.f);
	}

	if (bSendGameSessionUpdate)
	{
		UpdateGameSession();
	}
}

void AILLGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	if (ErrorMessage.IsEmpty())
	{
		// Authorize them
		UILLOnlineSessionClient* OSC = CastChecked<UILLOnlineSessionClient>(GetGameInstance()->GetOnlineSession());
		OSC->FullApproveLogin(Options, Address, UniqueId, ErrorMessage);
	}

	if (ErrorMessage.IsEmpty())
	{
		// Force a session update
		QueueGameSessionUpdate();
	}
}

APlayerController* AILLGameMode::Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal, const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	check(ErrorMessage.IsEmpty());

	// Lookup the AccountId
	const FILLBackendPlayerIdentifier AccountId(UGameplayStatics::ParseOption(Options, TEXT("AccountId")));

	// Look up the linked lobby client
	AILLLobbyBeaconClient* LinkedLobbyClient = nullptr;
	if (AccountId.IsValid() || UniqueId.IsValid())
	{
		// Link them to their lobby client
		UILLOnlineSessionClient* OSC = CastChecked<UILLOnlineSessionClient>(GetGameInstance()->GetOnlineSession());
		if (AILLLobbyBeaconState* LobbyHostState = OSC->IsHostingLobbyBeacon() ? OSC->GetLobbyBeaconState() : nullptr)
		{
			AILLSessionBeaconMemberState* MemberState = LobbyHostState->FindMember(AccountId);
			if (!MemberState)
				MemberState = LobbyHostState->FindMember(UniqueId);
			if (MemberState)
				LinkedLobbyClient = CastChecked<AILLLobbyBeaconClient>(MemberState->GetBeaconClientActor(), ECastCheckedType::NullAllowed);
		}
	}

	// Log the new player in
	APlayerController* Result = Super::Login(NewPlayer, InRemoteRole, Portal, Options, UniqueId, ErrorMessage);
	if (ErrorMessage.IsEmpty() && ensure(Result))
	{
		AILLPlayerController* PC = CastChecked<AILLPlayerController>(Result);

		// Store the linked lobby client
		PC->LinkedLobbyClient = LinkedLobbyClient;

		// Pass along the backend account ID to the new PC
		PC->SetBackendAccountId(AccountId);
	}

	return Result;
}

void AILLGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// Force a session update
	QueueGameSessionUpdate();
}

FString AILLGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal/* = TEXT("")*/)
{
	check(NewPlayerController);
	check(NewPlayerController->PlayerState);

	AILLPlayerController* PC = CastChecked<AILLPlayerController>(NewPlayerController);
	if (AILLPlayerState* NewPlayerState = Cast<AILLPlayerState>(PC->PlayerState))
	{
		// By this point AILLGameMode::Login will have parsed the backend account identifier
		// This might seem like an odd spot for this, but it's following how the Super call hits AGameSession::RegisterPlayer, which assigns the UniqueId to the PlayerState
		NewPlayerState->SetAccountId(PC->GetBackendAccountId());
	}

	return Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
}

bool AILLGameMode::SetPause(APlayerController* PC, FCanUnpause CanUnpauseDelegate/* = FCanUnpause()*/)
{
	if (!Super::SetPause(PC, CanUnpauseDelegate))
	{
		PendingPausers.Add(CastChecked<AILLPlayerController>(PC), CanUnpauseDelegate);
		return false;
	}

	PendingPausers.Empty();
	return true;
}

bool AILLGameMode::ClearPause()
{
	if (Super::ClearPause())
	{
		// Not paused? Clear PendingPausers
		PendingPausers.Empty();
		return true;
	}

	return false;
}

void AILLGameMode::ForceClearUnpauseDelegates(AActor* PauseActor)
{
	if (PauseActor)
	{
		for (int32 PauserIdx = Pausers.Num()-1; PauserIdx >= 0; --PauserIdx)
		{
			FCanUnpause& CanUnpauseDelegate = Pausers[PauserIdx];
			if (CanUnpauseDelegate.GetUObject() == PauseActor)
			{
				Pausers.RemoveAt(PauserIdx);
			}
		}
	}

	Super::ForceClearUnpauseDelegates(PauseActor);
}

void AILLGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	// Check if we can pause the game now
	AttemptFlushPendingPause();
}

void AILLGameMode::StartPlay()
{
	// Handle game session initialization
	UWorld* World = GetWorld();
	UILLOnlineSessionClient* OSC = CastChecked<UILLOnlineSessionClient>(GetGameInstance()->GetOnlineSession());
	if (GetNetMode() == NM_Standalone)
	{
		// Ensure our game session is cleaned up
		OSC->OnRegisteredStandalone();
	}
	else if (UOnlineEngineInterface::Get()->DoesSessionExist(World, NAME_GameSession))
	{
		// RegisterServer was skipped in the Super call, but we need to resume our lobby beacon
		OSC->ResumeGameSession();
	}
	else if (GetNetMode() == NM_DedicatedServer || GetNetMode() == NM_ListenServer)
	{
		AILLGameSession* GS = CastChecked<AILLGameSession>(GameSession);
		TSharedPtr<FILLGameSessionSettings> HostSettings = GS->GenerateHostSettings();

		if (OSC->HostGameSession(HostSettings))
		{
			UE_LOG(LogOnlineGame, Verbose, TEXT("%s: Created a game session"), ANSI_TO_TCHAR(__FUNCTION__));
		}
		else
		{
			UE_LOG(LogOnlineGame, Error, TEXT("%s: Failed to create server session!"), ANSI_TO_TCHAR(__FUNCTION__));
		}
	}

	// Catch any previously-registered AILLLobbyBeaconMemberState actors
	const TArray<AILLLobbyBeaconMemberState*>& LobbyMemberList = OSC->GetLobbyMemberList();
	for (AILLLobbyBeaconMemberState* MemberEntry : LobbyMemberList)
	{
		if (IsValid(MemberEntry) && MemberEntry->bHasSynchronized)
			AddLobbyMemberState(MemberEntry);
	}

	// Listen for any subsequent changes
	OSC->OnLobbyMemberAdded().AddDynamic(this, &ThisClass::AddLobbyMemberState);
	OSC->OnLobbyMemberRemoved().AddDynamic(this, &ThisClass::RemoveLobbyMemberState);

	// Call the super after we have started establishing a game session
	Super::StartPlay();
}

void AILLGameMode::RestartGame()
{
	UWorld* World = GetWorld();
	if (World->IsPlayInEditor())
	{
		return;
	}
	if (GetMatchState() == MatchState::LeavingMap)
	{
		return;
	}

	// Pending exit for recycle/replacement? Do not start a replacement request again
	if (TimerHandle_RecycleExit.IsValid())
		return;

	UILLGameInstance* GI = Cast<UILLGameInstance>(World->GetGameInstance());
	AILLGameSession* GS = Cast<AILLGameSession>(GameSession);
	if (GI->IsPendingRecycling())
	{
		// Attempt to find a replacement if that is supported
		UILLOnlineMatchmakingClient* OMC = GI->GetOnlineMatchmaking();
		if (!OMC->PendingServerReplacement() && !OMC->BeginRequestServerReplacement())
		{
			RecycleReplacementFailed();
		}
	}
	else if (GS->CanRestartGame())
	{
		FString LevelToLoad = UGameMapsSettings::GetServerDefaultMap();
		if (GS->IsQuickPlayMatch())
		{
			// Skip the lobby in quick play matches
			LevelToLoad = GI->PickRandomQuickPlayMap();
		}
		const FString LevelOptions = GS->GetPersistentTravelOptions();
		UILLGameBlueprintLibrary::ShowLoadingAndTravel(this, LevelToLoad, true, LevelOptions);
	}
}

TSubclassOf<AGameSession> AILLGameMode::GetGameSessionClass() const
{
	return AILLGameSession::StaticClass();
}

bool AILLGameMode::ReadyToStartMatch_Implementation()
{
	AILLGameSession* GS = Cast<AILLGameSession>(GameSession);
	if (!GS || !GS->IsReadyToStartMatch())
	{
		return false;
	}

	return Super::ReadyToStartMatch_Implementation();
}

void AILLGameMode::Logout(AController* Exiting)
{
	// Only logout players from the beacon and game session once it is InProgress, because we do not allow beacon connections and session registration to occur until then
	// Otherwise this may remove the local player in the event the create a game session on the main menu then transition to the map to play, as is the case with XB1 SmartMatch
	IOnlineSessionPtr Sessions = Online::GetSessionInterface();
	FNamedOnlineSession* Session = Sessions.IsValid() ? Sessions->GetNamedSession(NAME_GameSession) : nullptr;
	if (Session && Session->SessionState == EOnlineSessionState::InProgress)
	{
		if (AILLPlayerController* PC = Cast<AILLPlayerController>(Exiting))
		{
			// Do not log them out when changing maps for non-seamless travel
			if (GetMatchState() != MatchState::LeavingMap && PC->GetBackendAccountId().IsValid())
			{
				// Drop the user from the lobby beacon immediately
				// Otherwise session presence timeouts are the only thing to catch this
				UILLOnlineSessionClient* OSC = GetGameInstance() ? Cast<UILLOnlineSessionClient>(GetGameInstance()->GetOnlineSession()) : nullptr;
				if (OSC ? (OSC->IsHostingLobbyBeacon() && PC->LinkedLobbyClient.IsValid()) : false)
				{
					OSC->GetLobbyHostBeaconObject()->DisconnectClient(PC->LinkedLobbyClient.Get());
				}
			}
		}

		if (AILLPlayerState* PS = Cast<AILLPlayerState>(Exiting->PlayerState))
		{
			// Special handling for player state game session un-registration:
			// Call to the AILLPlayerState::UnregisterPlayerWithSession so that behavior simulates for the host
			// Otherwise GameSession::UnregisterPlayer will do so first
			PS->UnregisterPlayerWithSession();
		}
	}

	Super::Logout(Exiting);

	// Force a session update
	QueueGameSessionUpdate();
}

void AILLGameMode::GetSeamlessTravelActorList(bool bToTransition, TArray<AActor*>& ActorList)
{
	Super::GetSeamlessTravelActorList(bToTransition, ActorList);
}

void AILLGameMode::PostSeamlessTravel()
{
	Super::PostSeamlessTravel();

	// Let blueprint know that we are done seamlessly traveling for some reason
	K2_PostSeamlessTravel();

	// Force a session update
	QueueGameSessionUpdate();
}

void AILLGameMode::GenericPlayerInitialization(AController* C)
{
	Super::GenericPlayerInitialization(C);

	// Have late-joining players who get here after the match has started register with the session too (@see: AGameSession::OnStartOnlineGameComplete)
	if (AILLGameSession* GS = Cast<AILLGameSession>(GameSession))
	{
		if (GS->IsReadyToStartMatch())
		{
			if (AILLPlayerController* PC = Cast<AILLPlayerController>(C))
			{
				IOnlineSessionPtr Sessions = Online::GetSessionInterface();
				FNamedOnlineSession* Session = Sessions.IsValid() ? Sessions->GetNamedSession(NAME_GameSession) : nullptr;
				if (Session && Session->SessionState == EOnlineSessionState::InProgress)
				{
					// Tell them to also start their game session
					PC->ClientStartOnlineSession();
				}

				// Force a session update
				QueueGameSessionUpdate();
			}
		}
	}
}

void AILLGameMode::HandleMatchIsWaitingToStart()
{
	AILLGameState* GS = GetWorld()->GetGameState<AILLGameState>();
	if (GS && GS->bDeferBeginPlay)
	{
		// Do NOT call the Super function. We do not want BeginPlay firing immediately. @see AILLGameState::HandleMatchIsWaitingToStart.
		// We need to setup the GameSession the same, though...
		if (GameSession)
		{
			GameSession->HandleMatchIsWaitingToStart();
		}
	}
	else
	{
		Super::HandleMatchIsWaitingToStart();
	}

	// Force a session update
	QueueGameSessionUpdate();
}

void AILLGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	// Force a session update
	QueueGameSessionUpdate();
}

void AILLGameMode::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();

	// Update instance recycling for match completion
	UILLGameInstance* GI = Cast<UILLGameInstance>(GetGameInstance());
	GI->CheckInstanceRecycle();

	// Force a session update
	QueueGameSessionUpdate();
}

void AILLGameMode::HandleLeavingMap()
{
	Super::HandleLeavingMap();

	if (AILLGameSession* GS = Cast<AILLGameSession>(GameSession))
	{
		GS->HandleLeavingMap();
	}
}

bool AILLGameMode::AllowCheats(APlayerController* P)
{
#if UE_BUILD_SHIPPING
	return Super::AllowCheats(P);
#else
	return true;
#endif
}

void AILLGameMode::AddInactivePlayer(APlayerState* PlayerState, APlayerController* PC)
{
	// NOTE: Does nothing on purpose! AILLPlayerState::bHasRegisteredWithGameSession and bIsHost do not get along with this, and we simply don't need it in our games
}

void AILLGameMode::ConditionalBroadcast(AActor* Sender, const FString& Msg, FName Type)
{
	if (Msg.IsEmpty())
		return;

	APlayerState* FoundPlayerState = nullptr;
	if (Cast<APawn>(Sender))
	{
		FoundPlayerState = Cast<APawn>(Sender)->PlayerState;
	}
	else if (Cast<AController>(Sender))
	{
		FoundPlayerState = Cast<AController>(Sender)->PlayerState;
	}

	if (AILLPlayerState* SenderPlayerState = Cast<AILLPlayerState>(FoundPlayerState))
	{
		for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			if (AILLPlayerController* PC = Cast<AILLPlayerController>(*Iterator))
			{
				if (PC->CanReceiveTeamMessage(SenderPlayerState, Type))
				{
					PC->ClientTeamMessage(SenderPlayerState, Msg, Type);
				}
			}
		}
	}
}

int32 AILLGameMode::GetNumPendingConnections(const bool bWaitForFullyTraveled/* = true*/) const
{
	int32 Result = 0;

	UWorld* World = GetWorld();
	if (UNetDriver* NetDriver = World->GetNetDriver())
	{
		for (UNetConnection* Connection : NetDriver->ClientConnections)
		{
			if (!Connection)
				continue;

			// Welcomed connections without a PlayerController have gone as far as PreLogin but not as far as Login
			// Player controllers that have not fully traveled and synchronized will be considered pending
			AILLPlayerController* PC = Cast<AILLPlayerController>(Connection->PlayerController);
			if (Connection->ClientLoginState == EClientLoginState::Welcomed && (!PC || (bWaitForFullyTraveled && !PC->HasFullyTraveled())))
			{
				++Result;
			}
		}
	}

	return Result;
}

bool AILLGameMode::HasEveryPlayerLoadedIn(const bool bCheckLobbyBeacon/* = true*/) const
{
	// Since GetNumPendingConnections is checking HasFullyTraveled, we do not need to do that here
	UWorld* World = GetWorld();
	if (GetNumPendingConnections() == 0 && !World->IsInSeamlessTravel())
	{
		if (UILLOnlineSessionClient* OSC = GetGameInstance() ? Cast<UILLOnlineSessionClient>(GetGameInstance()->GetOnlineSession()) : nullptr)
		{
			// Check if we are still loading locally
			if (OSC->bLoadingMap)
				return false;

			// Check the lobby beacon for any pending players
			if (bCheckLobbyBeacon && OSC->IsHostingLobbyBeacon())
			{
				if (!OSC->GetLobbyBeaconState())
					return false;

				// When we have fewer player controllers than accepted lobby beacon members, someone is still loading
				int32 NumPlayerControllers = 0;
				for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
				{
					++NumPlayerControllers;
				}
				if (NumPlayerControllers < OSC->GetLobbyBeaconState()->GetNumAcceptedMembers())
					return false;

				// Check if we have any pending beacon members
				if (OSC->GetLobbyBeaconState()->HasAnyPendingMembers())
					return false;
			}
		}

		return true;
	}

	return false;
}

void AILLGameMode::RecycleReplacementFailed()
{
	UWorld* World = GetWorld();

	// Notify all ILLPlayerControllers to return to matchmaking
	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		if (AILLPlayerController* PC = Cast<AILLPlayerController>(*Iterator))
		{
			PC->ClientReturnToMatchMaking();
		}
	}

	// Suicide later
	DelayedRecycleExit();
}

void AILLGameMode::DelayedRecycleExit()
{
	// Set a timer to RecycleExit, allows the network to flush
	// This will automatically be cleaned up in Destroyed so that listen hosts do not exit
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_RecycleExit, this, &ThisClass::RecycleExit, RecycleExitDelay);
}

void AILLGameMode::RecycleExit()
{
	GIsRequestingExit = true;
}

void AILLGameMode::AttemptFlushPendingPause()
{
	// When players are wanting a pause try and pause the game again
	if (PendingPausers.Num() > 0 && !GetWorldSettings()->Pauser && AllowPausing())
	{
		// Attempt to pause the game
		for (auto PendingIt = PendingPausers.CreateConstIterator(); PendingIt; ++PendingIt)
		{
			if (SetPause(PendingIt.Key(), PendingIt.Value()))
				break;
		}

		// Did we pause the game? Clear the pending pausers
		if (GetWorldSettings()->Pauser)
			PendingPausers.Empty();
	}
}

void AILLGameMode::GameSecondTimer()
{
	// Check our player activity status
	UWorld* World = GetWorld();
	UILLGameInstance* GI = World ? Cast<UILLGameInstance>(World->GetGameInstance()) : nullptr;
	if (GI && GI->SupportsInstanceRecycling() && TimeEmptyBeforeEndMatch > 0 && HasEveryPlayerLoadedIn() && !HasMatchEnded())
	{
		// Tally up the total number of players we have
		int32 TotalActivePlayers = 0;
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (AILLPlayerController* PC = Cast<AILLPlayerController>(*It))
			{
				++TotalActivePlayers;
			}
		}

		if (TotalActivePlayers == 0)
		{
			// When we have been without players long enough, end the match
			// Instance recycling should then run it's checks and self-destruct if we are a dedicated server
			++SecondsWithoutPlayers;
			if (SecondsWithoutPlayers == TimeEmptyBeforeEndMatch)
			{
				// Skip straight to WaitingPostMatch instead of calling EndMatch, otherwise if we never started due to nobody connecting/joining the server will sit waiting to recycle indefinitely!
				SetMatchState(MatchState::WaitingPostMatch);
			}
		}
		else
		{
			SecondsWithoutPlayers = 0;
		}
	}
	else
	{
		SecondsWithoutPlayers = 0;
	}
}

void AILLGameMode::UpdateGameSession()
{
	// Because this can be hit while joining a game session at the main menu, we can't check Role!
	if (GetNetMode() == NM_DedicatedServer || GetNetMode() == NM_ListenServer)
	{
		// Update session information
		if (UILLOnlineSessionClient* OSC = GetGameInstance() ? Cast<UILLOnlineSessionClient>(GetGameInstance()->GetOnlineSession()) : nullptr)
		{
			OSC->SendGameSessionUpdate();
		}
	}

	SessionUpdateTimer = SessionUpdateDelay;
}

void AILLGameMode::QueueGameSessionUpdate()
{
#if PLATFORM_PS4
	// PS4 is a bitch, so delay the update a little while
	UILLOnlineSessionClient* OSC = GetGameInstance() ? Cast<UILLOnlineSessionClient>(GetGameInstance()->GetOnlineSession()) : nullptr;
	AILLGameSession* GS = Cast<AILLGameSession>(GameSession);
	if (OSC && GS && OSC->GetLobbyMemberList().Num() >= GS->GetMaxPlayers())
	{
		SessionUpdateTimer = .1f;
	}
	else
	{
		SessionUpdateTimer = .5f;
	}
#else
	SessionUpdateTimer = 0.f;
#endif
}

bool AILLGameMode::IsOpenToMatchmaking() const
{
	AILLGameSession* MyGameSession = Cast<AILLGameSession>(GameSession);
	if (!MyGameSession)
		return false;

	// Only matchmake to QuickPlay matches
	if (!MyGameSession->IsQuickPlayMatch())
		return false;

	// Do not remain open when at capacity
	if (MyGameSession->AtCapacity(false))
		return false;

	UWorld* World = GetWorld();
	AILLGameState* MyGameState = Cast<AILLGameState>(GameState);
	if (!World || !MyGameState)
		return false;

	// Do not list servers about to recycle
	if (MyGameState->IsPendingInstanceRecycle())
		return false;

	// Handle Play Together QuickPlays special by remaining closed for a period of time in the first match
	// This flag should be intentionally "lost" between level changes so it only affects the first QuickPlay match
	if (World->URL.HasOption(TEXT("ForPlayTogether")))
	{
		const float MatchAge = World->GetTimeSeconds() - MyGameState->CreationTime;
		if (MatchAge < PlayTogetherHoldMatchmakingTime)
			return false;
	}

	return true;
}

void AILLGameMode::AddLobbyMemberState(AILLLobbyBeaconMemberState* MemberState)
{
	QueueGameSessionUpdate();
}

void AILLGameMode::RemoveLobbyMemberState(AILLLobbyBeaconMemberState* MemberState)
{
	QueueGameSessionUpdate();
}
