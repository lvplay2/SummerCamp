// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPlayerController.h"

// Engine
#include "Online.h"
#include "OnlineEventsInterface.h"
#include "OnlineIdentityInterface.h"
#include "OnlineSessionInterface.h"

// IGF
#include "ILLGameOnlineBlueprintLibrary.h"

// SC
#include "SCBackendRequestManager.h"
#include "SCCharacter.h"
#include "SCCharacterPreferencesSaveGame.h"
#include "SCCharacterSelectionsSaveGame.h"
#include "SCCheatManager.h"
#include "SCCounselorCharacter.h"
#include "SCDriveableVehicle.h"
#include "SCGameInstance.h"
#include "SCGameMode_Hunt.h"
#include "SCGameSession.h"
#include "SCGameState_Hunt.h"
#include "SCHunterPlayerStart.h"
#include "SCInGameHUD.h"
#include "SCKillerCharacter.h"
#include "SCLocalPlayer.h"
#include "SCMusicComponent.h"
#include "SCMusicComponent_Killer.h"
#include "SCPlayerStart.h"
#include "SCPlayerState.h"
#include "SCPlayerState_Hunt.h"
#include "SCPlayerCameraManager.h"
#include "SCSpectatorPawn.h"
#include "SCStaticSpectatorCamera.h"
#include "SCVoiceOverComponent.h"
#include "SCWalkieTalkie.h"
#include "SCWeapon.h"
#include "SCWorldSettings.h"
#include "SCRadio.h"

#define LOCTEXT_NAMESPACE "ILLFonic.F13.SCPlayerController"

ASCPlayerController::ASCPlayerController(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	static const ConstructorHelpers::FObjectFinder<UClass> CameraManagerClass(TEXT("/Game/Blueprints/PlayerCameraManager_BP.PlayerCameraManager_BP_C"));
	if (CameraManagerClass.Object)
		PlayerCameraManagerClass = CameraManagerClass.Object;
	else
		PlayerCameraManagerClass = ASCPlayerCameraManager::StaticClass();
	CheatClass = USCCheatManager::StaticClass();

	static const ConstructorHelpers::FObjectFinder<USoundCue> StartTalkingObj(TEXT("/Game/UI/Sounds/StartTalking"));
	TalkingStartSound = StartTalkingObj.Object;
	static const ConstructorHelpers::FObjectFinder<USoundCue> StopTalkingObj(TEXT("/Game/UI/Sounds/StopTalking"));
	TalkingStopSound = StopTalkingObj.Object;
	bTakeKinectCPUReserve = true;
}

void ASCPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void ASCPlayerController::SeamlessTravelFrom(APlayerController* OldPC)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(SpectatorIntroTimer);
	}

	Super::SeamlessTravelFrom(OldPC);
}

void ASCPlayerController::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void ASCPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

#if !UE_BUILD_SHIPPING
	if (IsInState(NAME_Spectating))
	{
		if (CVarDebugSpectating.GetValueOnGameThread() > 0)
		{
			if (CurrentSpectatingMode == ESCSpectatorMode::Player)
			{
				AActor* ViewTarget = GetViewTarget();
				APawn* ViewTargetPawn = PlayerCameraManager ? PlayerCameraManager->GetViewTargetPawn() : nullptr;
				if (ViewTargetPawn)
				{
					GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::White, FString::Printf(TEXT("Spectating Pawn: %s"), *ViewTargetPawn->GetName()));
				}
				if (ViewTarget)
				{
					GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::White, FString::Printf(TEXT("Spectating Controller: %s"), *ViewTarget->GetName()));
				}
			}
		}
	}
#endif

	UWorld* World = GetWorld();

	// Update VoIP mute list
	if (Role == ROLE_Authority && GetNetMode() != NM_Standalone)
	{
		if (const ASCPlayerState* MyPS = Cast<ASCPlayerState>(PlayerState))
		{
			if (ASCGameState* GS = World ? World->GetGameState<ASCGameState>() : nullptr)
			{
				// Gather information about the local player
				const bool bLocalPlayerHasWalkieTalkie = GS->WalkieTalkiePlayers.Contains(MyPS);
				const bool bLocalPlayerIsSpectating = MyPS->GetSpectating();
				const FVector LocalPlayerLocation = MyPS->GetPlayerLocation();

				for (const APlayerState* PS : GS->PlayerArray)
				{
					// Don't need to mute ourself
					if (PS == MyPS)
						continue;

					if (const ASCPlayerState* RemotePS = Cast<ASCPlayerState>(PS))
					{
						if (RemotePS->UniqueId.IsValid())
						{
							// Gather information about the remote player
							const bool bRemotePlayerHasWalkieTalkie = GS->WalkieTalkiePlayers.Contains(RemotePS);
							const bool bRemotePlayerIsSpectating = RemotePS->GetSpectating();
							const FVector RemotePlayerLocation = RemotePS->GetPlayerLocation();

							// Update mute list while match is in progress
							if (GS->IsMatchInProgress())
							{
								// Both have walkie talkies, and are alive, make sure the remote player isn't muted
								if ((bLocalPlayerHasWalkieTalkie && !bLocalPlayerIsSpectating) && (bRemotePlayerHasWalkieTalkie && !bRemotePlayerIsSpectating))
								{
									if (IsPlayerMuted(RemotePS->UniqueId))
									{
										GameplayUnmutePlayer(RemotePS->UniqueId);
										UE_LOG_ONLINE(VeryVerbose, TEXT("Unmuting %s, both have walkie talkies."), *RemotePS->PlayerName);
									}
								}
								else if (RemotePS->VoiceAudioComponent)
								{
									// Distance from local player to remote player
									const float DistanceToRemotePlayer = (LocalPlayerLocation - RemotePlayerLocation).Size();
									const bool bInAttenuationRange = DistanceToRemotePlayer < RemotePS->VoiceAudioComponent->AttenuationOverrides.FalloffDistance;

									// Local player is spectating, remote player isn't
									if (bLocalPlayerIsSpectating && !bRemotePlayerIsSpectating)
									{
										// Don't mute players we are spectating if we are within attenuation range
										if (bInAttenuationRange)
										{
											if (IsPlayerMuted(RemotePS->UniqueId))
											{
												GameplayUnmutePlayer(RemotePS->UniqueId);
												UE_LOG_ONLINE(VeryVerbose, TEXT("Spectating: Unmuting %s, they are within attenuation range."), *RemotePS->PlayerName);
											}
										}
										// Remote player is far away, mute them
										else if (!IsPlayerMuted(RemotePS->UniqueId))
										{
											GameplayMutePlayer(RemotePS->UniqueId);
											UE_LOG_ONLINE(VeryVerbose, TEXT("Spectating: Muting %s, they are out of attenuation range."), *RemotePS->PlayerName);
										}
									}
									// Both local and remote players are spectating, make sure they can hear each other
									else if (bLocalPlayerIsSpectating && bRemotePlayerIsSpectating)
									{
										if (IsPlayerMuted(RemotePS->UniqueId))
										{
											GameplayUnmutePlayer(RemotePS->UniqueId);
											UE_LOG_ONLINE(VeryVerbose, TEXT("Spectating: Unmuting %s, both are spectating."), *RemotePS->PlayerName);
										}
									}
									// Remote player is spectating, local player isn't, mute them
									else if (bRemotePlayerIsSpectating)
									{
										if (!IsPlayerMuted(RemotePS->UniqueId))
										{
											GameplayMutePlayer(RemotePS->UniqueId);
											UE_LOG_ONLINE(VeryVerbose, TEXT("Spectating: Muting %s, they are spectating."), *RemotePS->PlayerName);
										}
									}
									// Both local and remote players are alive
									else
									{
										if (bInAttenuationRange)
										{
											// Remote player is in attenuation range, make sure they aren't muted
											if (IsPlayerMuted(RemotePS->UniqueId))
											{
												GameplayUnmutePlayer(RemotePS->UniqueId);
												UE_LOG_ONLINE(VeryVerbose, TEXT("Unmuting %s, they are in attenuation range."), *RemotePS->PlayerName);
											}
										}
										// Remote player is far away, mute them
										else if (!IsPlayerMuted(RemotePS->UniqueId))
										{
											GameplayMutePlayer(RemotePS->UniqueId);
											UE_LOG_ONLINE(VeryVerbose, TEXT("Muting %s, they are out of attenuation range."), *RemotePS->PlayerName);
										}
									}
								}
							}
							// Make sure players are unmuted while the match isn't in progress
							else if (IsPlayerMuted(RemotePS->UniqueId))
							{
								GameplayUnmutePlayer(RemotePS->UniqueId);
								UE_LOG_ONLINE(VeryVerbose, TEXT("Unmuting %s, the match isn't in progress."), *RemotePS->PlayerName);
							}
						}
					}
				}
			}
		}
	}
}

void ASCPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(SpectatorIntroTimer);
		GetWorldTimerManager().ClearTimer(TimerHandle_SpawnIntroCompleted);
	}

	Super::EndPlay(EndPlayReason);
}

void ASCPlayerController::ModifyInputSensitivity(FKey InputKey, float SensitivityMod)
{
	if (PlayerInput && IsLocalController())
	{
		if (SensitivityCache.Contains(InputKey))
		{
#if WITH_EDITOR
			FMessageLog("PIE").Error()
				->AddToken(FTextToken::Create(InputKey.GetDisplayName()))
				->AddToken(FTextToken::Create(FText::FromString(TEXT(" already has a cached value."))));
#endif
			return;
		}

		FInputAxisProperties ControllerAxisProps;
		if (PlayerInput->GetAxisProperties(InputKey, ControllerAxisProps))
		{
			SensitivityCache.Add(InputKey, ControllerAxisProps.Sensitivity);
			ControllerAxisProps.Sensitivity *= SensitivityMod;
			PlayerInput->SetAxisProperties(InputKey, ControllerAxisProps);
		}
	}
}

void ASCPlayerController::RestoreInputSensitivity(FKey InputKey)
{
	if (PlayerInput && IsLocalController())
	{
		if (!SensitivityCache.Contains(InputKey))
		{
#if WITH_EDITOR
			FMessageLog("PIE").Error()
				->AddToken(FTextToken::Create(InputKey.GetDisplayName()))
				->AddToken(FTextToken::Create(FText::FromString(TEXT(" has no cached value."))));
#endif
			return;
		}

		FInputAxisProperties ControllerAxisProps;
		if (PlayerInput->GetAxisProperties(InputKey, ControllerAxisProps))
		{
			ControllerAxisProps.Sensitivity = SensitivityCache[InputKey];
			PlayerInput->SetAxisProperties(InputKey, ControllerAxisProps);
			SensitivityCache.Remove(InputKey);
		}
	}
}

void ASCPlayerController::RestoreAllInputSensitivity()
{
	if (PlayerInput && IsLocalController())
	{
		FInputAxisProperties ControllerAxisProps;
		for (TPair<FKey, float> Modified : SensitivityCache)
		{
			if (PlayerInput->GetAxisProperties(Modified.Key, ControllerAxisProps))
			{
				ControllerAxisProps.Sensitivity = Modified.Value;
				PlayerInput->SetAxisProperties(Modified.Key, ControllerAxisProps);
			}
		}

		SensitivityCache.Empty();
	}
}

void ASCPlayerController::FailedToSpawnPawn()
{
	if (StateName == NAME_Inactive)
	{
		BeginInactiveState();
	}

	Super::FailedToSpawnPawn();
}

void ASCPlayerController::PawnPendingDestroy(APawn* P)
{
	// Pick a camera location
	FVector CameraLocation = P->GetActorLocation() + FVector(0.f, 0.f, 300.f);
	FRotator CameraRotation(-90.f, 0.f, 0.f);
	FindDeathCameraSpot(P, CameraLocation, CameraRotation);

	Super::PawnPendingDestroy(P);

	// Enter spectate
	ClientSetSpectatorCamera(CameraLocation, CameraRotation);
}

void ASCPlayerController::ChangeState(FName NewState)
{
	const FName CurrentState = StateName;

	Super::ChangeState(NewState);

	if (NewState != CurrentState)
	{
		if (NewState == NAME_Spectating)
		{
			bool bSkipIntro = false;
			if (ASCPlayerState* PS = Cast<ASCPlayerState>(PlayerState))
			{
				// Don't attempt to play the spectator hud intro if we're a mid-game join
				if (!PS->DidSpawnIntoMatch())
					bSkipIntro = true;
			}

			CurrentSpectatingMode = ESCSpectatorMode::SpectateIntro;
			UpdateSpectatorHUD();

			if (bSkipIntro)
			{
				OnToggleSpectateMode();
				UpdateSpectatorHUD();
			}
			else
			{
				// Switch to spectator intro
				GetWorldTimerManager().SetTimer(SpectatorIntroTimer, this, &ThisClass::OnToggleSpectateMode, 7.f);
			}
		}
		else // Could be respawning as the hunter, need to reset SpectatingPlayer
		{
			SpectatingPlayer = nullptr;
		}
	}
}

void ASCPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindAction(TEXT("GLB_ShowScoreboard"), IE_Pressed, this, &ThisClass::ShowScoreboardAction);
	InputComponent->BindAction(TEXT("GLB_ShowScoreboard"), IE_Released, this, &ThisClass::HideScoreboardAction);

	BindPersistentAction(TEXT("GLB_Pause"), EInputEvent::IE_Released, this, &ASCPlayerController::OnPause);

	// TODO: Add UI Inputs (to span player character and spectator character)
	if (!SpectateInputComponent)
	{
		SpectateInputComponent = NewObject<UInputComponent>(this, TEXT("SpectatorInput"));
	}
	SpectateInputComponent->bBlockInput = true;
	SpectateInputComponent->BindAction(TEXT("SPEC_ToggleMode"), IE_Pressed, this, &ThisClass::OnToggleSpectateMode);
	SpectateInputComponent->BindAction(TEXT("SPEC_ViewNext"), IE_Pressed, this, &ThisClass::OnViewNextSpectatorCamera);
	SpectateInputComponent->BindAction(TEXT("SPEC_ViewPrev"), IE_Pressed, this, &ThisClass::OnViewPreviousSpectatorCamera);
	SpectateInputComponent->BindAction(TEXT("GLB_ShowScoreboard"), IE_Pressed, this, &ThisClass::ShowScoreboardAction);
	SpectateInputComponent->BindAction(TEXT("GLB_ShowScoreboard"), IE_Released, this, &ThisClass::HideScoreboardAction);
}

bool ASCPlayerController::IsMoveInputIgnored() const
{
	if (IsInState(NAME_Spectating))
	{
		return false;
	}
	else
	{
		return Super::IsMoveInputIgnored();
	}
}

bool ASCPlayerController::IsLookInputIgnored() const
{
	if (IsInState(NAME_Spectating))
	{
		return false;
	}
	else
	{
		return Super::IsLookInputIgnored();
	}
}

bool ASCPlayerController::SetPause(bool bPause, FCanUnpause CanUnpauseDelegate /*= FCanUnpause()*/)
{
	const bool Result = APlayerController::SetPause(bPause, CanUnpauseDelegate);

	// Update rich presence
	const IOnlinePresencePtr PresenceInterface = Online::GetPresenceInterface();
	const ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	TSharedPtr<const FUniqueNetId> UserId = LocalPlayer ? LocalPlayer->GetCachedUniqueNetId() : nullptr;

	if (PresenceInterface.IsValid() && UserId.IsValid())
	{
		FOnlineUserPresenceStatus PresenceStatus;
		if (Result && bPause)
		{
			PresenceStatus.Properties.Add(DefaultPresenceKey, TEXT("Paused"));
		}
		else
		{
			PresenceStatus.Properties.Add(DefaultPresenceKey, TEXT("InGame"));
		}

		PresenceInterface->SetPresence(*UserId, PresenceStatus);
	}

	return Result;
}

