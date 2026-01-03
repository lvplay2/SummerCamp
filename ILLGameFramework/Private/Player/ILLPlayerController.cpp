// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLPlayerController.h"

#include "Engine/Console.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

#include "ILLAudioSettingsSaveGame.h"
#include "ILLBackendPlayer.h"
#include "ILLCharacter.h"
#include "ILLCheatManager.h"
#include "ILLGameMode.h"
#include "ILLGameOnlineBlueprintLibrary.h"
#include "ILLGameInstance.h"
#include "ILLGameSession.h"
#include "ILLGameState.h"
#include "ILLGameState_Lobby.h"
#include "ILLHUD.h"
#include "ILLInputSettingsSaveGame.h"
#include "ILLLobbyBeaconState.h"
#include "ILLLocalPlayer.h"
#include "ILLOnlineMatchmakingClient.h"
#include "ILLOnlineSessionClient.h"
#include "ILLPlayerState.h"
#include "ILLPlayerInput.h"

#include "SoundNodeLocalPlayer.h"

#define LOCTEXT_NAMESPACE "ILLFonic.IGF.ILLPlayerController"

FName AILLPlayerController::NAME_Say(TEXT("Say"));

UILLPlayerNotificationMessage::UILLPlayerNotificationMessage(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

UILLPlayerStateChatMessage::UILLPlayerStateChatMessage(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

AILLPlayerController::AILLPlayerController(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	CheatClass = UILLCheatManager::StaticClass();
	PlayerInputClass = UILLPlayerInput::StaticClass();
	SayLengthLimit = 192;
	MaxNotificationMessages = 32;
}

void AILLPlayerController::BeginDestroy()
{
	Super::BeginDestroy();
	
	if (UWorld* World = GetWorld())
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_AttemptOrRetryStartGameSession);
	}

	// Update USoundNodeLocalPlayer
	if (!GExitPurge)
	{
		const uint32 UniqueID = GetUniqueID();
		FAudioThread::RunCommandOnAudioThread([UniqueID]()
		{
			USoundNodeLocalPlayer::GetLocallyControlledActorCache().Remove(UniqueID);
		});
	}
}

void AILLPlayerController::Destroyed()
{
	// Force garbage collection to run on the next tick
	// Ensures any dangling SteamP2P sockets are flushed before creating new ones, there's a bug allow ones pending kill to be used!
	GEngine->DelayGarbageCollection();

	Super::Destroyed();
}

void AILLPlayerController::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);

	// Update USoundNodeLocalPlayer
	bool bLocallyControlled = IsLocalController();
	if (AILLCharacter* Char = Cast<AILLCharacter>(GetPawn()))
	{
		bLocallyControlled = bLocallyControlled || Char->IsLocalPlayerViewTarget();
	}
	const uint32 UniqueID = GetUniqueID();
	FAudioThread::RunCommandOnAudioThread([UniqueID, bLocallyControlled]()
	{
		USoundNodeLocalPlayer::GetLocallyControlledActorCache().Add(UniqueID, bLocallyControlled);
	});
}

bool AILLPlayerController::UseShortConnectTimeout() const
{
	if (Super::UseShortConnectTimeout())
	{
		// Wait for them to finish loading so they don't get kicked for taking too long on the short timeout (use InitialConnectTimeout)
		return HasFullyTraveled();
	}

	return false;
}

void AILLPlayerController::PostNetReceive()
{
	Super::PostNetReceive();

	// Update travel status
	HasFullyTraveled(true);
}

void AILLPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	BindPersistentAction(TEXT("GLB_Speak"), IE_Pressed, this, &ThisClass::StartTalking);
	BindPersistentAction(TEXT("GLB_Speak"), IE_Released, this, &ThisClass::StopTalking);
}

void AILLPlayerController::InitPlayerState()
{
	Super::InitPlayerState();

	// Acknowledge it
	AcknowledgePlayerState(PlayerState);
}

void AILLPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// Ack with the server
	AcknowledgePlayerState(PlayerState);
}

bool AILLPlayerController::IsMoveInputIgnored() const
{
	if (Super::IsMoveInputIgnored())
	{
		return true;
	}

	if (!IsGameInputAllowed())
	{
		return true;
	}

	return false;
}

bool AILLPlayerController::IsLookInputIgnored() const
{
	if (Super::IsLookInputIgnored())
	{
		return true;
	}

	if (!IsGameInputAllowed())
	{
		return true;
	}

	return false;
}

void AILLPlayerController::GetSeamlessTravelActorList(bool bToEntry, TArray<AActor*>& ActorList)
{
	Super::GetSeamlessTravelActorList(bToEntry, ActorList);

	if (!bSeamlessTravelHUD)
	{
		// Remove the HUD
		ActorList.Remove(MyHUD);
	}
}

