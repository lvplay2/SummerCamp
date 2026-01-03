// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCAnimInstance.h"

#include "AIController.h"
#include "AnimInstanceProxy.h"
#include "Animation/AnimMontage.h"

#include "SCCharacterMovement.h"
#include "SCCounselorCharacter.h"
#include "SCGameInstance.h"
#include "SCPerkData.h"
#include "SCPlayerState.h"
#include "SCWeapon.h"

DECLARE_CYCLE_STAT(TEXT("Anim Update Start Stop"), Stat_AnimStartStop, STATGROUP_Anim);
DECLARE_CYCLE_STAT(TEXT("Anim Update Start Stop: Idle"), Stat_AnimStartStop_Idle, STATGROUP_Anim);
DECLARE_CYCLE_STAT(TEXT("Anim Update Start Stop: Turn"), Stat_AnimStartStop_Turn, STATGROUP_Anim);
DECLARE_CYCLE_STAT(TEXT("Anim Update Start Stop: Start"), Stat_AnimStartStop_Start, STATGROUP_Anim);
DECLARE_CYCLE_STAT(TEXT("Anim Update Start Stop: Move"), Stat_AnimStartStop_Move, STATGROUP_Anim);
DECLARE_CYCLE_STAT(TEXT("Anim Update Start Stop: Stop"), Stat_AnimStartStop_Stop, STATGROUP_Anim);
DECLARE_CYCLE_STAT(TEXT("Anim Update Start Stop: Juke"), Stat_AnimStartStop_Juke, STATGROUP_Anim); // Unused
DECLARE_CYCLE_STAT(TEXT("Anim Update Start Stop: Transition"), Stat_AnimStartStop_Transition, STATGROUP_Anim);


const FName NAME_AccelerationCurve(TEXT("Acceleration"));
const FName NAME_FrameCurve(TEXT("Frame"));
const FName NAME_LeftPlantSync(TEXT("LeftPlant"));
const FName NAME_LocomotionGroup(TEXT("Locomotion"));
const FName NAME_RotationCurve(TEXT("rotationOfPivot"));

USCAnimInstance::USCAnimInstance(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, IgnoreTurnTransitionLimit(45.f)
, MaxLeanSpeed(300.f)
, LeanSmoothSpeed(5.f)
, BlockedStopDistance(100.f)
, BlockedMaxStopSpeed(EMovementSpeed::Run)
, OutOfCombat(1.f)
, WalkFootstepLoudness(0.25f)
, RunFootstepLoudness(0.5f)
, SprintFootstepLoudness(0.75f)
, MaxAcceleration(2048.f)
, BrakingDecelerationWalking(2048.f)

// Start Machine Transition Cache
, StartToIdleRatio(0.5f)
{
	bAutomaticallyCollectRawCurves = true;
}

void USCAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OnMontageStarted.AddDynamic(this, &USCAnimInstance::OnMontagePlay);

	Player = Cast<ASCCharacter>(GetOwningActor());
	if (Player)
	{
		MaxAcceleration = Player->GetCharacterMovement()->MaxAcceleration;
		BrakingDecelerationWalking = Player->GetCharacterMovement()->BrakingDecelerationWalking;
		bUsingStartStopTransitions = bUseStartStopTransitions;
	}
}

void USCAnimInstance::NativeUninitializeAnimation()
{
	Super::NativeUninitializeAnimation();

	OnMontageStarted.Clear();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Attack_TimerHandle);
		World->GetTimerManager().ClearTimer(Recoil_TimerHandle);
		World->GetTimerManager().ClearTimer(Impact_TimerHandle);
	}
}

float USCAnimInstance::GetMoveSpeedState(const float CurrentSpeed) const
{
	if (CurrentSpeed <= KINDA_SMALL_NUMBER)
		return (float)EMovementSpeed::Idle;

	// Combat stance doesn't allow running or sprinting
	if (Player->InCombatStance())
		return (float)EMovementSpeed::Walk;

	return (float)(Player->IsSprinting() ? EMovementSpeed::Sprint : (Player->IsRunning() ? EMovementSpeed::Run : EMovementSpeed::Walk));
}

float USCAnimInstance::GetMoveSpeedState() const
{
	if (Player)
	{
		return GetMoveSpeedState(Player->GetVelocity().Size());
	}

	return 0.f;
}

float USCAnimInstance::GetMovementPlaybackRate() const
{
#if 0 // Disabled for now per GUN feedback
	if (!Player)
		return 1.f;

	return Player->GetSpeedModifier();
#else
	return 1.f;
#endif
}

EMovementSpeed USCAnimInstance::GetDesiredStartMoveState() const
{
	if (Player->IsSprinting())
		return EMovementSpeed::Sprint;

	if (Player->IsRunning())
		return EMovementSpeed::Run;

	return EMovementSpeed::Walk;
}