bool ASCPlayerController::CanRestartPlayer()
{
	if (!Super::CanRestartPlayer())
		return false;

	if (ASCPlayerState* PS = Cast<ASCPlayerState>(PlayerState))
	{
		if (PS->GetIsDead() || PS->GetSpectating())
			return false;

		if (!PS->CanSpawnActiveCharacter())
			return false;
	}

	return true;
}

void ASCPlayerController::ServerAcknowledgePossession_Implementation(APawn* P)
{
	Super::ServerAcknowledgePossession_Implementation(P);

	// Bump idle timer
	BumpIdleTime();

	// Check if we should kick off a deferred spawn intro
	TryStartSpawnIntroSequence();
}

void ASCPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);

	if (IsLocalController())
	{
		if (ASCInGameHUD* HUD = GetSCHUD())
		{
			HUD->UpdateCharacterHUD();
		}
	}
}

void ASCPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	SendPreferencesToServer();
}

void ASCPlayerController::Destroyed()
{
	if (Role == ROLE_Authority)
	{
		if (UWorld* World = GetWorld())
		{
			// Pre-match intro abandon infraction
			ASCGameMode* GameMode = World->GetAuthGameMode<ASCGameMode>();
			if (GameMode && GameMode->GetMatchState() == MatchState::PreMatchIntro)
			{
				if (ASCGameSession* GameSession = Cast<ASCGameSession>(GameMode->GameSession))
				{
					if (GameSession->IsQuickPlayMatch())
					{
						USCGameInstance* GameInstance = Cast<USCGameInstance>(GetGameInstance());
						USCBackendRequestManager* RequestManager = GameInstance->GetBackendRequestManager<USCBackendRequestManager>();
						RequestManager->ReportInfraction(GetBackendAccountId(), InfractionTypes::LeftDuringIntroSequence);
					}
				}
			}

			if (ASCGameState* GS = World->GetGameState<ASCGameState>())
			{
				// Only handle pushing experience if match is in progress
				if (GS->IsMatchInProgress() && GS->ShouldAwardExperience())
				{
					if (ASCPlayerState* PS = Cast<ASCPlayerState>(PlayerState))
					{
						// Only people that played the match at least a little get exp after leaving early
						if (PS->GetIsDead() || PS->GetEscaped() || PS->GetIsHunter())
						{
							USCGameInstance* GI = Cast<USCGameInstance>(GetGameInstance());
							USCBackendRequestManager* BRM = GI->GetBackendRequestManager<USCBackendRequestManager>();
							if (BRM->CanSendAnyRequests())
							{
								const int32 PlayerScore = FMath::Max(PS->Score, 1.f) * GS->XPModifier;
								FOnILLBackendSimpleRequestDelegate Delegate = FOnILLBackendSimpleRequestDelegate::CreateUObject(this, &ThisClass::OnExpPushComplete);
								BRM->AddExperienceToProfile(GetBackendAccountId(), PlayerScore, Delegate);
							}
						}
					}
				}
			}
		}
	}

	Super::Destroyed();
}

void ASCPlayerController::AcknowledgePlayerState(APlayerState* PS)
{
	Super::AcknowledgePlayerState(PS);

	SendPreferencesToServer();
}

void ASCPlayerController::ServerAcknowledgePlayerState_Implementation(APlayerState* PS)
{
	Super::ServerAcknowledgePlayerState_Implementation(PS);

	// Bump idle timer
	BumpIdleTime();

	// Check if we should kick off a deferred spawn intro
	TryStartSpawnIntroSequence();
}

void ASCPlayerController::SetCinematicMode(bool bInCinematicMode, bool bHidePlayer, bool bAffectsHUD, bool bAffectsMovement, bool bAffectsTurning)
{
	Super::SetCinematicMode(bInCinematicMode, bHidePlayer, bAffectsHUD, bAffectsMovement, bAffectsTurning);

	// If we have a pawn we need to determine if we should show/hide the weapon
	ASCCharacter* MyCharacter = Cast<ASCCharacter>(GetPawn());
	ASCWeapon* MyWeapon = MyCharacter ? MyCharacter->GetCurrentWeapon() : nullptr;
	if (MyWeapon)
	{
		if (bInCinematicMode && bHidePlayer)
		{
			MyWeapon->SetActorHiddenInGame(true);
		}
		else if (!bCinematicMode)
		{
			MyWeapon->SetActorHiddenInGame(false);
		}
	}
}

void ASCPlayerController::UnFreeze()
{
	if (Role == ROLE_Authority)
	{
		// This is triggered by FailedToSpawnPawn so check if they can spawn now
		TimerHandle_UnFreeze.Invalidate();
		if (ASCGameMode_Online* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode_Online>())
		{
			if (GameMode->IsMatchInProgress())
			{
				// Respawn them when the match is in-progress
				GameMode->RestartPlayer(this);
			}
			else if (!GameMode->HasMatchEnded())
			{
				// Only retry when the match has not ended
				GetWorldTimerManager().SetTimer(TimerHandle_UnFreeze, this, &ThisClass::UnFreeze, GetMinRespawnDelay());
			}
		}
	}
}

void ASCPlayerController::PawnLeavingGame()
{
	UWorld* World = GetWorld();
	ASCGameMode* GameMode = World ? World->GetAuthGameMode<ASCGameMode>() : nullptr;

	if (APawn* MyPawn = GetPawn())
	{
		ASCCharacter* SCCharacter = nullptr;
		if (ASCDriveableVehicle* Vehicle = Cast<ASCDriveableVehicle>(MyPawn))
		{
			SCCharacter = Cast<ASCCharacter>(Vehicle->Driver);
		}
		else
		{
			SCCharacter = Cast<ASCCharacter>(MyPawn);
		}

		if (SCCharacter)
		{
			if (ASCGameSession* GameSession = GameMode ? Cast<ASCGameSession>(GameMode->GameSession) : nullptr)
			{
				if (GameSession->IsQuickPlayMatch() && !GameSession->bHostReturningToMainMenu && GameMode->GetMatchState() == MatchState::InProgress)
				{
					// We have an alive character to deal with
					FString InfractionID;
					if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(SCCharacter))
					{
						// Determine infraction type
						if (Counselor->bIsGrabbed || Counselor->IsInContextKill())
						{
							InfractionID = InfractionTypes::CounselorLeftWhileBeingKilled;
						}
						else
						{
							InfractionID = InfractionTypes::CounselorLeftWhileAlive;
						}

						// Be sure to eject from a vehicle if we're in one
						if (Counselor->IsInVehicle())
						{
							Counselor->GetVehicle()->EjectCounselor(Counselor, false);
						}

						// Also cleanup any grab
						if (Counselor->bIsGrabbed)
						{
							Counselor->GrabbedBy->DropCounselor();
						}
					}
					else if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(SCCharacter))
					{
						// Determine infraction type
						if (Killer->IsInContextKill() && !Killer->GetGrabbedCounselor())
						{
							InfractionID = InfractionTypes::JasonLeftWhileBeingKilled;
						}
						else
						{
							InfractionID = InfractionTypes::JasonLeftWhileAlive;
						}
					}

					// Now snitch!
					if (!InfractionID.IsEmpty())
					{
						USCGameInstance* GameInstance = Cast<USCGameInstance>(GetGameInstance());
						USCBackendRequestManager* RequestManager = GameInstance->GetBackendRequestManager<USCBackendRequestManager>();
						RequestManager->ReportInfraction(GetBackendAccountId(), InfractionID);
					}
				}
			}

			// Now die!
			SCCharacter->Suicide();
		}
	}

	if (GameMode)
	{
		if (GameMode->HunterToSpawn == this)
		{
			GameMode->HunterToSpawn = nullptr;
			GetWorldTimerManager().ClearTimer(GameMode->TimerHandle_HunterTimer);
			GameMode->AttemptSpawnHunter();
		}
	}
}

void ASCPlayerController::OnExpPushComplete(int32 ResponseCode, const FString& ResponseContents)
{
	if (ResponseCode != 200)
	{
		UE_LOG(LogSC, Warning, TEXT("OnExpPushComplete: ResponseCode %i Reason: %s"), ResponseCode, *ResponseContents);
	}
}