void AILLPlayerController::SeamlessTravelFrom(class APlayerController* OldPC)
{
	Super::SeamlessTravelFrom(OldPC);

	if (AILLPlayerController* OldPlayer = Cast<AILLPlayerController>(OldPC))
	{
		// And the LinkedLobbyClient
		LinkedLobbyClient = OldPlayer->LinkedLobbyClient;

		// Persist the AccountID across travel
		SetBackendAccountId(OldPlayer->GetBackendAccountId());
	}
}

void AILLPlayerController::UpdatePing(float InPing)
{
	Super::UpdatePing(InPing);

	// Update travel status
	HasFullyTraveled(true);
}

void AILLPlayerController::ClientEnableNetworkVoice_Implementation(bool bEnable)
{
	// Intentionally not calling the Super because it's inflexible as shit
	// bEnable = !GameSession.bRequiresPushToTalk
	bSessionRequiresPushToTalk = !bEnable;

	UpdatePushToTalkTransmit();
}

void AILLPlayerController::ClientSetHUD_Implementation(TSubclassOf<AHUD> NewHUDClass)
{
	Super::ClientSetHUD_Implementation(NewHUDClass);

	if (ensure(MyHUD))
	{
		OnPlayerHUDCreated.Broadcast(Cast<AILLHUD>(MyHUD));
	}
}

void AILLPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// Update multiplayer feature usage for PS4
	UILLLocalPlayer* LP = Cast<UILLLocalPlayer>(Player);
	if (LP && LP->GetPreferredUniqueNetId().IsValid())
	{
		const bool bUsingMultiplayerFeatures = IsUsingMultiplayerFeatures();
		if (bUsedMultiplayerFeatures != bUsingMultiplayerFeatures || !bUpdatedMultiplayerFeatures)
		{
			UE_LOG(LogOnlineGame, Verbose, TEXT("AILLPlayerController bUsingMultiplayerFeatures changed to %s"), bUsingMultiplayerFeatures ? TEXT("true") : TEXT("false"));

			bUsedMultiplayerFeatures = bUsingMultiplayerFeatures;
			bUpdatedMultiplayerFeatures = true;
			IOnlineSubsystem::Get()->SetUsingMultiplayerFeatures(*LP->GetPreferredUniqueNetId(), bUsingMultiplayerFeatures);
		}
	}

	// Update fully traveled for offline modes
	HasFullyTraveled(true);
}

void AILLPlayerController::InitInputSystem()
{
	// Initialize PlayerInput BEFORE the Super call
	if (!PlayerInput)
	{
		PlayerInput = NewObject<UPlayerInput>(this, PlayerInputClass);
	}

	Super::InitInputSystem();

	if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(PlayerInput))
	{
		// Listen for gamepad use changes
		Input->OnUsingGamepadChanged.AddDynamic(this, &ThisClass::OnUsingGamepadChanged);

		// Fake an update now
		OnUsingGamepadChanged(Input->IsUsingGamepad());
	}

	UILLLocalPlayer* LocalPlayer = CastChecked<UILLLocalPlayer>(Player);
	if (UILLInputSettingsSaveGame* InputSettings = LocalPlayer->GetLoadedSettingsSave<UILLInputSettingsSaveGame>())
	{
		bCrouchToggle = InputSettings->bToggleCrouch;

		SetMouseSensitivity(InputSettings->MouseSensitivity);
		SetInvertedLook(InputSettings->bInvertedMouseLook, EKeys::MouseY);

		SetControllerHorizontalSensitivity(InputSettings->ControllerHorizontalSensitivity);
		SetControllerVerticalSensitivity(InputSettings->ControllerVerticalSensitivity);
		SetInvertedLook(InputSettings->bInvertedControllerLook, EKeys::Gamepad_RightY);
	}
}

void AILLPlayerController::SetInvertedLook(bool bInInvertedYAxis, const FKey KeyToInvert)
{
	if (PlayerInput)
	{
		if (bInInvertedYAxis != PlayerInput->GetInvertAxisKey(KeyToInvert))
		{
			PlayerInput->InvertAxisKey(KeyToInvert);
		}
	}
}

void AILLPlayerController::SetMouseSensitivity(float Value)
{
	if (PlayerInput)
	{
		PlayerInput->SetMouseSensitivity(Value);
	}
}

void AILLPlayerController::SetControllerHorizontalSensitivity(float Sensitivity)
{
	if (PlayerInput)
	{
		FInputAxisProperties ControllerAxisProps;
		if (PlayerInput->GetAxisProperties(EKeys::Gamepad_RightX, ControllerAxisProps))
		{
			ControllerAxisProps.Sensitivity = Sensitivity;
			PlayerInput->SetAxisProperties(EKeys::Gamepad_RightX, ControllerAxisProps);
		}
	}
}