EMovementSpeed USCAnimInstance::GetDesiredStopMoveState(const float CurrentSpeed, float& SpeedDifference) const
{
	USCCharacterMovement* CharacterMovement = Cast<USCCharacterMovement>(Player->GetCharacterMovement());

	EMovementSpeed DesiredStopSpeed = EMovementSpeed::Walk;
	SpeedDifference = FMath::Abs(CurrentSpeed - CharacterMovement->MaxWalkSpeed);

	const float SprintSpeedDifference = FMath::Abs(CurrentSpeed - CharacterMovement->MaxSprintSpeed);
	if (SprintSpeedDifference < SpeedDifference)
	{
		DesiredStopSpeed = EMovementSpeed::Sprint;
		SpeedDifference = SprintSpeedDifference;
	}

	const float RunSpeedDifference = FMath::Abs(CurrentSpeed - CharacterMovement->MaxRunSpeed);
	if (RunSpeedDifference < SpeedDifference)
	{
		DesiredStopSpeed = EMovementSpeed::Run;
		SpeedDifference = RunSpeedDifference;
	}

	// See if there's a wall in front of us
	if (BlockedStopDistance > 0.f)
	{
		const FVector Start = Player->GetActorLocation();
		const FVector End = Start + Player->GetActorRotation().Vector() * BlockedStopDistance;

		static const FName NAME_StopWallCheck = FName(TEXT("StopWallCheck"));
		FCollisionQueryParams TraceParams(NAME_StopWallCheck, false, Player);
		TraceParams.bTraceAsyncScene = true;

		TArray<FHitResult> HitResults;
		const bool bFoundBlocker = GetWorld()->LineTraceMultiByChannel(HitResults, Start, End, ECollisionChannel::ECC_Visibility, TraceParams);

		if (bFoundBlocker)
		{
			DesiredStopSpeed = FMath::Min(BlockedMaxStopSpeed, DesiredStopSpeed);
		}
	}

	return DesiredStopSpeed;
}

void USCAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (Player == nullptr)
		return;

	if (StunMontage)
	{
		if (!Montage_IsPlaying(StunMontage))
		{
			StunMontage = nullptr;
		}
	}

	MovementPlaybackRate = GetMovementPlaybackRate();

	const FRotator Rotation = Player->GetActorRotation();
	const FVector Velocity = Player->GetVelocity();
	const FVector UnrotatedVelocity = Rotation.UnrotateVector(Velocity);

	ForwardVelocity = UnrotatedVelocity.X;
	RightVelocity = UnrotatedVelocity.Y;

	Speed = Velocity.Size();
	if (Speed <= KINDA_SMALL_NUMBER)
	{
		Direction = 0.f;
		MoveSpeedState = GetMoveSpeedState(Speed);
		bIsMoving = false;
	}
	else
	{
		Direction = CalculateDirection(Velocity, Rotation);
		MoveSpeedState = GetMoveSpeedState(Speed);
		bIsMoving = true;
	}

	if (IsAnyMontagePlaying())
	{
		const UAnimMontage* Montage = GetCurrentActiveMontage();
		bIsMontageUsingRootMotion = Montage ? Montage->HasRootMotion() : false;
	}
	else
	{
		bIsMontageUsingRootMotion = false;
	}

	bIsInSpecialMove = Player->IsInSpecialMove();
	const ESCCharacterStance NewStance = Player->CurrentStance;
	if (CurrentStance != NewStance)
	{
		if (NewStance == ESCCharacterStance::Combat)
		{
			EnterCombatStance();
		}
		else if (CurrentStance == ESCCharacterStance::Combat)
		{
			LeaveCombatStance();
		}
	}

	CurrentStance = NewStance;

	// camera settings
	if (Player->IsLocallyControlled())
	{
		if (Cast<AAIController>(Player->Controller))
		{
			Player->GetActorEyesViewPoint(CameraLocation, CameraRotation);
		}
		else
		{
			CameraLocation = Player->GetCachedCameraInfo().Location;
			CameraRotation = Player->GetCachedCameraInfo().Rotation;
		}
	}
	TargetLookAtRotation = Player->GetHeadLookAtRotation();

	// Sync group right/left foot handling
	const FMarkerSyncAnimPosition SyncPosition = GetSyncGroupPosition(NAME_LocomotionGroup);
	bOnLeftFoot = SyncPosition.PreviousMarkerName.IsEqual(NAME_LeftPlantSync);

	const FVector MovementVector = FVector(Player->GetRequestedMoveInput(), 0.f);
	const FVector LocalMovementVector = FRotator(0.f, CameraRotation.Yaw, 0.f).GetInverse().RotateVector(MovementVector);
	RequestedMoveInput = FVector2D(LocalMovementVector);

	UCharacterMovementComponent* CharacterMovement = Player->GetCharacterMovement();
	bIsFalling = (bIsMontageUsingRootMotion || bIsInSpecialMove) ? false : CharacterMovement->IsFalling();

	if (bIsFalling)
	{
		InAirTime += DeltaSeconds;
		Log(FColor::Orange, false, FString::Printf(TEXT(" - Falling for %.2f seconds"), InAirTime));
	}
	else
	{
		InAirTime = 0.f;
	}


	// We don't want to do starts/stops with combat.... yet.
	if (Player->InCombatStance())
	{
		Log(FColor::Yellow, false, TEXT("Aborting due to character being in combat."));
		return;
	}

	// Leans
	const bool bShouldLean = (bUseStartStopTransitions == false || CurrentMoveState == EMovementState::Move) && CurrentStance != ESCCharacterStance::Combat;
	float DesiredLeanWorldDirection = 0.f;
	if (bShouldLean)
	{
		const float LeanAlphaTarget = FMath::Clamp(Speed / FMath::Max(MaxLeanSpeed, KINDA_SMALL_NUMBER), 0.f, 1.f);
		LeanAlpha = FMath::FInterpTo(LeanAlpha, LeanAlphaTarget, DeltaSeconds, LeanSmoothSpeed);
		DesiredLeanWorldDirection = CalculateDirection(MovementVector, FRotator::ZeroRotator);
		if (FMath::Abs(LeanDirectionWorld - DesiredLeanWorldDirection) > 180.f)
		{
			if (DesiredLeanWorldDirection > 0.f)
				DesiredLeanWorldDirection -= 360.f;
			else
				DesiredLeanWorldDirection += 360.f;
		}

		LeanDirectionWorld = FRotator::NormalizeAxis(FMath::FInterpTo(LeanDirectionWorld, DesiredLeanWorldDirection, DeltaSeconds, LeanSmoothSpeed));
		Log(FColor::Orange, false, FString::Printf(TEXT(" - Leaning: Alpha %.2f  World %.2f  Local %.2f"), LeanAlpha, LeanDirectionWorld, LeanDirectionFiltered));
	}
	else
	{
		LeanAlpha = 0.f;
		LeanDirectionWorld = Rotation.Yaw;
	}

	LeanDirectionFiltered = FRotator::NormalizeAxis(LeanDirectionWorld - Rotation.Yaw);

	const bool bMoveRequested = RequestedMoveInput.SizeSquared() > SMALL_NUMBER;
	RequestedMoveDirection = bMoveRequested ? -CalculateDirection(MovementVector, Rotation) : 0.f;

	// Used in place of early out calls so we can setup blueprint specific variables with all data in mind at the end of this update tick
	bool bShouldUseStartStops = true;

	// Early out for those who don't want smooth lengthy transitions
	if (!bUseStartStopTransitions)
	{
		CurrentMoveState = bMoveRequested ? EMovementState::Move : EMovementState::Idle;
		bShouldUseStartStops = false;
	}
	else if (CharacterMovement->MovementMode != MOVE_Walking && CharacterMovement->MovementMode != MOVE_None)
	{
		if (CVarILLDebugAnimation.GetValueOnGameThread() > 0)
		{
			const UEnum* StateMode = FindObject<UEnum>(ANY_PACKAGE, TEXT("EMovementMode"), true);
			Log(FColor::Yellow, false, FString::Printf(TEXT("Aborting due to character not being in the walking or disabled movement state. In %s state"), *StateMode->GetNameStringByValue((int32)CharacterMovement->MovementMode)));
		}
		bShouldUseStartStops = false;
	}
	else if (Player->IsInContextKill())
	{
		bShouldUseStartStops = false;
	}
	// CLEANUP: lpederson - Boy howdy this is ugly.
	else if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Player))
	{
		if (Killer->bUnderWater)
		{
			bShouldUseStartStops = false;
		}
	}

	SetUsingStartStopTransitions(bShouldUseStartStops);

	if (bShouldUseStartStops)
	{
		SCOPE_CYCLE_COUNTER(Stat_AnimStartStop);

		// Don't update move state if we're in a special move or otherwise being driven by root motion
		if (!bIsInSpecialMove && !bIsMontageUsingRootMotion)
		{
			const EMovementState OldMoveState = CurrentMoveState;
			switch (CurrentMoveState)
			{
			case EMovementState::Idle:
				{
					SCOPE_CYCLE_COUNTER(Stat_AnimStartStop_Idle);
					if (bMoveRequested)
					{
						TransitionMovementState(EMovementState::Start);
						bStartingToTheLeft = RequestedMoveDirection > 0.f;
					}
				}
				break;

			case EMovementState::Turn:
				{
					SCOPE_CYCLE_COUNTER(Stat_AnimStartStop_Turn);
					// Apply rotation for this frame
					// DGarcia: Removed this for now because it was causing remote clients to wig out
					//const float RotationCurve = GetUnblendedCurveValue(NAME_RotationCurve);
					//Player->SetActorRotation(FRotator(StartTurnRotation.Pitch, FRotator::ClampAxis(StartTurnRotation.Yaw - RotationCurve), StartTurnRotation.Roll), ETeleportType::TeleportPhysics);
				}
				break;

			case EMovementState::Start:
				{
					SCOPE_CYCLE_COUNTER(Stat_AnimStartStop_Start);
					// Apply rotation for this frame
					const float RotationCurve = GetUnblendedCurveValue(NAME_RotationCurve);
					const FRotator ActorRotation = FMath::RInterpTo(Player->GetActorRotation(), FRotator(StartTurnRotation.Pitch, FRotator::NormalizeAxis(StartTurnRotation.Yaw - RotationCurve), StartTurnRotation.Roll), DeltaSeconds, 4.0f);
					Player->SetActorRotation(ActorRotation, ETeleportType::TeleportPhysics);
					Log(FColor::Purple, false, FString::Printf(TEXT(" - Rotating towards requested move direction (%.2f / %.2f)"), RotationCurve, RequestedStartDirection));

					// Update our transition speed and let the blendspace take care of making it look good
					DesiredStartTransitionSpeed = GetDesiredStartMoveState();
					DesiredStartTransitionSpeedAsFloat = DesiredMoveSpeedState = (float)DesiredStartTransitionSpeed;

					// Handle our start direction trying to skip over the -180/180 or 0 barriers
					const float NewStartDirection = bMoveRequested ? -CalculateDirection(MovementVector, StartTurnRotation) : 0.f;
					if (bStartingToTheLeft == (NewStartDirection > 0.f))
					{
						// Don't allow start direction to dip below how far we've already turned
						if (FMath::Abs(RotationCurve) - FMath::Abs(NewStartDirection) < 0.1f)
							RequestedStartDirection = NewStartDirection;
					}
					else
					{
						const float CurrentTime = GetWorld()->GetTimeSeconds();
						// If the difference between the old direction and the new are small enough, we can jump the 0 barrier and switch hemispheres
						if (FMath::Abs(RequestedStartDirection - NewStartDirection) < 45.f && CurrentTime - StartStarted_Timestamp < 0.2f)
						{
							Log(FColor::Red, true, FString::Printf(TEXT(" - Switching hemispheres (%s) O: %.1f N: %.1f"), NewStartDirection > 0.f ? TEXT("Left") : TEXT("Right"), RequestedStartDirection, NewStartDirection));

							RequestedStartDirection = NewStartDirection;
							bStartingToTheLeft = RequestedStartDirection > 0.f;
						}
						// This is the +/-180 barrier, we don't want to jump that one so we'll just clamp the value and hope that the motion hides things enough
						else
						{
							RequestedStartDirection = FRotator::NormalizeAxis(RequestedStartDirection);//FMath::Abs(RequestedStartDirection) > 90.f ? 180.f * FMath::Sign(RequestedStartDirection) : 0.f;
						}
					}

					// Apply any extra needed offset
					if (bUseRootOffset && Speed > KINDA_SMALL_NUMBER)
					{
						// Rotate our root (smoothly) to make up for rotation we can't handle in the blendspace
						const float RotationToIgnore = RotationCurve - RequestedStartDirection;
						const FRotator TargetOffset = FRotator(0.f, FRotator::NormalizeAxis(Direction - RotationToIgnore), 0.f);

						MeshRootOffset = FMath::RInterpTo(MeshRootOffset, TargetOffset, DeltaSeconds, 4.0f);
						//MeshRootOffset = FRotator(0.f, TargetOffset.Yaw, 0.f);

						Log(FColor::Purple, false, FString::Printf(TEXT(" - Root offset %.2f"), MeshRootOffset.Yaw));
					}

					if (!bMoveRequested)
					{
						TransitionMovementState(EMovementState::Stop);
						Log(FColor::Red, true, FString(TEXT(" - Movement no longer requested")));
					}
					else
					{
						// Only accelerate locally
						if (Player->IsLocallyControlled())
						{
							const float TransitionFrameNumber = GetUnblendedCurveValue(NAME_AccelerationCurve);
							Log(FColor::Purple, false, FString::Printf(TEXT(" - Acceleration %.2f"), TransitionFrameNumber));

							if (TransitionFrameNumber > 0.f)
							{
								CharacterMovement->MaxAcceleration = FMath::Min(CharacterMovement->GetMaxSpeed() * (TransitionFrameNumber * 30.f), MaxAcceleration);
							}
						}

						StopSpeed = Speed;
					}
				}
				break;

			case EMovementState::Move:
				{
					SCOPE_CYCLE_COUNTER(Stat_AnimStartStop_Move);
					DesiredMoveSpeedState = MoveSpeedState;

					if (!bMoveRequested)
					{
						TransitionMovementState(EMovementState::Stop);
						Log(FColor::Red, true, FString(TEXT(" - Movement no longer requested")));
					}
					else
					{
						StopSpeed = Speed;

						if (bUseRootOffset)
						{
							MeshRootOffset.Yaw = FMath::RInterpTo(FRotator(MeshRootOffset.Yaw,0,0), FRotator::ZeroRotator, DeltaSeconds, 4.0f).Pitch;

							Log(FColor::Purple, false, FString::Printf(TEXT(" - Root offset %.2f"), MeshRootOffset.Yaw));
						}
					}
				}
				break;

			case EMovementState::Stop:
				{
					SCOPE_CYCLE_COUNTER(Stat_AnimStartStop_Stop);
					if (bMoveRequested)
					{
						TransitionMovementState(EMovementState::Start);
					}

					// Only decelerate locally
					if (Player->IsLocallyControlled())
					{
						const float TransitionFrameNumber = GetUnblendedCurveValue(NAME_FrameCurve);
						Log(FColor::Purple, false, FString::Printf(TEXT(" - Transition Frame: %.2f, Stop Speed: %.2f"), TransitionFrameNumber, StopSpeed));

						if (TransitionFrameNumber >= 0.f)
						{
							CharacterMovement->BrakingDecelerationWalking = FMath::Min(StopSpeed / (FMath::Max(TransitionFrameNumber, KINDA_SMALL_NUMBER) * (1.f / 30.f)), BrakingDecelerationWalking);
						}
					}
				}
				break;

			case EMovementState::Juke: // Not implemented
			case EMovementState::MAX:
			default:
				break;
			}

			PreviousMoveState = OldMoveState;
		}

		if (CVarILLDebugAnimation.GetValueOnGameThread()  > 0)
		{
			const UEnum* StateEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EMovementState"), true);
			const UEnum* ModeEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EMovementMode"), true);
			// - State (Mode) Speed: Speed Max: Max Accl: Acceleration Dccl: Deceleration Input: RequestedInput Sync: (PrevFoot SyncPositionDelta NextFoot)
			Log(FColor::Purple, false, 
				FString::Printf(TEXT(" - %s (%s) Speed: %.1f Max: %.1f Accl: %.2f Dccl: %.2f Input: (%s) Sync: (%s %.2f %s) %s"),
					*StateEnum->GetNameStringByValue((int32)CurrentMoveState), *ModeEnum->GetNameStringByValue((int32)CharacterMovement->MovementMode), Speed, CharacterMovement->GetMaxSpeed(), CharacterMovement->MaxAcceleration, CharacterMovement->BrakingDecelerationWalking,
					*RequestedMoveInput.ToString(), *SyncPosition.PreviousMarkerName.ToString(), SyncPosition.PositionBetweenMarkers < 0.f ? -1.f : SyncPosition.PositionBetweenMarkers, *SyncPosition.NextMarkerName.ToString(), bOnLeftFoot ? TEXT("Left") : TEXT("Right"))
				);
			static const float LINE_SIZE = 150.f;
			static const float ARROW_SIZE = 15.f;

			const FVector ActorLocation = Player->GetActorLocation();
			const FRotator ActorRotation = Player->GetActorRotation();
			const FRotator RequestedMoveRotation = StartTurnRotation + FRotator(0.f, -RequestedMoveDirection, 0.f);

			DrawDebugDirectionalArrow(GetWorld(), ActorLocation, ActorLocation + StartTurnRotation.RotateVector(FVector::ForwardVector) * LINE_SIZE, ARROW_SIZE, FColor::Red, false, 0.f, 0, 1.f); // Start
			DrawDebugDirectionalArrow(GetWorld(), ActorLocation, ActorLocation + ActorRotation.RotateVector(FVector::ForwardVector) * LINE_SIZE, ARROW_SIZE, FColor::Yellow, false, 0.f, 0, 1.f); // Current
			DrawDebugDirectionalArrow(GetWorld(), ActorLocation, ActorLocation + (RequestedMoveRotation + ActorRotation).RotateVector(FVector::ForwardVector) * LINE_SIZE, ARROW_SIZE, FColor::Green, false, 0.f, 0, 1.f); // Target

			Log(FColor::Purple, false, 
				FString::Printf(TEXT(" - StartRot: %.3f ActorRot: %.3f ReqDir: %.3f"),
					StartTurnRotation.Yaw, ActorRotation.Yaw, RequestedMoveDirection)
				);
		}
	}

	// State Machine Transition Cache
	bInIdleState = (CurrentMoveState == EMovementState::Idle);
	bInMoveState = (CurrentMoveState == EMovementState::Move);
	bInStopState = (CurrentMoveState == EMovementState::Stop);
	bStartStop_IdleToStart = (CurrentMoveState == EMovementState::Start) && !bIsMontageUsingRootMotion;
	const float AccelerationCurve = GetCurveValue(NAME_AccelerationCurve);
	bStartStop_StartToIdle = (CurrentMoveState == EMovementState::Stop) && (AccelerationCurve < StartToIdleRatio);
	bStartStop_StopToStart = bStartStop_IdleToStart;
	bStartStop_StartToStop = (CurrentMoveState == EMovementState::Stop) && (AccelerationCurve >= StartToIdleRatio);
}