void ASCPlayerController::SetBackendAccountId(const FILLBackendPlayerIdentifier& InBackendAccountId)
{
	Super::SetBackendAccountId(InBackendAccountId);

	if (ASCPlayerState* PS = Cast<ASCPlayerState>(PlayerState))
	{
		// Update backend profile
		PS->RequestBackendProfileUpdate();

		// Get Player stats
		PS->RequestPlayerStats();

		// Get Player badges
		PS->RequestPlayerBadges();

		// Get this player's platform achievements ready
		PS->QueryAchievements();
	}
}

bool ASCPlayerController::IsGameInputAllowed() const
{
	if (!Super::IsGameInputAllowed())
	{
		return false;
	}

	// Wait for the spawn intro to complete before allowing input
	if (bPlayingSpawnIntro && bSpawnIntroBlocksInput)
	{
		return false;
	}

	// Disallow input when the match is not in-progress
	UWorld* World = GetWorld();
	ASCGameState* GS = Cast<ASCGameState>(World->GetGameState());
	if (!GS || (!GS->IsMatchInProgress() && !GS->IsMatchWaitingPostMatchOutro()))
	{
		return false;
	}

	return true;
}

bool ASCPlayerController::IsUsingMultiplayerFeatures() const
{
	// according to Sony you arent using multiplayer features when spectating
	if (IsInState(NAME_Spectating))
		return false;

	if (GetPawn() && GetPawn()->IsA(ASCSpectatorPawn::StaticClass()))
		return false;

	return Super::IsUsingMultiplayerFeatures();
}

void ASCPlayerController::NotifyLevelIntroSequenceStarted()
{
	if (ASCInGameHUD* HUD = GetSCHUD())
	{
		HUD->OnLevelIntroSequenceStarted();
	}
}

void ASCPlayerController::NotifyLevelIntroSequenceCompleted()
{
	if (ASCInGameHUD* HUD = GetSCHUD())
	{
		HUD->OnLevelIntroSequenceCompleted();
	}
}

void ASCPlayerController::NotifyLevelOutroSequenceStarted()
{
	if (Role == ROLE_Authority)
	{
		if (ASCPlayerState* PS = Cast<ASCPlayerState>(PlayerState))
		{
			PS->RequestBackendProfileUpdate();
		}
	}

	if (ASCInGameHUD* HUD = GetSCHUD())
	{
		HUD->OnLevelOutroSequenceStarted();
	}
}

void ASCPlayerController::NotifyLevelOutroSequenceCompleted()
{
	if (ASCInGameHUD* HUD = GetSCHUD())
	{
		HUD->OnLevelOutroSequenceCompleted();
	}
}

bool ASCPlayerController::TryStartSpawnIntroSequence(ASCPlayerStart* OverrideStartSpot /*= nullptr*/)
{
	check(Role == ROLE_Authority);

	// When StartSpot is NULL we have already played a spawn intro, or spawned by UNNATURAL MEANS
	if (!StartSpot.IsValid() && !OverrideStartSpot)
		return false;

	UWorld* World = GetWorld();
	ASCGameMode* GameMode = World->GetAuthGameMode<ASCGameMode>();

	// Do nothing until the match is in-progress
	// We check the GameMode instead of the GameState because this may be called from ASCGameMode::HandleMatchHasStarted, and that is hit BEFORE the GameState gets updated!
	if (World->bBegunPlay && GameMode->IsMatchInProgress())
	{
		// Wait for pawn acknowledgment before starting the spawn intro
		if ((OverrideStartSpot || AcknowledgedPawn == GetPawn()) && AcknowledgedPlayerState == PlayerState)
		{
			if (ASCPlayerState* SCPS = Cast<ASCPlayerState>(PlayerState))
			{
				if (SCPS->bPlayedSpawnIntro)
					return false;

				SCPS->bPlayedSpawnIntro = true;
			}
			// Start the spawn intro and consume StartSpot so repossession does not trigger it again
			PlaySpawnIntroSequence(OverrideStartSpot == nullptr ? Cast<ASCPlayerStart>(StartSpot.Get()) : OverrideStartSpot);
			StartSpot = nullptr;
			return true;
		}
	}

	return false;
}

void ASCPlayerController::PlaySpawnIntroSequence(ASCPlayerStart* StartSpawnedAt)
{
	if (IsLocalController())
	{
		// Ensure the level intro sequence has COMPLETELY finished locally
		Cast<ASCWorldSettings>(GetWorldSettings())->StopLevelIntro(true);
	}
	else if (Role == ROLE_Authority)
	{
		// Tell the client to do it too
		ClientPlaySpawnIntroSequence(StartSpawnedAt);

		// Flag us as spawned
		if (ASCPlayerState* PS = Cast<ASCPlayerState>(PlayerState))
		{
			if (GetPawn() && GetPawn()->IsA<ASCCharacter>())
			{
				if (ASCPlayerState_Hunt* HuntPlayerState = Cast<ASCPlayerState_Hunt>(PlayerState))
					HuntPlayerState->SpawnedCharacterClass = GetPawn()->GetClass();
			}
			else if (StartSpawnedAt && !StartSpawnedAt->IsA(ASCHunterPlayerStart::StaticClass()) && ensure(!PS->CanSpawnActiveCharacter()))
			{
				// Ensure this player is properly put into spectator
				ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>();
				GameMode->Spectate(this);
			}
		}
	}

	// Attempt to play a spawn intro and return
	if (StartSpawnedAt)
	{
		if (IsLocalPlayerController())
		{
			if (ULevelSequencePlayer* SequencePlayer = StartSpawnedAt->PlaySpawnSequence(this, bSpawnIntroBlocksInput))
			{
				bPlayingSpawnIntro = true;
				GetWorldTimerManager().SetTimer(TimerHandle_SpawnIntroCompleted, this, &ThisClass::OnSpawnIntroCompleted, FMath::Max(0.1f, StartSpawnedAt->GetSpawnSequenceLength(this)));

				// Update the HUD
				if (ASCInGameHUD* HUD = GetSCHUD())
				{
					HUD->OnSpawnIntroSequenceStarted();
				}
				return;
			}

				// Fail! Skip the spawn intro
				OnSpawnIntroCompleted();
			}
		else
		{
			// On the host just use a timer to simulate the completion behavior
			GetWorldTimerManager().SetTimer(TimerHandle_SpawnIntroCompleted, this, &ThisClass::OnSpawnIntroCompleted, FMath::Max(0.1f, StartSpawnedAt->GetSpawnSequenceLength(this)));
		}
	}
	else
	{
		// Skip to the end of the spawn intro
		OnSpawnIntroCompleted();
	}
}

void ASCPlayerController::ClientPlaySpawnIntroSequence_Implementation(ASCPlayerStart* StartSpawnedAt)
{
	PlaySpawnIntroSequence(StartSpawnedAt);
}

void ASCPlayerController::OnSpawnIntroCompleted()
{
	// Allow input again
	bPlayingSpawnIntro = false;

	// Ensure we are viewing our pawn
	// Spawn intro may take control of the camera, so we have to handle that
	SetViewTarget(GetPawn());

	// Now to play the HUD start sequence
	if (ASCCharacter* CurrentCharacter = Cast<ASCCharacter>(GetPawn()))
	{
		// Play match start music
		static FName NAME_MatchStart(TEXT("MatchStart"));
		if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(CurrentCharacter))
		{
			Counselor->MusicComponent->PlayMusic(NAME_MatchStart);
		}
	}

	if (IsLocalController())
	{
		// Play HUD match start animation
		if (ASCInGameHUD* HUD = GetSCHUD())
		{
			HUD->OnSpawnIntroSequenceCompleted();
		}
	}

	// Play match start VO for the counselor
	if (ASCCounselorCharacter* CounselorCharacter = Cast<ASCCounselorCharacter>(GetPawn()))
	{
		const FName NAME_MatchStartVO = TEXT("MatchStartVO");
		CounselorCharacter->VoiceOverComponent->PlayVoiceOver(NAME_MatchStartVO);
	}
}