void AILLPlayerController::SetControllerVerticalSensitivity(float Sensitivity)
{
	if (PlayerInput)
	{
		FInputAxisProperties ControllerAxisProps;
		if (PlayerInput->GetAxisProperties(EKeys::Gamepad_RightY, ControllerAxisProps))
		{
			ControllerAxisProps.Sensitivity = Sensitivity;
			PlayerInput->SetAxisProperties(EKeys::Gamepad_RightY, ControllerAxisProps);
		}
	}
}

void AILLPlayerController::ClientStartOnlineSession_Implementation()
{
	AttemptOrRetryStartGameSession();
}

void AILLPlayerController::ClientPlayForceFeedback_Implementation(UForceFeedbackEffect* ForceFeedbackEffect, bool bLooping, bool bIgnoreTimeDilation, FName Tag)
{
	if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(PlayerInput))
	{
		if (!Input->IsUsingGamepad())
			return;
	}

	UILLLocalPlayer* LocalPlayer = CastChecked<UILLLocalPlayer>(Player);
	if (UILLInputSettingsSaveGame* InputSettings = LocalPlayer->GetLoadedSettingsSave<UILLInputSettingsSaveGame>())
	{
		if (!InputSettings->bControllerVibrationEnabled)
			return;
	}

	Super::ClientPlayForceFeedback_Implementation(ForceFeedbackEffect, bLooping, bIgnoreTimeDilation, Tag);
}

void AILLPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalPlayerController())
	{
		bHasReceivedPlayer = true;
		ServerNotifyReceivedPlayer();

#if PLATFORM_XBOXONE
		// Take the XboxOne Kinect GPU reserve back, make it great again
		FXboxOneMisc::TakeKinectGPUReserve(bTakeKinectCPUReserve);
#endif

		// Login locally so that we have our AccountID too
		if (UILLBackendPlayer* BackendPlayer = GetLocalBackendPlayer())
		{
			SetBackendAccountId(BackendPlayer->GetAccountId());
		}
	}
}

void AILLPlayerController::SetCameraMode(FName NewCamMode)
{
	Super::SetCameraMode(NewCamMode);

	// Update mesh visibility on camera mode changes
	if (AILLCharacter* Char = Cast<AILLCharacter>(GetPawn()))
	{
		Char->UpdateCharacterMeshVisibility();
	}
}

bool AILLPlayerController::InputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad)
{
	// Only break into gamepad mode when this is a gamepad key
	if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(PlayerInput))
	{
		if (bGamepad)
		{
			Input->SetUsingGamepad(true);
		}
#if PLATFORM_DESKTOP
		else
		{
			// Only switch out of gamepad mode when a non-gamepad key is pressed on desktop platforms
			Input->SetUsingGamepad(false);
		}
#endif
	}

	if (AILLHUD* HUD = Cast<AILLHUD>(MyHUD))
	{
		if (HUD->HUDInputKey(Key, EventType, AmountDepressed, bGamepad))
		{
			return true;
		}
	}

	return Super::InputKey(Key, EventType, AmountDepressed, bGamepad);
}

bool AILLPlayerController::InputKey(FKey Key, int32 ControllerId, EInputEvent EventType, float AmountDepressed, bool bGamepad)
{
	if (AILLHUD* HUD = Cast<AILLHUD>(MyHUD))
	{
		UILLUserWidget* CurrentMenu = HUD->GetMenuStackComp() ? HUD->GetMenuStackComp()->GetCurrentMenu() : nullptr;
		if (CurrentMenu && CurrentMenu->bAllUserFocus)
		{
			// HACK: Route input along as if this player pressed it
			if (InputKey(Key, EventType, AmountDepressed, bGamepad))
				return true;
		}
	}

	return false;
}

bool AILLPlayerController::InputAxis(FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad)
{
	if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(PlayerInput))
	{
#if PLATFORM_WINDOWS
		if (!Input->IsUsingGamepad())
		{
			// When transitioning from not using a game pad to using one, ensure the Delta is greater than the typical dead zone
			// This mitigates XInput sending dumb near-zero axis whenever you Shift-TAB for the Steam overlay
			bGamepad = bGamepad && FMath::Abs(Delta) >= .25f;
		}
#endif

		Input->SetUsingGamepad(bGamepad);
	}

	if (AILLHUD* HUD = Cast<AILLHUD>(MyHUD))
	{
		if (HUD->HUDInputAxis(Key, Delta, DeltaTime, NumSamples, bGamepad))
		{
			return true;
		}
	}

	return Super::InputAxis(Key, Delta, DeltaTime, NumSamples, bGamepad);
}