bool USCAnimInstance::TransitionMovementState(const EMovementState NewState)
{
	if (Player == nullptr)
		return false;

	// Not implemented
	if (NewState == EMovementState::Juke)
		return false;

	// Don't switch if we're already in the state
	if (CurrentMoveState == NewState)
		return false;

	// Early out for those who don't want smooth lengthy transitions
	if (bUseStartStopTransitions == false)
		return false;

	if (CVarILLDebugAnimation.GetValueOnGameThread() > 0)
	{
		const UEnum* ResourceEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EMovementState"), true);
		Log(FColor::Red, true, FString::Printf(TEXT(" - Transitioning from %s to %s"), *ResourceEnum->GetNameStringByValue((int32)CurrentMoveState), *ResourceEnum->GetNameStringByValue((int32)NewState)));
	}

	SCOPE_CYCLE_COUNTER(Stat_AnimStartStop_Transition);
	USCCharacterMovement* CharacterMovement = Cast<USCCharacterMovement>(Player->GetCharacterMovement());

	// Exit
	switch (CurrentMoveState)
	{
	case EMovementState::Idle:
		break;

	case EMovementState::Turn:
		break;

	case EMovementState::Start:
		break;

	case EMovementState::Move:
		StopSpeed = CharacterMovement->GetMaxSpeed();
		break;

	case EMovementState::Stop:
		// Character movement properties only happen on the local client
		if (Player->IsLocallyControlled())
		{
			CharacterMovement->GroundFriction = 8.f;
		}
		break;

	case EMovementState::Juke: // Not implemented -- should never have been in this state, what is going on?!
	default:
		return false;
	}

	// Enter
	switch (NewState)
	{
	case EMovementState::Turn:
		{
			StartTurnRotation = Player->GetActorRotation();
		}
		//break; Fallthrough

	case EMovementState::Idle:
		{
			// Character movement properties only happen on the local client
			if (Player->IsLocallyControlled())
			{
				CharacterMovement->bOrientRotationToMovement = false;
			}

			DesiredMoveSpeedState = 0.f;
		}
		break;

	case EMovementState::Start:
		{
			StartTurnRotation = Player->GetActorRotation();
			bDoneRotatingForStart = false;

			RequestedStartDirection = RequestedMoveDirection;
			bStartingToTheLeft = RequestedStartDirection > 0.f;

			StartStarted_Timestamp = GetWorld()->GetTimeSeconds();

			// Character movement properties only happen on the local client
			if (Player->IsLocallyControlled())
			{
				CharacterMovement->bOrientRotationToMovement = false;

				CharacterMovement->BrakingDecelerationWalking = BrakingDecelerationWalking;
				CharacterMovement->MaxAcceleration = 0.f;
			}

			DesiredStartTransitionSpeed = GetDesiredStartMoveState();
			DesiredStartTransitionSpeedAsFloat = (float)DesiredStartTransitionSpeed;
		}
		break;

	case EMovementState::Move:
		{
			// Character movement properties only happen on the local client
			if (Player->IsLocallyControlled())
			{
				CharacterMovement->BrakingDecelerationWalking = BrakingDecelerationWalking;
				CharacterMovement->MaxAcceleration = MaxAcceleration;

				if (!Player->InCombatStance())
				{
					CharacterMovement->bOrientRotationToMovement = true;
				}
			}
		}
		break;

	case EMovementState::Stop:
		{
			if (bUseRootOffset)
			{
				Player->SetActorRotation(Player->GetActorRotation() + MeshRootOffset, ETeleportType::TeleportPhysics);
				MeshRootOffset = FRotator::ZeroRotator;
			}

			float StopSpeedDifference;
			DesiredStopTransitionSpeed = GetDesiredStopMoveState(StopSpeed, StopSpeedDifference);
			DesiredStopTransitionSpeedAsFloat = DesiredMoveSpeedState = (float)DesiredStopTransitionSpeed;

			const FMarkerSyncAnimPosition SyncPosition = GetSyncGroupPosition(NAME_LocomotionGroup);
			float TimeToNextMarker = 0.f;
			GetTimeToClosestMarker(NAME_LocomotionGroup, SyncPosition.NextMarkerName, TimeToNextMarker);
			// If we're too close to passing over to the next foot then use the other foot to stop
			bStopOnLeftFoot = bOnLeftFoot;

			if (CVarILLDebugAnimation.GetValueOnGameThread() > 0)
			{
				const UEnum* ResourceEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EMovementSpeed"), true);
				Log(FColor::Red, true, 
					FString::Printf(TEXT(" - Stopping on %s foot, %.2f to %s. Stop Speed is %.1f off by %.1f from %s"),
						bStopOnLeftFoot ? TEXT("Left") : TEXT("Right"), TimeToNextMarker, *SyncPosition.NextMarkerName.ToString(),
						StopSpeed, StopSpeedDifference, *ResourceEnum->GetNameStringByValue((int32)DesiredStopTransitionSpeed))
					);
			}

			// Character movement properties only happen on the local client
			if (Player->IsLocallyControlled())
			{
				// Make sure we don't let the character movement component stop the player before we do
				CharacterMovement->BrakingDecelerationWalking = 0.f;
				CharacterMovement->GroundFriction = 0.f;
				CharacterMovement->Velocity = Player->GetActorRotation().RotateVector(FVector::ForwardVector) * StopSpeed;
				CharacterMovement->bOrientRotationToMovement = false;
			}
		}
		break;

	case EMovementState::Juke: // Not implemented
	default:
		return false;
	}

	PreviousMoveState = CurrentMoveState;
	CurrentMoveState = NewState;

	return true;
}