void ASCPlayerController::BumpIdleTime()
{
	if (Role != ROLE_Authority)
		return;

	if (UWorld* World = GetWorld())
	{
		LastActivityTimeSeconds = World->GetTimeSeconds();
		LastCheckIdleTime = 0.f;
	}
}

bool ASCPlayerController::IdleCheck()
{
	if (Role != ROLE_Authority)
		return false;

	// Do not kick the server or someone loading
	if (IsLocalController() || !bHasReceivedPlayer)
		return false;

	if (UWorld* World = GetWorld())
	{
		// Do not idle kick dead/spectating players
		ASCPlayerState* PS = Cast<ASCPlayerState>(PlayerState);
		if (!PS || PS->GetIsDead() || PS->GetSpectating())
		{
			BumpIdleTime();
			return false;
		}

#if WITH_EDITOR
		if (World->IsPlayInEditor())
			return false;
#endif

		if (ASCGameMode* GameMode = World->GetAuthGameMode<ASCGameMode>())
		{
			// Only idle check when the match is in-progress
			if (!GameMode->IsMatchInProgress())
				return false;

			ASCGameSession* GameSession = Cast<ASCGameSession>(GameMode->GameSession);
			if (ensure(GameSession))
			{
				if (GameSession->IsQuickPlayMatch() && GameMode->MaxIdleTimeBeforeKick > 0.f)
				{
					const float PlayerIdleTime = World->GetTimeSeconds() - LastActivityTimeSeconds;
					if (PlayerIdleTime >= GameMode->MaxIdleTimeBeforeKick)
					{
						// Idle time limit exceeded, kick them
						GameSession->KickPlayer(this, FText::Format(LOCTEXT("KickIdleTimeLimit", "Idle time limit {0} seconds exceeded!"), FText::FromString(FString::FromInt(FMath::FloorToInt(GameMode->MaxIdleTimeBeforeKick)))));
						return true;
					}

					// Check if we have crossed the WarningTime
					const float WarningHalfTime = GameMode->MaxIdleTimeBeforeKick*.5f;
					const float WarningLastQuarterTime = GameMode->MaxIdleTimeBeforeKick*.75f;
					if((LastCheckIdleTime < WarningHalfTime && PlayerIdleTime >= WarningHalfTime)
					|| (LastCheckIdleTime < WarningLastQuarterTime && PlayerIdleTime >= WarningLastQuarterTime))
					{
						ClientIdleKickWarning(GameMode->MaxIdleTimeBeforeKick - FMath::TruncToFloat(PlayerIdleTime));
					}
					LastCheckIdleTime = PlayerIdleTime;
				}
			}
		}
	}

	return false;
}

void ASCPlayerController::ClientIdleKickWarning_Implementation(const float TimeUntilKick)
{
	UILLPlayerNotificationMessage* Message = NewObject<UILLPlayerNotificationMessage>(); // CLEANUP: pjackson: Custom sub-class? Only really necessary if we need a special sound
	Message->NotificationText = FText::Format(LOCTEXT("KickIdleWarning", "You will be kicked for being idle in {0} seconds!"), FText::FromString(FString::FromInt(TimeUntilKick)));
	HandleLocalizedNotification(Message);
}

void ASCPlayerController::DebugDrawInputStack(const float DisplayTime, const FColor TextColor)
{
	TArray<UInputComponent*> InputStack;
	BuildInputStack(InputStack);

	if (InputStack.Num() > 0)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Orange, FString::Printf(TEXT("%d input components"), InputStack.Num()), false);
	}

	for (int32 iInput(InputStack.Num() - 1); iInput >= 0; --iInput)
	{
		AActor* InputComponentOwner = InputStack[iInput]->GetOwner();
		if (InputComponentOwner)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Orange, FString::Printf(TEXT(" - %s.%s"), *InputComponentOwner->GetName(), *InputStack[iInput]->GetName()), false);
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Orange, FString::Printf(TEXT(" - %s)"), *InputStack[iInput]->GetName()), false);
		}
	}
}

void ASCPlayerController::ScoreChanged(const int32 PreviousScore, const int32 NewScore)
{
	const int32 Delta = NewScore - PreviousScore;
	if (Delta == 0)
		return;
}

void ASCPlayerController::ClientDisplayScoreEvent_Implementation(TSubclassOf<USCScoringEvent> ScoringEvent, const uint8 Category, const float ScoreModifier)
{
	if (USCScoringEvent* ScoreEvent = ScoringEvent->GetDefaultObject<USCScoringEvent>())
	{
		if (ASCPlayerState* SCPlayerState = Cast<ASCPlayerState>(PlayerState))
		{
			if (ASCInGameHUD* Hud = GetSCHUD())
			{
				Hud->OnShowScoreEvent(ScoreEvent->GetDisplayMessage(ScoreModifier, SCPlayerState->IsActiveCharacterKiller() ? TArray<USCPerkData*>() : SCPlayerState->GetActivePerks()), Category);
			}
		}
	}
}

void ASCPlayerController::PushSpectateInputComponent()
{
	PushInputComponent(SpectateInputComponent);
}

void ASCPlayerController::PopSpectateInputComponent()
{
	PopInputComponent(SpectateInputComponent);
}

void ASCPlayerController::SetSpectateBlocking(bool bShouldBlockInput)
{
	SpectateInputComponent->bBlockInput = bShouldBlockInput;
}

bool ASCPlayerController::ContainsInputComponent(const UInputComponent* InComponent) const
{
	return CurrentInputStack.Contains(InComponent);
}

void ASCPlayerController::SetPlayer(UPlayer* InPlayer)
{
	Super::SetPlayer(InPlayer);
}

void ASCPlayerController::OnShowScoreboard(const bool bLock/* = false*/)
{
	if (ASCInGameHUD* Hud = GetSCHUD())
	{
		Hud->DisplayScoreboard(true, bLock);
	}
}

void ASCPlayerController::ShowScoreboardAction()
{
	if (ASCInGameHUD* Hud = GetSCHUD())
	{
		Hud->DisplayScoreboard(true);
	}
}

void ASCPlayerController::OnHideScoreboard(const bool bUnlock/* = false*/)
{
	if (ASCInGameHUD* Hud = GetSCHUD())
	{
		Hud->DisplayScoreboard(false, bUnlock);
	}
}

void ASCPlayerController::HideScoreboardAction()
{
	if (ASCInGameHUD* Hud = GetSCHUD())
	{
		Hud->DisplayScoreboard(false);
	}
}

void ASCPlayerController::OnShowEmoteSelect()
{
	if (ASCInGameHUD* Hud = GetSCHUD())
	{
		Hud->DisplayEmoteSelect(true);
	}
}

void ASCPlayerController::OnHideEmoteSelect()
{
	if (ASCInGameHUD* Hud = GetSCHUD())
	{
		Hud->DisplayEmoteSelect(false);
	}
}

bool ASCPlayerController::IsEmoteSelectDisplayed() const
{
	if (ASCInGameHUD* HUD = GetSCHUD())
		return HUD->IsEmoteSelectDisplayed();

	return false;
}

void ASCPlayerController::ShowEndMatchMenu(const bool bIsWinner)
{
}

void ASCPlayerController::CloseEndMatchMenu()
{
}

void ASCPlayerController::ClientSetSpectatorCamera_Implementation(FVector CameraLocation, FRotator CameraRotation)
{
	SetInitialLocationAndRotation(CameraLocation, CameraRotation);
	SetViewTarget(this);
}