void AILLPlayerController::ClientTeamMessage_Implementation(APlayerState* SenderPlayerState, const FString& S, FName Type, float MsgLifeTime)
{
	Super::ClientTeamMessage_Implementation(SenderPlayerState, S, Type, MsgLifeTime);

	if (AILLPlayerState* Sender = Cast<AILLPlayerState>(SenderPlayerState))
	{
		UILLPlayerStateChatMessage* Message = NewObject<UILLPlayerStateChatMessage>();
		Message->NotificationText = FText::FromString(S);
		Message->PlayerState = Sender;
		Message->Type = Type;
		Message->Lifetime = MsgLifeTime;
		HandleLocalizedNotification(Message);
	}
}

void AILLPlayerController::ClientReturnToMainMenu_Implementation(const FText& ReturnReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UILLGameInstance* GI = Cast<UILLGameInstance>(World->GetGameInstance()))
		{
			// Store off the kick reason for our return to the main menu
			if (!ReturnReason.IsEmpty())
			{
				GI->SetLastPlayerKickReason(FText::Format(LOCTEXT("GameKickedWithReason", "You were kicked: {0}"), ReturnReason)); // Same as ClientWasKicked
			}
		}
	}

	Super::ClientReturnToMainMenu_Implementation(ReturnReason);
}

void AILLPlayerController::ClientWasKicked_Implementation(const FText& KickReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UILLGameInstance* GI = Cast<UILLGameInstance>(World->GetGameInstance()))
		{
			// Store off the kick reason for our return to the main menu
			if (KickReason.IsEmpty())
			{
				GI->SetLastPlayerKickReason(LOCTEXT("GameKickedWithoutReason", "You were kicked!"));
			}
			else
			{
				GI->SetLastPlayerKickReason(FText::Format(LOCTEXT("GameKickedWithReason", "You were kicked: {0}"), KickReason)); // Same as ClientReturnToMainMenu
			}

			// Leave our party if we are only a member
			UILLOnlineSessionClient* OSC = CastChecked<UILLOnlineSessionClient>(GI->GetOnlineSession());
			if (OSC->IsInPartyAsMember())
			{
				OSC->LeavePartySession();
			}
		}
	}

	Super::ClientWasKicked_Implementation(KickReason);
}

void AILLPlayerController::StartTalking()
{
	if (!IsLocalPlayerUsingPushToTalk())
		return;

	Super::StartTalking();
}

void AILLPlayerController::StopTalking()
{
	if (!IsLocalPlayerUsingPushToTalk())
		return;

	Super::StopTalking();
}

void AILLPlayerController::ToggleSpeaking(bool bInSpeaking)
{
	if (bIsSpeaking != bInSpeaking)
	{
		bIsSpeaking = bInSpeaking;

		// Play an optional sound effect for the push to talk toggle
		bool bPlayPushToTalkSound = IsLocalPlayerUsingPushToTalk();
		if (bPlayPushToTalkSound)
		{
			UILLLocalPlayer* LocalPlayer = CastChecked<UILLLocalPlayer>(Player);
			if (UILLAudioSettingsSaveGame* AudioSettings = LocalPlayer->GetLoadedSettingsSave<UILLAudioSettingsSaveGame>())
			{
				bPlayPushToTalkSound = AudioSettings->bPlayPushToTalkSound;
			}
		}

		if (bPlayPushToTalkSound)
		{
			if (UWorld* World = GetWorld())
			{
				IOnlineSessionPtr SessionInt = Online::GetSessionInterface();
				if (SessionInt.IsValid() && SessionInt->GetNumSessions() > 0)
				{
					if (bIsSpeaking)
					{
						if (TalkingStartSound)
							UGameplayStatics::PlaySound2D(World, TalkingStartSound);
					}
					else
					{
						if (TalkingStopSound)
							UGameplayStatics::PlaySound2D(World, TalkingStopSound);
					}
				}
			}
		}
	}

	Super::ToggleSpeaking(bInSpeaking);
}

void AILLPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	Super::PostProcessInput(DeltaTime, bGamePaused);

	ProcessActions(PlayerInput);
	ProcessAxes(PlayerInput, bGamePaused);
}

bool AILLPlayerController::IsUsingMultiplayerFeatures() const
{
	// Can fire while at the main menu and in the process of joining a game session, so guard against that
	if (GetNetMode() == NM_Standalone)
		return false;

	// Wait for a non-LAN game session to be in-progress
	IOnlineSessionPtr SessionInt = Online::GetSessionInterface();
	FNamedOnlineSession* OnlineSession = SessionInt.IsValid() ? SessionInt->GetNamedSession(NAME_GameSession) : nullptr;
	if (OnlineSession && OnlineSession->SessionInfo.IsValid() && OnlineSession->SessionState == EOnlineSessionState::InProgress && !OnlineSession->SessionSettings.bIsLANMatch)
	{
		// Wait for at least 2 people in private games
		if (OnlineSession->SessionSettings.bShouldAdvertise || OnlineSession->RegisteredPlayers.Num() >= 2)
		{
			// Check the game state
			UWorld* World = GetWorld();
			AILLGameState* GameState = World ? World->GetGameState<AILLGameState>() : nullptr;
			return (GameState && GameState->IsUsingMultiplayerFeatures());
		}
	}

	return false;
}