void USCAnimInstance::SetUsingStartStopTransitions(const bool bUseTransitions)
{
	// Don't update movement mode more than once, or ever if we're not using start/stop transitions at all
	if (bUsingStartStopTransitions == bUseTransitions || !bUseStartStopTransitions)
		return;

	// If we turn off start/stops, make sure we can move like normal people
	if (!bUseTransitions)
	{
		if (Player->IsLocallyControlled())
		{
			UCharacterMovementComponent* CharacterMovement = Player->GetCharacterMovement();
			CharacterMovement->BrakingDecelerationWalking = BrakingDecelerationWalking;
			CharacterMovement->MaxAcceleration = MaxAcceleration;
			CharacterMovement->bOrientRotationToMovement = true;
			//CharacterMovement->GroundFriction = 8.f; -- Disabled for now because the killer wants to set this when underwater and we're doing ugly shit to make sure that happens. No, it's fine. I do feel bad and so should you.
		}
	}

	bUsingStartStopTransitions = bUseTransitions;
}

UAnimSequenceBase* USCAnimInstance::GetCurrentItemIdlePose() const
{
	const ASCItem* Item = Player ? Player->GetEquippedItem() : nullptr;
	if (!Item)
		return nullptr;

	return Item->GetIdlePose(CurrentSkeleton);
}