bool ASCPlayerController::FindDeathCameraSpot(const APawn* CurrentPawn, FVector& CameraLocation, FRotator& CameraRotation)
{
	if (CurrentPawn)
	{
		const FVector PawnLocation = CurrentPawn->GetActorLocation();
		FRotator ViewDir = GetControlRotation();
		ViewDir.Pitch = -45.0f;

		const float YawOffsets[] = { 0.0f, -180.0f, 90.0f, -90.0f, 45.0f, -45.0f, 135.0f, -135.0f };
		const float CameraOffset = 600.0f;
		FCollisionQueryParams TraceParams(TEXT("DeathCamera"), true, CurrentPawn);

		for (int32 i = 0; i < ARRAY_COUNT(YawOffsets); i++)
		{
			FRotator CameraDir = ViewDir;
			CameraDir.Yaw += YawOffsets[i];
			CameraDir.Normalize();

			FVector TestLocation = PawnLocation - CameraDir.Vector() * CameraOffset;
			const bool bBlocked = GetWorld()->LineTraceTestByChannel(PawnLocation, TestLocation, ECC_WorldStatic, TraceParams);
			if (!bBlocked)
			{
				CameraLocation = TestLocation;
				CameraRotation = CameraDir;
				return true;
			}
		}
	}

	return false;
}

void ASCPlayerController::SimulateInputKey(FKey Key, bool bPressed)
{
	InputKey(Key, bPressed ? IE_Pressed : IE_Released, 1, false);
}

void ASCPlayerController::SetGodMode(bool bEnable)
{
	bGodMode = bEnable;
}

void ASCPlayerController::OnViewNextSpectatorCamera()
{
	ASCPlayerState* SCPlayerState = Cast<ASCPlayerState>(PlayerState);
	if (SCPlayerState && SCPlayerState->GetSpectating())
	{
		if (CurrentSpectatingMode == ESCSpectatorMode::Player)
		{
			OnViewPlayer(1);
		}
		else if (CurrentSpectatingMode == ESCSpectatorMode::LevelStatic)
		{
			OnViewStaticSpectatorCamera(++CurrentSpectatingCameraIndex);
		}

		UpdateSpectatorHUD();
	}
}

void ASCPlayerController::OnViewPreviousSpectatorCamera()
{
	ASCPlayerState* SCPlayerState = Cast<ASCPlayerState>(PlayerState);
	if (SCPlayerState && SCPlayerState->GetSpectating())
	{
		if (CurrentSpectatingMode == ESCSpectatorMode::Player)
		{
			OnViewPlayer(-1);
		}
		else if (CurrentSpectatingMode == ESCSpectatorMode::LevelStatic)
		{
			OnViewStaticSpectatorCamera(--CurrentSpectatingCameraIndex);
		}

		UpdateSpectatorHUD();
	}
}

void ASCPlayerController::OnViewStaticSpectatorCamera(int32& Index)
{
	if (UWorld* World = GetWorld())
	{
		if (const ASCGameState* const GS = World->GetGameState<ASCGameState>())
		{
			// Don't update if the match is over
			if (GS->HasMatchEnded())
			{
				return;
			}

			const int32 NumCameras = GS->StaticSpectatorCameras.Num();
			if (NumCameras > 0)
			{
				if (Index < 0)
				{
					Index = NumCameras - 1;
				}
				else if (Index >= NumCameras)
				{
					Index = 0;
				}

				ClientSetSpectatorCamera(GS->StaticSpectatorCameras[Index]->GetActorLocation(), GS->StaticSpectatorCameras[Index]->GetActorRotation());
				SetViewTarget(GS->StaticSpectatorCameras[Index]);
			}
		}
	}
}

void ASCPlayerController::OnViewPlayer(int32 SearchDirection)
{
	if (UWorld* World = GetWorld())
	{
		if (const ASCGameState* const GS = World->GetGameState<ASCGameState>())
		{
			// Don't update if the match is over
			if (GS->HasMatchEnded())
			{
				return;
			}

			int32 NewPlayerSpectatingIndex = CurrentPlayerSpectatingIndex;
			const int32 NumPlayers = GS->PlayerCharacterList.Num();
			if (NumPlayers > 0)
			{
				if (NewPlayerSpectatingIndex < 0)
				{
					NewPlayerSpectatingIndex = NumPlayers - 1;
				}
				else if (NewPlayerSpectatingIndex > NumPlayers)
				{
					NewPlayerSpectatingIndex = 0;
				}

				// Find a valid player to view
				ASCCharacter* ViewTarget = [&]() -> ASCCharacter*
				{
					for (NewPlayerSpectatingIndex += SearchDirection; (NewPlayerSpectatingIndex >= 0) && (NewPlayerSpectatingIndex < NumPlayers); NewPlayerSpectatingIndex += SearchDirection)
					{
						if (ASCCharacter* Character = GS->PlayerCharacterList[NewPlayerSpectatingIndex])
						{
							ASCPlayerState* NextPS = Character->GetPlayerState();
							if (!NextPS)
							{
								if (ASCDriveableVehicle* Vehicle = Character->GetVehicle())
								{
									if (Vehicle->Driver == Character)
									{
										NextPS = Cast<ASCPlayerState>(Vehicle->PlayerState);
									}
								}
							}

							if (NextPS && !NextPS->GetSpectating())
							{
								ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character);
								if (Counselor && !NextPS->GetEscaped())
								{
									return Counselor;
								}
								else if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Character))
								{
									if (UILLGameOnlineBlueprintLibrary::IsCurrentGameSessionPrivate(World))
									{
										return Killer;
									}
								}
							}
						}
					}

					// wrap around
					NewPlayerSpectatingIndex = NewPlayerSpectatingIndex < 0 ? NumPlayers : -1;
					for (NewPlayerSpectatingIndex += SearchDirection; (NewPlayerSpectatingIndex >= 0) && (NewPlayerSpectatingIndex < NumPlayers); NewPlayerSpectatingIndex += SearchDirection)
					{
						if (ASCCharacter* Character = GS->PlayerCharacterList[NewPlayerSpectatingIndex])
						{
							ASCPlayerState* NextPS = Character->GetPlayerState();
							if (!NextPS)
							{
								if (ASCDriveableVehicle* Vehicle = Character->GetVehicle())
								{
									if (Vehicle->Driver == Character)
									{
										NextPS = Cast<ASCPlayerState>(Vehicle->PlayerState);
									}
								}
							}

							if (NextPS && !NextPS->GetSpectating())
							{
								ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character);
								if (Counselor && !NextPS->GetEscaped())
								{
									return Counselor;
								}
								else if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Character))
								{
									if (UILLGameOnlineBlueprintLibrary::IsCurrentGameSessionPrivate(World))
									{
										return Killer;
									}
								}
							}
						}
					}

					return nullptr;
				}();

				// Update the selected player
				if (ViewTarget != SpectatingPlayer)
				{
					CurrentPlayerSpectatingIndex = NewPlayerSpectatingIndex;
					SpectatingPlayer = ViewTarget;
				}
			}
		}
	}
}