bool AILLPlayerController::IsLaggingOut() const
{
	AILLPlayerState* PS = Cast<AILLPlayerState>(PlayerState);
	if (bLagging || !PS)
		return true;

	return (PS->GetRealPing() > 400); // CLEANUP: pjackson: Config?
}

void AILLPlayerController::ClientReturnToMatchMaking_Implementation()
{
	UWorld* World = GetWorld();
	if (UILLGameInstance* GameInstance = Cast<UILLGameInstance>(GetWorld()->GetGameInstance()))
	{
		UILLOnlineMatchmakingClient* OMC = GameInstance->GetOnlineMatchmaking();
		OMC->HandleDisconnectForMatchmaking(World, World->GetNetDriver());
	}
	else
	{
		GEngine->HandleDisconnect(World, World->GetNetDriver());
	}
}

void AILLPlayerController::ClientShowLoadingScreen_Implementation(const FURL& NextURL)
{
	if (UILLGameInstance* GameInstance = Cast<UILLGameInstance>(GetWorld()->GetGameInstance()))
	{
		GameInstance->ShowLoadingScreen(NextURL);
	}
}

bool AILLPlayerController::HasFullyTraveled(const bool bCanUpdate/* = false*/) const
{
	if (!bHasFullyTraveled && bCanUpdate)
	{
		check(IsInGameThread());

		// When this is true, they have ticked and sent an RPC once
		if (!Super::UseShortConnectTimeout()) // HACK: Skip the local call to avoid infinite recursion
			return false;

		// Received their LocalPlayer?
		if (!HasReceivedPlayer())
			return false;

		// Loaded the map?
		if (!HasClientLoadedCurrentWorld())
			return false;

		// Acknowledged their player state?
		if (!PlayerState || PlayerState != AcknowledgedPlayerState)
			return false;
		
		bHasFullyTraveled = true;
	}

	if (AILLPlayerState* MyPS = Cast<AILLPlayerState>(PlayerState))
	{
		// Has their player state fully synced too?
		if (!MyPS->HasFullyTraveled(bCanUpdate))
			return false;

		return bHasFullyTraveled;
	}

	return false;
}

bool AILLPlayerController::CanKickPlayer(AILLPlayerState* KickPlayerState) const
{
	AILLPlayerState* PS = Cast<AILLPlayerState>(PlayerState);
	if (PS && PS->IsHost() && KickPlayerState && !KickPlayerState->IsHost())
	{
		// Only allow kicking in LAN or Private matches
		UWorld* World = GetWorld();
		if(UILLGameOnlineBlueprintLibrary::IsCurrentGameSessionLAN(World)
		|| UILLGameOnlineBlueprintLibrary::IsCurrentGameSessionPrivate(World))
		{
			return true;
		}
	}

	return false;
}

bool AILLPlayerController::ServerRequestKickPlayer_Validate(AILLPlayerState* KickPlayerState)
{
	return true;
}

void AILLPlayerController::ServerRequestKickPlayer_Implementation(AILLPlayerState* KickPlayerState)
{
	if (!CanKickPlayer(KickPlayerState))
		return;

	if (AILLGameMode* GameMode = GetWorld()->GetAuthGameMode<AILLGameMode>())
	{
		if (AILLGameSession* GameSession = Cast<AILLGameSession>(GameMode->GameSession))
		{
			if (AILLPlayerController* KickingController = Cast<AILLPlayerController>(KickPlayerState->GetOwner()))
			{
				GameSession->KickPlayer(KickingController, LOCTEXT("KickedByHostMessage", "Kicked by host!"));
			}
		}
	}
}

void AILLPlayerController::CacheMenuStack()
{
	if (AILLHUD* HUD = Cast<AILLHUD>(GetHUD()))
	{
		if (UILLLocalPlayer* LP = Cast<UILLLocalPlayer>(GetLocalPlayer()))
		{
			LP->CachedMenuStack = HUD->GetMenuStackComp()->GetFullStack();
		}
	}
}

bool AILLPlayerController::RestoreMenuStack()
{
	if (UILLLocalPlayer* LP = Cast<UILLLocalPlayer>(GetLocalPlayer()))
	{
		if (LP->CachedMenuStack.Num() > 0)
		{
			if (AILLHUD* HUD = Cast<AILLHUD>(GetHUD()))
			{
				// Tell the hud to restore our cached menus.
				HUD->OverrideMenuStackComp(LP->CachedMenuStack);

				// Reset our cached data so we don't get stuck in a loading loop.
				LP->CachedMenuStack.Empty();
				return true;
			}
		}
	}

	return false;
}