UAnimSequenceBase* USCAnimInstance::GetCurrentWeaponCombatPose() const
{
	const ASCWeapon* Weapon = Player ? Player->GetCurrentWeapon() : nullptr;
	if (!Weapon)
		return nullptr;

	return Weapon->GetCombatIdlePose(CurrentSkeleton, Player->GetIsFemale());
}

void USCAnimInstance::PlayAttackAnim(bool HeavyAttack /*= false*/)
{
	const ASCWeapon* Weapon = Player ? Player->GetCurrentWeapon() : nullptr;
	if (Weapon == nullptr)
		return;

	CachedAttackData = Weapon->GetNeutralAttack(CurrentSkeleton, Player->GetIsFemale());
	float MontageLength = 0.0f;
	float MontagePlaySpeed = 1.0f;

	// Apply perk modifiers
	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Player);
	if (ASCPlayerState* PS = Counselor ? Counselor->GetPlayerState() : nullptr)
	{
		for (const USCPerkData* Perk : PS->GetActivePerks())
		{
			check(Perk);

			if (!Perk)
				continue;

			MontagePlaySpeed += Perk->AttackSpeedModifier;
		}
	}

	if (HeavyAttack)
	{
		UAnimMontage* HeavyAttackMontage = Weapon->GetForwardAttack(CurrentSkeleton, Player->GetIsFemale()).AttackMontage;
		if (bIsAttacking)
		{
			Montage_Stop(0.0f, CachedAttackData.AttackMontage);
			if (HeavyAttackMontage)
			{
				const float CurLoc = Montage_GetPosition(CachedAttackData.AttackMontage) / FMath::Max(CachedAttackData.AttackMontage->GetPlayLength(), KINDA_SMALL_NUMBER);
				MontageLength = Montage_Play(HeavyAttackMontage, MontagePlaySpeed);
				Montage_SetPosition(HeavyAttackMontage, HeavyAttackMontage->GetPlayLength() * CurLoc);
			}
			GetWorld()->GetTimerManager().ClearTimer(Attack_TimerHandle);
		}
		else
		{
			MontageLength = Montage_Play(HeavyAttackMontage, MontagePlaySpeed);
		}

		CachedAttackData = Weapon->GetForwardAttack(CurrentSkeleton, Player->GetIsFemale());
		bIsHeavyAttack = true;
	}
	else
	{
		MontageLength = Montage_Play(CachedAttackData.AttackMontage, MontagePlaySpeed);
	}

	if (MontageLength > 0.0f)
	{
		FTimerDelegate Delegate;
		Delegate.BindLambda([this]() { bIsAttacking = false; bIsHeavyAttack = false; } );
		GetWorld()->GetTimerManager().SetTimer(Attack_TimerHandle, Delegate, MontageLength, false);
	}

	bIsAttacking = true;
}