void ASCPlayerController::OnToggleSpectateMode()
{
	// Don't update if we aren't spectating
	ASCPlayerState* PS = Cast<ASCPlayerState>(PlayerState);
	if (PS == nullptr || !PS->GetSpectating())
	{
		return;
	}

	// Don't update if the match has ended
	UWorld* World = GetWorld();
	ASCGameState* GS = World ? Cast<ASCGameState>(World->GetGameState()) : nullptr;
	if (GS == nullptr || GS->HasMatchEnded() || !GS->HasMatchStarted())
	{
		return;
	}

	switch (CurrentSpectatingMode)
	{
		case ESCSpectatorMode::SpectateIntro:
		{
			// Don't allow players to skip the intro
			if (GetWorldTimerManager().GetTimerRemaining(SpectatorIntroTimer) > 0.f)
			{
				return;
			}

			// Fade the intro out
			if (!GS->IsMatchWaitingPostMatchOutro())
			{
				if (ASCInGameHUD* HUD = GetSCHUD())
				{
					HUD->OnSpectatorIntroFinished();
				}
			}

			if (!PS->DidSpawnIntoMatch())
			{
				CurrentSpectatingMode = ESCSpectatorMode::Player;
				OnViewPlayer(1);
			}
			else
			{
				CurrentSpectatingMode = ESCSpectatorMode::DeadBody;
				// Don't view the death camera if we escaped
				if (!PS->GetEscaped())
				{
					// Set up the death camera
					if (AcknowledgedPawn)
					{
						FVector CameraLocation = AcknowledgedPawn->GetActorLocation() + FVector(0.0f, 300.0f, 0.0f);
						FRotator CamneraRotation = AcknowledgedPawn->GetActorForwardVector().Rotation();
						FindDeathCameraSpot(AcknowledgedPawn, CameraLocation, CamneraRotation);
						if (!DeathCamera)
							DeathCamera = World->SpawnActor<ACameraActor>(CameraLocation, CamneraRotation);
						else
							DeathCamera->SetActorLocationAndRotation(CameraLocation, CamneraRotation); // if we spawned as TJ, show his dead body
					}

					SetViewTarget(DeathCamera);
				}
			}
			break;
		}
		case ESCSpectatorMode::DeadBody:
		{
#if !UE_BUILD_SHIPPING
			CurrentSpectatingMode = ESCSpectatorMode::FreeCam;
			ClientSetSpectatorCamera(PlayerCameraManager->GetCameraLocation(), PlayerCameraManager->GetCameraRotation());
#else
			CurrentSpectatingMode = ESCSpectatorMode::Player;
			OnViewPlayer(1);
#endif
			break;
		}
		case ESCSpectatorMode::FreeCam:
		{
			CurrentSpectatingMode = ESCSpectatorMode::Player;
			OnViewPlayer(1);
			break;
		}
		case ESCSpectatorMode::Player:
		{
			CurrentSpectatingMode = ESCSpectatorMode::LevelStatic;
			SpectatingPlayer = nullptr;
			OnViewStaticSpectatorCamera(CurrentSpectatingCameraIndex);
			break;
		}
		case ESCSpectatorMode::LevelStatic:
		{
			if (PS->GetEscaped() || !PS->DidSpawnIntoMatch())
			{
#if !UE_BUILD_SHIPPING
				CurrentSpectatingMode = ESCSpectatorMode::FreeCam;
				ClientSetSpectatorCamera(PlayerCameraManager->GetCameraLocation(), PlayerCameraManager->GetCameraRotation());
#else
				CurrentSpectatingMode = ESCSpectatorMode::Player;
				OnViewPlayer(1);
#endif
			}
			else
			{
				CurrentSpectatingMode = ESCSpectatorMode::DeadBody;
				SetViewTarget(DeathCamera);
			}
			break;
		}
		default:
		{
			CurrentSpectatingMode = ESCSpectatorMode::Player;
			OnViewPlayer(1);
		}
	}

	UpdateSpectatorHUD();
}

void ASCPlayerController::UnlockAchievements()
{
	if (ASCPlayerState* PS = Cast<ASCPlayerState>(PlayerState))
	{
		PS->UnlockAllAchievements();
	}
}

void ASCPlayerController::ClearAchievements()
{
	if (ASCPlayerState* PS = Cast<ASCPlayerState>(PlayerState))
	{
		PS->ClearAchievements();
	}
}

void ASCPlayerController::PrintPlayerStats()
{
#if !UE_BUILD_SHIPPING
	if (ASCPlayerState* PS = Cast<ASCPlayerState>(PlayerState))
	{
		PS->PrintPlayerStats();
	}
#endif
}

void ASCPlayerController::UpdateSpectatorHUD()
{
	if (CurrentSpectatingMode == ESCSpectatorMode::SpectateIntro)
	{
		// Show spectator intro
		UWorld* World = GetWorld();
		const ASCGameState* GS = World ? Cast<ASCGameState>(World->GetGameState()) : nullptr;
		ASCPlayerState* PS = Cast<ASCPlayerState>(PlayerState);
		ASCInGameHUD* HUD = GetSCHUD();
		if (GS && PS && HUD)
		{
			if (!GS->IsMatchWaitingPostMatchOutro())
			{
				// Hunt has specific criteria for showing dead/survived screens
				if (const ASCGameState_Hunt* GS_Hunt = Cast<ASCGameState_Hunt>(GS))
				{
					// We're adding one here for the spawned hunter since they will not increment the SpawnedCounselorCount
					const bool bIsLastPlayerToFinish = GS_Hunt->DeadCounselorCount + GS_Hunt->EscapedCounselorCount >= GS_Hunt->SpawnedCounselorCount + (int32)GS_Hunt->bIsHunterSpawned;
					const bool bHunterIsOrWillBeActive = GS_Hunt->HunterTimeRemaining >= 0 && !GS_Hunt->bHunterDied && !GS_Hunt->bHunterEscaped;
					if (!bIsLastPlayerToFinish || bHunterIsOrWillBeActive)
						HUD->OnShowSpectatorIntro(PS->GetIsDead(), !PS->DidSpawnIntoMatch());
				}
				// Other modes, do not
				else
				{
					HUD->OnShowSpectatorIntro(PS->GetIsDead(), !PS->DidSpawnIntoMatch());
				}
			}
		}
	}
	else
	{
		FString SpectatorModeText;
		FString NextModeText;

		if (CurrentSpectatingMode == ESCSpectatorMode::DeadBody)
		{
			ASCPlayerState* PS = Cast<ASCPlayerState>(PlayerState);
			if (PS && PS->GetEscaped())
			{
				SpectatorModeText = FText(LOCTEXT("SpectatorStatus_Escaped","You Escaped!")).ToString();
			}
			else
			{
				SpectatorModeText = FText(LOCTEXT("SpectatorStatus_YourDeadBody", "Your Dead Body")).ToString();
			}
#if !UE_BUILD_SHIPPING
			NextModeText = FText(LOCTEXT("NextModeText_FreeCamera", "Free Camera")).ToString();
#else
			NextModeText = FText(LOCTEXT("NextModeText_SpecatatePlayers", "Spectate Players")).ToString();
#endif
		}
		else if (CurrentSpectatingMode == ESCSpectatorMode::FreeCam)
		{
			SpectatorModeText = FText(LOCTEXT("SpectatorStatus_FreeCam", "Free Camera")).ToString();
			NextModeText = FText(LOCTEXT("NextModeText_SpecatatePlayers", "Spectate Players")).ToString();
		}
		else if (CurrentSpectatingMode == ESCSpectatorMode::Player)
		{
			ASCPlayerState* ViewingPS = SpectatingPlayer ? SpectatingPlayer->GetPlayerState() : nullptr;
			if (SpectatingPlayer && !ViewingPS)
			{
				if (ASCDriveableVehicle* Vehicle = SpectatingPlayer->GetVehicle())
				{
					if (Vehicle->Driver == SpectatingPlayer)
					{
						ViewingPS = Cast<ASCPlayerState>(Vehicle->PlayerState);
					}
				}
			}

			if (ViewingPS)
			{
				SpectatorModeText = FText(LOCTEXT("SpectatorStatus_SpectatingPlayer", "Spectating: ")).ToString() + ViewingPS->PlayerName;
			}
			else
			{
				SpectatorModeText = FText(LOCTEXT("SpectatorStatus_SpectatingNone", "No counselors to spectate")).ToString();
			}

			NextModeText = FText(LOCTEXT("NextModeText_CampCameras", "Camp Cameras")).ToString();
		}
		else if (CurrentSpectatingMode == ESCSpectatorMode::LevelStatic)
		{
			if (ASCStaticSpectatorCamera* LevelCamera = Cast<ASCStaticSpectatorCamera>(GetViewTarget()))
			{
				SpectatorModeText = FText(LOCTEXT("SpectatorStatus_ViewingLevelSpot", "Viewing: ")).ToString() + LevelCamera->DisplayName.ToString();
			}
			else
			{
				SpectatorModeText = FText(LOCTEXT("SpectatorStatus_ViewingNoLevelSpot", "No Camp Cameras!")).ToString();
			}

			ASCPlayerState* PS = Cast<ASCPlayerState>(PlayerState);
			if (PS && (PS->GetEscaped() || !PS->DidSpawnIntoMatch()))
			{
#if !UE_BUILD_SHIPPING
				NextModeText = FText(LOCTEXT("NextModeText_FreeCamera", "Free Camera")).ToString();
#else
				NextModeText = FText(LOCTEXT("NextModeText_SpectatePlayers", "Spectate Players")).ToString();
#endif
			}
			else
			{
				NextModeText = FText(LOCTEXT("NextModeText_YourDeadBody", "Your Dead Body")).ToString();
			}
		}

		if (!SpectatorModeText.IsEmpty())
		{
			if (ASCInGameHUD* HUD = GetSCHUD())
			{
				HUD->OnSpectatorUpdateStatus(SpectatorModeText, NextModeText);
			}
		}
	}
}