void AILLPlayerController::AttemptOrRetryStartGameSession()
{
	if (!IsPrimaryPlayer())
		return;
	
	UWorld* World = GetWorld();
	if (!World)
		return;

	bool bStartedGameSession = false;
	if (PlayerState && GetGameInstance())
	{
		if (UILLOnlineSessionClient* OSC = Cast<UILLOnlineSessionClient>(GetGameInstance()->GetOnlineSession()))
		{
			IOnlineSessionPtr SessionInterface = Online::GetSessionInterface(World);
			if (SessionInterface.IsValid() && ensure(PlayerState->SessionName == NAME_GameSession))
			{
				if (FNamedOnlineSession* Session = SessionInterface->GetNamedSession(PlayerState->SessionName))
				{
					if (Session->SessionState == EOnlineSessionState::Pending || Session->SessionState == EOnlineSessionState::Ended)
					{
						OSC->StartOnlineSession(PlayerState->SessionName);
						bStartedGameSession = true;

						// Update push to talk now that voice should be registered
						UpdatePushToTalkTransmit();
					}
					else if (Session->SessionState == EOnlineSessionState::InProgress)
					{
						bStartedGameSession = true;
					}
				}
			}
		}
	}

	if (!bStartedGameSession)
	{
		// Keep retrying until everything is replicated
		World->GetTimerManager().SetTimer(TimerHandle_AttemptOrRetryStartGameSession, this, &AILLPlayerController::AttemptOrRetryStartGameSession, .1f, false);
	}
}

bool AILLPlayerController::FlushPendingInvite()
{
	if (IsPrimaryPlayer() && PlayerState && GetGameInstance())
	{
		if (UILLOnlineSessionClient* OnlineSession = Cast<UILLOnlineSessionClient>(GetGameInstance()->GetOnlineSession()))
		{
			if (OnlineSession->CheckPlayTogetherParty())
				return true;
			if (OnlineSession->HasDeferredInvite())
			{
				OnlineSession->QueueFlushDeferredInvite();
				return true;
			}
		}
	}

	return false;
}

void AILLPlayerController::AcknowledgePlayerState(APlayerState* PS)
{
	if (IsLocalController())
	{
		AcknowledgedPlayerState = PlayerState;
		ServerAcknowledgePlayerState(PlayerState);
	}
}

bool AILLPlayerController::ServerAcknowledgePlayerState_Validate(APlayerState* PS)
{
	return true;
}

void AILLPlayerController::ServerAcknowledgePlayerState_Implementation(APlayerState* PS)
{
	AcknowledgedPlayerState = PS;

	if (UWorld* World = GetWorld())
	{
		if (AILLPlayerState* MyPS = Cast<AILLPlayerState>(PS))
		{
			MyPS->bHasAckedPlayerState = true;
			MyPS->ForceNetUpdate();
		}
	}
}

bool AILLPlayerController::ServerNotifyReceivedPlayer_Validate()
{
	return true;
}

void AILLPlayerController::ServerNotifyReceivedPlayer_Implementation()
{
	bHasReceivedPlayer = true;
}

bool AILLPlayerController::CanReceiveTeamMessage(class AILLPlayerState* Sender, FName Type) const
{
	return true;
}

void AILLPlayerController::HandleLocalizedNotification(UILLPlayerNotificationMessage* Notification)
{
	check(Notification);

	// Check if we are over the limit and remove the oldest entry
	if (NotificationMessages.Num() >= MaxNotificationMessages)
	{
		NotificationMessages.RemoveAt(0);
	}

	// Add it
	NotificationMessages.Add(Notification);

	// Print to the console
	if (ULocalPlayer* LP = Cast<ULocalPlayer>(Player))
	{
		if (LP->ViewportClient && LP->ViewportClient->ViewportConsole)
		{
			LP->ViewportClient->ViewportConsole->OutputText(Notification->NotificationText.ToString());
		}
	}

	// Tell the people
	OnPlayerReceivedNotification.Broadcast(Notification);
}

void AILLPlayerController::Say(const FString& Message)
{
	// Trim off start/end whitespace and only send if the remainder is not empty
	FString TrimmedMessage(Message.Left(SayLengthLimit));
	TrimmedMessage.TrimStartInline();
	TrimmedMessage.TrimEndInline();
	if (!TrimmedMessage.IsEmpty())
	{
		ServerSay(TrimmedMessage);
	}
}

bool AILLPlayerController::ServerSay_Validate(const FString& Message)
{
	return true;
}

void AILLPlayerController::ServerSay_Implementation(const FString& Message)
{
	if (AILLGameMode* GM = Cast<AILLGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->ConditionalBroadcast(this, Message, NAME_Say);
	}
}

class UILLBackendPlayer* AILLPlayerController::GetLocalBackendPlayer() const
{
	if (IsLocalPlayerController())
	{
		if (UILLLocalPlayer* LocalPlayer = Cast<UILLLocalPlayer>(Player))
			return LocalPlayer->GetBackendPlayer();
	}