void USCAnimInstance::PlayRecoilAnim()
{
	bIsRecoiling = true;

	GetWorld()->GetTimerManager().ClearTimer(Attack_TimerHandle);
	bIsAttacking = false;
	bIsHeavyAttack = false;

	if (CachedAttackData.RecoilMontage)
	{
		const float MontageLength = Montage_Play(CachedAttackData.RecoilMontage);

		if (MontageLength > 0.f)
		{
			FTimerDelegate Delegate;
			Delegate.BindLambda([this]() { bIsRecoiling = false; bIsHeavyAttack = false; } );
			GetWorld()->GetTimerManager().SetTimer(Recoil_TimerHandle, Delegate, MontageLength, false);
		}
		else
		{
			bIsRecoiling = false;
		}
	}
	else
	{
		bIsRecoiling = false;
	}
}

void USCAnimInstance::PlayWeaponImpactAnim()
{
	bIsWeaponStuck = true;

	GetWorld()->GetTimerManager().ClearTimer(Attack_TimerHandle);

	if (CachedAttackData.RecoilMontage)
	{
		const float MontageLength = Montage_Play(CachedAttackData.ImpactMontage);
		
		if (MontageLength > 0.f)
		{
			FTimerDelegate Delegate;
			Delegate.BindLambda([this]() { bIsWeaponStuck = false; bIsAttacking = false; bIsHeavyAttack = false; } );
			GetWorld()->GetTimerManager().SetTimer(Impact_TimerHandle, Delegate, MontageLength, false);
		}
		else
		{
			bIsWeaponStuck = false;
		}
	}
	else
	{
		bIsWeaponStuck = false;
	}
}

int32 USCAnimInstance::ChooseBlockAnim() const
{
	return FMath::RandRange(0, FMath::Max(0, BlockMontages.Num() - 1));
}

void USCAnimInstance::PlayBlockAnim(int32 Index/* = 0 */)
{
	if (BlockMontages.Num() <= 0)
		return;

	Montage_Play(BlockMontages[Index]);
}

void USCAnimInstance::EndBlockAnim(int32 Index /* = 0 */)
{
	if (BlockMontages.Num() <= 0)
		return;

	Montage_JumpToSection(TEXT("Exit"), BlockMontages[Index]);
}

float USCAnimInstance::GetBlockAnimLength(int32 Index)
{
	if (BlockMontages.Num() <= 0)
		return 0.0f;

	return BlockMontages[Index]->GetPlayLength();
}

int32 USCAnimInstance::ChooseHitAnim(ESCCharacterHitDirection HitDirection, bool bBlocking/* = false*/) const
{
	const int32 NumMontages = bBlocking ? BlockingHitMontages.Num() - 1 : HitMontages[(int32)HitDirection].Montages.Num() - 1;
	return FMath::RandRange(0, FMath::Max(0, NumMontages));
}

void USCAnimInstance::PlayHitAnim(ESCCharacterHitDirection HitDirection, int32 Index/* = 0 */, bool bBlocking/* = false*/)
{
	// If we're stunned we don't want to play a raction.
	if (IsStunned())
		return;

	const TArray<UAnimMontage*> MontageArray = bBlocking ? BlockingHitMontages : HitMontages[(int32)HitDirection].Montages;
	if (MontageArray.Num() <= 0)
		return;

	const float MontageLength = Montage_Play(MontageArray[Index]);

	// Revert attacking status, we're not doing that if we got hit.
	bIsAttacking = false;
	bIsHeavyAttack = false;

	// Clear attack timer and flags so we can attack right away
	GetWorld()->GetTimerManager().ClearTimer(Attack_TimerHandle);
	bIsAttacking = false;
	bIsHeavyAttack = false;
}

bool USCAnimInstance::PlayDeathAnimation(const FVector DamageDirection)
{
	// Only supporting a couple of stances for right now
	if (CurrentStance != ESCCharacterStance::Standing && CurrentStance != ESCCharacterStance::Crouching)
		return false;

	// Grab a random aimation based on the player who wants one
	const bool bFromFront = (Player->GetActorForwardVector() | DamageDirection) < 0.f;
	const bool bIsCrouching = Player->IsCrouching();

	// Get dat montage
	UAnimMontage* DeathMontage = [&]() -> UAnimMontage*
	{
		if (bIsCrouching)
		{
			if (bFromFront)
				return FrontCrouchDeathMontage;
			return RearCrouchDeathMontage;
		}

		if (bFromFront)
			return FrontDeathMontage;
		return RearDeathMontage;
	}();

	// Nope.
	if (!IsValid(DeathMontage))
		return false;

	// We have a death animation, play it!
	return Montage_Play(DeathMontage) > 0.f;
}

void USCAnimInstance::PlayGrabAnim()
{
	if (!Montage_IsPlaying(GrabMontage))
		Montage_Play(GrabMontage);
}

void USCAnimInstance::StopGrabAnim()
{
	static const FName NAME_MissedSection(TEXT("GrabMissed"));
	if (GrabMontage->GetSectionIndex(NAME_MissedSection) != INDEX_NONE)
		Montage_JumpToSection(NAME_MissedSection, GrabMontage);
	else
		Montage_Stop(0.2f, GrabMontage);
}

bool USCAnimInstance::IsGrabAnimPlaying() const
{
	return Montage_IsPlaying(GrabMontage);
}

void USCAnimInstance::AllowRootMotion()
{
	TransitionMovementState(EMovementState::Idle);

	// Character movement properties only happen on the local client
	if (Player->IsLocallyControlled())
	{
		UCharacterMovementComponent* CharacterMovement = Player->GetCharacterMovement();
		CharacterMovement->BrakingDecelerationWalking = BrakingDecelerationWalking;
		CharacterMovement->MaxAcceleration = MaxAcceleration;
		if (!Player->InCombatStance())
		{
			CharacterMovement->bOrientRotationToMovement = true;
		}
	}
}