void ASCPlayerController::CLIENT_PlayHUDEndMatchOutro_Implementation(bool bIsDead, bool bIsKiller, bool bLateJoin)
{
	if (ASCInGameHUD* HUD = GetSCHUD())
	{
		HUD->CloseCounselorMap();
		HUD->UpdateCharacterHUD(true);
		HUD->OnMatchWaitingPostMatchOutro(bIsDead, bIsKiller, bLateJoin);
		HUD->DisplayEmoteSelect(false);
	}

	// we dont want screen effects anymore once the player is dead, nuke that shit
	if (ASCCharacter* SCPlayer = Cast<ASCCharacter>(GetPawn()))
	{
		if (APostProcessVolume* PPV = SCPlayer->GetPostProcessVolume())
		{
			PPV->Destroy();
			PPV = nullptr;
		}
	}
}

bool ASCPlayerController::ServerRequestSpectate_Validate()
{
	return true;
}

void ASCPlayerController::ServerRequestSpectate_Implementation()
{
	if (ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
	{
		BumpIdleTime();
		GameMode->Spectate(this);
	}
}

void ASCPlayerController::Suicide()
{
	if (IsInState(NAME_Playing))
	{
		ServerSuicide();
	}
}

bool ASCPlayerController::ServerSuicide_Validate()
{
	return true;
}

void ASCPlayerController::ServerSuicide_Implementation()
{
	if (ASCCharacter* MyPawn = Cast<ASCCharacter>(GetPawn()))
	{
		if (GetWorld()->TimeSeconds - MyPawn->CreationTime > 1 || GetNetMode() == NM_Standalone)
		{
			MyPawn->Suicide();
		}
	}
}

bool ASCPlayerController::HasGodMode() const
{
	return bGodMode;
}

ASCInGameHUD* ASCPlayerController::GetSCHUD() const
{
	return Cast<ASCInGameHUD>(GetHUD());
}

void ASCPlayerController::OnPause()
{
	// Check if the game mode says the match is over.
	if (ASCGameState* GameState = Cast<ASCGameState>(GetWorld()->GetGameState()))
	{
		const bool OnlinePlay = Cast<ASCGameState_Online>(GameState) != nullptr;
		if (GameState->IsMatchInProgress())
		{
			// if we're currently interacting with something don't pause.
			if (ASCCharacter* MyCharacter = Cast<ASCCharacter>(GetPawn()))
			{
				if (MyCharacter->IsInteractGameActive())
					return;
			}

			// If we're a counselor stop aiming on pause
			if (ASCCounselorCharacter* MyPawn = Cast<ASCCounselorCharacter>(GetPawn()))
			{
				// Don't pause if we're in a context kill unless we're in a multiplayer match... for consistency.
				if (MyPawn->IsInContextKill() && !OnlinePlay)
					return;

				MyPawn->OnAimReleased();
			}
			else if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(GetPawn()))
			{
				// Don't pause if we're in a context kill unless we're in a multiplayer match... for consistency.
				if (Killer->IsInContextKill() && !OnlinePlay)
					return;

				Killer->EndThrow();
			}

			// Just make sure the scoreboard hides if it's open when the player hits pause
			OnHideScoreboard();
			OnHideEmoteSelect();

			if (ASCInGameHUD* HUD = GetSCHUD())
			{
				// Close the map before attempting to bring up the menu
				HUD->CloseCounselorMap();
				HUD->ShowInGameMenu();
			}
		}
	}
}

void ASCPlayerController::SetInvertedYAxis(bool Inverted)
{
	FInputAxisProperties CurrentMouseAxisProps;
	FInputAxisProperties MouseAxisProps;

	if (PlayerInput && PlayerInput->GetAxisProperties(EKeys::MouseY, CurrentMouseAxisProps))
	{
		MouseAxisProps.bInvert = Inverted;
		MouseAxisProps.Exponent = CurrentMouseAxisProps.Exponent;
		MouseAxisProps.DeadZone = CurrentMouseAxisProps.DeadZone;
		MouseAxisProps.Sensitivity = CurrentMouseAxisProps.Sensitivity;
		PlayerInput->SetAxisProperties(EKeys::MouseY, MouseAxisProps);
	}
}

void ASCPlayerController::SetCurrentCursor(TEnumAsByte<EMouseCursor::Type> CursorType)
{
	CurrentMouseCursor = CursorType;
}

void ASCPlayerController::CLIENT_OnCharacterDeath_Implementation(ASCCharacter* DeadCharacter)
{
	if (DeadCharacter)
	{
		DeadCharacter->CleanupLocalAfterDeath(this);
	}
}

void ASCPlayerController::ClientMatchSummaryInfo_Implementation(const FSCEndMatchPlayerInfo& CategorizedScoreEvents)
{
	// Notify the HUD
	if (ASCInGameHUD* HUD = GetSCHUD())
	{
		HUD->OnSetEndMatchMenus(CategorizedScoreEvents.MatchScoreEvents, CategorizedScoreEvents.BadgesUnlocked);
	}
}

// NOTE: Copy of ASCPlayerController_Lobby::SendPreferencesToServer
void ASCPlayerController::SendPreferencesToServer()
{
	if (!IsLocalController() || bSentPreferencesToServer)
	{
		return;
	}

	if (USCLocalPlayer* LocalPlayer = Cast<USCLocalPlayer>(GetLocalPlayer()))
	{
		if (ASCPlayerState* SCPS = Cast<ASCPlayerState>(PlayerState))
		{
			if (USCCharacterSelectionsSaveGame* Settings = LocalPlayer->GetLoadedSettingsSave<USCCharacterSelectionsSaveGame>())
			{
				// Send our counselor and killer selections to the server
				SCPS->ServerRequestCounselor(Settings->GetPickedCounselorProfile());
				SCPS->ServerRequestKiller(Settings->GetPickedKillerProfile());
				bSentPreferencesToServer = true;
			}
		}
		
		// Send our spawn preference
		if (ASCPlayerState_Hunt* HuntPlayerState = Cast<ASCPlayerState_Hunt>(PlayerState))
		{
			if (USCCharacterPreferencesSaveGame* Settings = LocalPlayer->GetLoadedSettingsSave<USCCharacterPreferencesSaveGame>())
			{
				HuntPlayerState->ServerSetSpawnPreference(Settings->CharacterPreference);
			}
		}
	}

	// Update our HUD
	if (ASCInGameHUD* HUD = GetSCHUD())
	{
		HUD->UpdateCharacterHUD();
	}
}

void ASCPlayerController::UpdateStreamerSettings(bool bStreamerMode)
{
	// only update if streamer mode has changed.
	if (CurrentStreamerMode != bStreamerMode)
	{
		CurrentStreamerMode = bStreamerMode;

		// Update the radios for Streaming Mode.
		for (TActorIterator<ASCRadio> ActorItr(GetWorld()); ActorItr; ++ActorItr) // OPTIMIZE: pjackson
		{
			if (ASCRadio* Radio = *ActorItr)
			{
				Radio->UpdateStreamerMode(bStreamerMode);
			}
		}

		// Update the car radio for Streaming mode.
		for (TActorIterator<ASCDriveableVehicle> ActorItr(GetWorld()); ActorItr; ++ActorItr) // OPTIMIZE: pjackson
		{
			if (ASCDriveableVehicle* DrivableVehicle = *ActorItr)
			{
				DrivableVehicle->UpdateStreamerMode(bStreamerMode);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