	return nullptr;
}

void AILLPlayerController::SetBackendAccountId(const FILLBackendPlayerIdentifier& InBackendAccountId)
{
	if (BackendAccountId == InBackendAccountId)
		return;

	BackendAccountId = InBackendAccountId;

	// Attempt to set LinkedLobbyClient locally
	if (!LinkedLobbyClient.IsValid())
	{
		UILLOnlineSessionClient* OSC = CastChecked<UILLOnlineSessionClient>(GetGameInstance()->GetOnlineSession());
		if (AILLLobbyBeaconState* LobbyState = OSC->GetLobbyBeaconState())
		{
			if (AILLSessionBeaconMemberState* MemberState = LobbyState->FindMember(BackendAccountId))
			{
				LinkedLobbyClient = CastChecked<AILLLobbyBeaconClient>(MemberState->GetBeaconClientActor(), ECastCheckedType::NullAllowed);
			}
		}
	}

	// Ensure the PlayerState is updated
	if (AILLPlayerState* PS = Cast<AILLPlayerState>(PlayerState))
	{
		PS->SetAccountId(BackendAccountId);
	}
}

FKey AILLPlayerController::FirstKeyForAction(const FName ActionName) const
{
	if (PlayerInput)
	{
		const TArray<FInputActionKeyMapping>& KeysForAction = PlayerInput->GetKeysForAction(ActionName);
		if (KeysForAction.Num() > 0)
		{
			return KeysForAction[0].Key;
		}
	}

	return EKeys::AnyKey;
}

void AILLPlayerController::ReturnToMainMenu(const FText& ReturnReason)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("AILLPlayerController::ReturnToMainMenu: %s"), *ReturnReason.ToString());

	if (UWorld* World = GetWorld())
	{
		if (UILLGameInstance* GI = Cast<UILLGameInstance>(World->GetGameInstance()))
		{
			GI->ShowLoadingScreen(FURL()); // FIXME: pjackson: ShowReturningToMainMenuLoadingScreen?
		}
	}

	ClientReturnToMainMenu(ReturnReason);
}

bool AILLPlayerController::IsGameInputAllowed() const
{
	if (!bAllowGameInputInCinematicMode && bCinematicMode)
	{
		return false;
	}

	return true;
}

void AILLPlayerController::OnUsingGamepadChanged(bool bUsingGamepad)
{
	UpdatePushToTalkTransmit();
}

void AILLPlayerController::Cheat(const FString& CheatCommand)
{
	if (Role < ROLE_Authority)
	{
		ServerCheat(CheatCommand);
	}
	else if (CheatManager)
	{
		Say(FString(TEXT("CHEAT ")) + CheatCommand);
		Say(ConsoleCommand(CheatCommand, false));
	}
}

bool AILLPlayerController::ServerCheat_Validate(const FString& CheatCommand)
{
	return true;
}

void AILLPlayerController::ServerCheat_Implementation(const FString& CheatCommand)
{
	Cheat(CheatCommand);
}

bool AILLPlayerController::IsLocalPlayerUsingPushToTalk() const
{
	if (!ensure(IsLocalPlayerController()) || !ensure(Player))
		return false;

#if PLATFORM_DESKTOP
	// This is only enabled on desktop right now, we need a separate gamepad push to talk option which respects if the game actually has that bound to a gamepad button (looking at you F13)
	UILLLocalPlayer* LocalPlayer = CastChecked<UILLLocalPlayer>(Player);
	if (UILLAudioSettingsSaveGame* AudioSettings = LocalPlayer->GetLoadedSettingsSave<UILLAudioSettingsSaveGame>())
	{
		if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(PlayerInput))
		{
			// Check for push to talk
			return AudioSettings->bUsePushToTalk;
		}
	}
#endif

	return false;
}

void AILLPlayerController::UpdatePushToTalkTransmit()
{
	if (!IsLocalPlayerController() || !Player)
		return;

	const bool bUsingPushToTalk = IsLocalPlayerUsingPushToTalk();
	if (bSessionRequiresPushToTalk || bUsingPushToTalk)
	{
		// Stop speaking if we are currently transmitting
		ToggleSpeaking(false);
	}
	else
	{
		// Automatically start speaking if we are not using push to talk and not talking
		if (!bIsSpeaking)
		{
			ToggleSpeaking(true);
		}
	}
}

bool AILLPlayerController::DoesUserOwnEntitlement(const FString& EntitlementId) const
{
	if (EntitlementId.IsEmpty())
		return true;

	if (IsLocalPlayerController())
	{
		if (UILLLocalPlayer* LocalPlayer = Cast<UILLLocalPlayer>(GetLocalPlayer()))
		{
			return LocalPlayer->DoesUserOwnEntitlement(EntitlementId);
		}
	}

	return false;
}