void USCAnimInstance::HandleFootstep(const FName Bone, const bool bJustLanded /*= false*/)
{
	if (Player == nullptr)
		return;

	UWorld* World = GetWorld();

	const FVector& Offset = FVector::UpVector * 25.f;
	const FVector& BoneLocation = Player->GetMesh()->GetSocketLocation(Bone) + Offset;
	const FVector& RayEnd = BoneLocation - (Offset * 2.f);

	static const FName FootstepTraceTag = FName(TEXT("FootstepTrace"));
	FCollisionQueryParams TraceParams(FootstepTraceTag, false, Player);
	TraceParams.bTraceAsyncScene = true;
	TraceParams.bReturnPhysicalMaterial = true;

	TArray<FHitResult> HitResults;
	const bool FoundBlocker = World->LineTraceMultiByChannel(HitResults, BoneLocation, RayEnd, ECollisionChannel::ECC_Visibility, TraceParams);

	if (FoundBlocker)
	{
		int32 HitIndex = 0;
		while (HitResults[HitIndex].bBlockingHit == false && HitIndex < HitResults.Num()) ++HitIndex;

		PendingFootstepSound = GetFootstepSound(UGameplayStatics::GetSurfaceType(HitResults[HitIndex]), bJustLanded);
		if (PendingFootstepSound.IsNull())
			PendingFootstepSound = GetFootstepSound(EPhysicalSurface::SurfaceType_Default, bJustLanded);
		PendingFootstepLocation = BoneLocation;
		
		// Notify nearby AI if loud enough
		const EMovementSpeed MovementSpeed = GetDesiredStartMoveState();
		switch (MovementSpeed)
		{
		case EMovementSpeed::Walk: Player->MakeStealthNoise(WalkFootstepLoudness, Player, PendingFootstepLocation); break;
		case EMovementSpeed::Run: Player->MakeStealthNoise(RunFootstepLoudness, Player, PendingFootstepLocation); break;
		case EMovementSpeed::Sprint: Player->MakeStealthNoise(SprintFootstepLoudness, Player, PendingFootstepLocation); break;
		}

		// Load/play the footstep sound
		if (PendingFootstepSound.Get())
		{
			DeferredPlayFootstepSound();
		}
		else if (!PendingFootstepSound.IsNull())
		{
			// Load and play when it's ready
			USCGameInstance* GameInstance = Cast<USCGameInstance>(World->GetGameInstance());
			GameInstance->StreamableManager.RequestAsyncLoad(PendingFootstepSound.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &ThisClass::DeferredPlayFootstepSound));
		}
	}
}

void USCAnimInstance::OnMontagePlay(UAnimMontage* Montage)
{
	// If a montage wants to move you, just let it happen
	if (bUseStartStopTransitions && Montage->HasRootMotion())
	{
		AllowRootMotion();
	}
}

void USCAnimInstance::PlayStunAnimation(UAnimMontage* Montage)
{
	bEndStun = false;
	StunMontage = Montage;
	Montage_Play(Montage);
}

void USCAnimInstance::EndStunAnimation()
{
	bEndStun = true;
	if (StunMontage)
	{
		//Montage_SetNextSection(Montage_GetCurrentSection(StunMontage), TEXT("EndStun"), StunMontage); // Using jump so stuns end as soon as you win the break free animation
		Montage_JumpToSection(TEXT("EndStun"), StunMontage);
	}
}

float USCAnimInstance::PlayDrown()
{
	if (DrownFromSwimmingMontage == nullptr)
		return 0.f;

	return Montage_Play(DrownFromSwimmingMontage);
}

void USCAnimInstance::DeferredPlayFootstepSound()
{
	if (PendingFootstepSound.Get())
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), PendingFootstepSound.Get(), PendingFootstepLocation);
	}
}

void USCAnimInstance::EnterCombatStance()
{
	TransitionMovementState(EMovementState::Idle);
	OutOfCombat = 0.f;
	bInCombatStance = true;

	// Character movement properties only happen on the local client
	if (Player->IsLocallyControlled())
	{
		UCharacterMovementComponent* CharacterMovement = Player->GetCharacterMovement();
		CharacterMovement->BrakingDecelerationWalking = BrakingDecelerationWalking;
		CharacterMovement->MaxAcceleration = MaxAcceleration;
		CharacterMovement->bOrientRotationToMovement = false;
	}
}

void USCAnimInstance::LeaveCombatStance()
{
	TransitionMovementState(Speed > 0.f ? EMovementState::Move : EMovementState::Idle);
	OutOfCombat = 1.f;
	bInCombatStance = false;

	// Character movement properties only happen on the local client
	if (Player->IsLocallyControlled())
	{
		UCharacterMovementComponent* CharacterMovement = Player->GetCharacterMovement();
		CharacterMovement->BrakingDecelerationWalking = BrakingDecelerationWalking;
		CharacterMovement->MaxAcceleration = MaxAcceleration;

		if (bUseStartStopTransitions == false)
			CharacterMovement->bOrientRotationToMovement = true;
	}
}

void USCAnimInstance::SetRagdollPhysics(FName BoneName /*  = NAME_None*/, bool Simulate /* = true*/, float BlendWeight /* = 1.f*/, bool SkipCustomPhys /* = false*/)
{
	if (Player)
	{
		Player->SetRagdollPhysics(BoneName, Simulate, BlendWeight, SkipCustomPhys);
	}
}

void USCAnimInstance::SetFullRagdoll(const bool bEnabled)
{
	if (!Player)
		return;

	Player->GetMesh()->SetSimulatePhysics(bEnabled);
	Player->GetMesh()->bBlendPhysics = bEnabled;
	Player->GetMesh()->bPauseAnims = bEnabled;

	if (bEnabled)
		Player->GetMesh()->WakeAllRigidBodies();

	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Player))
	{
		// If the head is still attached when we ragdoll, turn off physics
		if (Counselor->FaceLimb->GetAttachParent())
			Counselor->FaceLimb->SetCollisionEnabled(bEnabled ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	}
}