FInputActionBinding& AILLPlayerController::AddActionBinding(FInputActionBinding& InBinding)
{
	ActionBindings.Add(InBinding);
	if (InBinding.KeyEvent == IE_Pressed || InBinding.KeyEvent == IE_Released)
	{
		const EInputEvent PairedEvent = (InBinding.KeyEvent == IE_Pressed ? IE_Released : IE_Pressed);
		for (int32 BindingIndex = ActionBindings.Num() - 2; BindingIndex >= 0; --BindingIndex)
		{
			FInputActionBinding& ActionBinding = ActionBindings[BindingIndex];
			if (ActionBinding.ActionName == InBinding.ActionName)
			{
				// If we find a matching event that is already paired we know this is paired so mark it off and we're done
				if (ActionBinding.bPaired)
				{
					ActionBindings.Last().bPaired = true;
					break;
				}
				// Otherwise if this is a pair to the new one mark them both as paired
				// Don't break as there could be two bound paired events
				else if (ActionBinding.KeyEvent == PairedEvent)
				{
					ActionBinding.bPaired = true;
					ActionBindings.Last().bPaired = true;
				}
			}
		}
	}

	return ActionBindings.Last();
}

void AILLPlayerController::RemoveActionBinding(const int32 BindingIndex)
{
	if (BindingIndex >= 0 && BindingIndex < ActionBindings.Num())
	{
		const FInputActionBinding& BindingToRemove = ActionBindings[BindingIndex];

		// Potentially need to clear some pairings
		if (BindingToRemove.bPaired)
		{
			TArray<int32> IndicesToClear;
			const EInputEvent PairedEvent = (BindingToRemove.KeyEvent == IE_Pressed ? IE_Released : IE_Pressed);

			for (int32 ActionIndex = 0; ActionIndex < ActionBindings.Num(); ++ActionIndex)
			{
				if (ActionIndex != BindingIndex)
				{
					const FInputActionBinding& ActionBinding = ActionBindings[ActionIndex];
					if (ActionBinding.ActionName == BindingToRemove.ActionName)
					{
						// If we find another of the same key event then the pairing is intact so we're done
						if (ActionBinding.KeyEvent == BindingToRemove.KeyEvent)
						{
							IndicesToClear.Empty();
							break;
						}
						// Otherwise we may need to clear the pairing so track the index
						else if (ActionBinding.KeyEvent == PairedEvent)
						{
							IndicesToClear.Add(ActionIndex);
						}
					}
				}
			}

			for (int32 ClearIndex = 0; ClearIndex < IndicesToClear.Num(); ++ClearIndex)
			{
				ActionBindings[IndicesToClear[ClearIndex]].bPaired = false;
			}
		}

		ActionBindings.RemoveAt(BindingIndex);
	}
}

void AILLPlayerController::ProcessActions(UPlayerInput* Input)
{
	// iterate through the bindings and find their associated keys/chords. if the conditions are met call the delegates.
	for (FInputActionBinding Binding : ActionBindings)
	{
		TArray<FInputActionKeyMapping> KeyMappings = Input->GetKeysForAction(Binding.ActionName);
		for (FInputActionKeyMapping Mapping : KeyMappings)
		{
			if (FKeyState* KeyState = Input->GetKeyState(Mapping.Key))
			{
				if (KeyState->bConsumed)
					continue;

				// Is our keystate what we want for this action?
				if (KeyState->EventCounts[Binding.KeyEvent].Num() > 0)
				{
					Binding.ActionDelegate.Execute(Mapping.Key);
				}
			}
		}
	}
}

void AILLPlayerController::ProcessAxes(UPlayerInput* Input, bool bGamePaused)
{
	for (FInputAxisBinding Binding : AxisBindings)
	{
		float AxisValue = 0.f;

		UPlayerInput::FAxisKeyDetails* KeyDetails = Input->GetAxisBinding(Binding.AxisName);
		if (KeyDetails)
		{
			for (int32 AxisIndex = 0; AxisIndex < KeyDetails->KeyMappings.Num(); ++AxisIndex)
			{
				const FInputAxisKeyMapping& KeyMapping = (KeyDetails->KeyMappings)[AxisIndex];
				if (!Input->IsKeyConsumed(KeyMapping.Key))
				{
					if (!bGamePaused || Binding.bExecuteWhenPaused)
					{
						AxisValue += Input->GetKeyValue(KeyMapping.Key) * KeyMapping.Scale;
					}
				}
			}

			if (KeyDetails->bInverted)
			{
				AxisValue *= -1.f;
			}
		}
		if (Binding.AxisDelegate.IsBound())
		{
			Binding.AxisDelegate.Execute(AxisValue);
		}
	}
}

#undef LOCTEXT_NAMESPACE
