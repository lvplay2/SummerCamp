// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCInteractComponent.h"

#include "Animation/AnimInstance.h"

#include "ILLSpecialMove_MoveToward.h"

#include "SCCounselorAIController.h"
#include "SCCounselorCharacter.h"
#include "SCInGameHUD.h"
#include "SCInteractableManagerComponent.h"
#include "SCInteractWidget.h"
#include "SCKillerCharacter.h"
#include "SCPlayerController.h"
#include "SCSpecialMoveComponent.h"
#include "SCSpecialMove_SoloMontage.h"
#include "SCSplineCamera.h"
#include "SCGameState.h"

USCInteractComponent::USCInteractComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, AllowableFailsBeforeKillerNotify(3)
, BlendInTime(0.f)
, BlendBackTime(0.25f)
, bForceNewCameraRotation(true)
, bAutoReturnCamera(true)
{
	bWantsInitializeComponent = true;

	bCheckForOcclusion = true;

	InteractionWidgetClass = StaticLoadClass(USCInteractWidget::StaticClass(), nullptr, TEXT("/Game/UI/Interact/SCInteractionWidget.SCInteractionWidget_C"));

	bRandomizePips = true;
	WidgetDestructionTimer = 0.0f;
}

void USCInteractComponent::InitializeComponent()
{
	Super::InitializeComponent();

	bShowMinigameProperties = (InteractMethods & (int32) EILLInteractMethod::Hold) == (int32) EILLInteractMethod::Hold;
}

void USCInteractComponent::UninitializeComponent()
{
	Super::UninitializeComponent();

	if (InteractWidgetInstance)
	{
		InteractWidgetInstance->CleanUpWidget();
		InteractWidgetInstance = nullptr;
		WidgetDestructionTimer = 0.f;
	}
}

void USCInteractComponent::BeginPlay()
{
	Super::BeginPlay();

	AttachedSplineCameras.Empty();
	CharacterNextAllowedPathTime.Empty();

	TArray<USceneComponent*> ChildComponents;
	GetChildrenComponents(true, ChildComponents);

	// Find all of the spline cameras
	for (USceneComponent* comp : ChildComponents)
	{
		if (USCSplineCamera* SplineCam = Cast<USCSplineCamera>(comp))
		{
			AttachedSplineCameras.Add(SplineCam);
		}
	}

	SetComponentTickEnabled(InteractWidgetInstance != nullptr);
}

#if WITH_EDITOR
void USCInteractComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	bShowMinigameProperties = (InteractMethods & (int32)EILLInteractMethod::Hold) == (int32)EILLInteractMethod::Hold;
}
#endif

void USCInteractComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (InteractWidgetInstance)
	{
		FName CurrentMatchState;
		if (ASCGameState* GS = Cast<ASCGameState>(GetWorld()->GetGameState()))
		{
			CurrentMatchState = GS->GetMatchState();
		}

		if (CurrentMatchState != MatchState::InProgress && (InteractWidgetInstance->bIsHighlighted || InteractWidgetInstance->bBestInteractable))
		{
			InteractWidgetInstance->HideWidget();

			InteractWidgetInstance->bIsHighlighted = false;
			InteractWidgetInstance->bBestInteractable = false;

			WidgetDestructionTimer = 10.0f;
		}

		InteractWidgetInstance->UpdatePosition();

		// Widget is hidden, lets see if we should destroy it
		if (WidgetDestructionTimer > 0.0f)
		{
			WidgetDestructionTimer -= DeltaTime;

			if (WidgetDestructionTimer <= 0.0f)
			{
				InteractWidgetInstance->CleanUpWidget();
				InteractWidgetInstance = nullptr;
				SetComponentTickEnabled(false);
			}
		}
	}
}

void USCInteractComponent::OnHoldStateChangedBroadcast(AActor* Interactor, EILLHoldInteractionState NewState)
{
	Super::OnHoldStateChangedBroadcast(Interactor, NewState);

	ASCCharacter* Character = Cast<ASCCharacter>(Interactor);
	if (!Character)
		return;

	switch (NewState)
	{
		case EILLHoldInteractionState::Interacting:
		{
			Character->PlayInteractAnim(this);

			TArray<USceneComponent*> ChildComponents;
			GetChildrenComponents(true, ChildComponents);
			USCSpecialMoveComponent* BestSpecialMove = nullptr;
			if (ChildComponents.Num() > 0)
			{
				float BestDistanceSq = FLT_MAX;
				for (USceneComponent* Comp : ChildComponents)
				{
					if (USCSpecialMoveComponent* SpecialMove = Cast<USCSpecialMoveComponent>(Comp))
					{
						float DistanceSq = (Character->GetActorLocation() - SpecialMove->GetComponentLocation()).SizeSquared();
						if (DistanceSq < BestDistanceSq)
						{
							BestDistanceSq = DistanceSq;
							BestSpecialMove = SpecialMove;
						}
					}
				}

				// If we have a dummy special move hooked up, move to it
				if (BestSpecialMove)
				{
					BestSpecialMove->SpecialAborted.AddUniqueDynamic(this, &USCInteractComponent::OnHoldSpecialMoveFailed);
					BestSpecialMove->ActivateSpecial(Character);
					ActivatedSpecialMove = Character->GetActiveSCSpecialMove();
				}
			}
		}
		break;

		case EILLHoldInteractionState::Canceled:
		{
			Character->CancelInteractAnim();

			// If we activated a special move, and didn't give it a timeout, nuke it
			if (IsValid(ActivatedSpecialMove) && ActivatedSpecialMove == Character->GetActiveSCSpecialMove())
			{
				if (ActivatedSpecialMove->GetTimeRemaining() < 0.f)
					ActivatedSpecialMove->CancelSpecialMove();

				ActivatedSpecialMove = nullptr;
			}

			if (!OnHoldStateChanged.IsBound())
			{
				Character->GetInteractableManagerComponent()->UnlockInteraction(this);
			}
		}
		break;

		case EILLHoldInteractionState::Success:
		{
			Character->FinishInteractAnim();
			if (!OnHoldStateChanged.IsBound())
			{
				Character->GetInteractableManagerComponent()->UnlockInteraction(this);
			}
		}
		break;
	}
}

bool USCInteractComponent::IsInteractionAllowed(AILLCharacter* Character)
{
	if (bAlwaysAllowInteractionFromAI && Character && Character->PlayerState && Character->PlayerState->bIsABot)
		return true;

	return Super::IsInteractionAllowed(Character);
}

void USCInteractComponent::OnInRangeBroadcast(AActor* Interactor)
{
	FName CurrentMatchState;
	if (ASCGameState* GS = Cast<ASCGameState>(GetWorld()->GetGameState()))
	{
		CurrentMatchState = GS->GetMatchState();
	}

	if (bHoldInteracting || CurrentMatchState != MatchState::InProgress)
		return;

	Super::OnInRangeBroadcast(Interactor);

	// Spawn our interactable widget
	AutoCreateInteractWidget(Interactor);
}

void USCInteractComponent::StartHighlightBroadcast(AActor* Interactor)
{
	FName CurrentMatchState;
	if (ASCGameState* GS = Cast<ASCGameState>(GetWorld()->GetGameState()))
	{
		CurrentMatchState = GS->GetMatchState();
	}

	if (bHoldInteracting || CurrentMatchState != MatchState::InProgress)
		return;

	Super::StartHighlightBroadcast(Interactor);

	AutoCreateInteractWidget(Interactor);

	if (InteractWidgetInstance && CanAffectInteractWidget(Interactor))
	{
		InteractWidgetInstance->bIsHighlighted = true;
		InteractWidgetInstance->ShowWidget();
	}
}

void USCInteractComponent::StopHighlightBroadcast(AActor* Interactor)
{
	FName CurrentMatchState;
	if (ASCGameState* GS = Cast<ASCGameState>(GetWorld()->GetGameState()))
	{
		CurrentMatchState = GS->GetMatchState();
	}

	if (bHoldInteracting || CurrentMatchState != MatchState::InProgress)
		return;

	Super::StopHighlightBroadcast(Interactor);
	
	// Destroy our interactable widget
	if (InteractWidgetInstance && CanAffectInteractWidget(Interactor))
	{
		if (InteractWidgetInstance->bBestInteractable)
			InteractWidgetInstance->OnNotInteractableOrHighlighted();
		else
			InteractWidgetInstance->HideWidget();

		InteractWidgetInstance->bIsHighlighted = false;
		InteractWidgetInstance->bBestInteractable = false;

		WidgetDestructionTimer = 10.0f;
	}
}

void USCInteractComponent::BroadcastBecameBest(AActor* Interactor)
{
	FName CurrentMatchState;
	if (ASCGameState* GS = Cast<ASCGameState>(GetWorld()->GetGameState()))
	{
		CurrentMatchState = GS->GetMatchState();
	}

	if (bHoldInteracting || CurrentMatchState != MatchState::InProgress)
		return;

	Super::BroadcastBecameBest(Interactor);
	
	AutoCreateInteractWidget(Interactor);

	if (InteractWidgetInstance && CanAffectInteractWidget(Interactor))
	{
		InteractWidgetInstance->bBestInteractable = true;
		InteractWidgetInstance->OnIsInteractable();
	}
}

void USCInteractComponent::BroadcastLostBest(AActor* Interactor)
{
	FName CurrentMatchState;
	if (ASCGameState* GS = Cast<ASCGameState>(GetWorld()->GetGameState()))
	{
		CurrentMatchState = GS->GetMatchState();
	}

	if (bHoldInteracting || CurrentMatchState != MatchState::InProgress)
		return;

	Super::BroadcastLostBest(Interactor);

	if (InteractWidgetInstance && CanAffectInteractWidget(Interactor))
	{
		InteractWidgetInstance->bBestInteractable = false;
		if (InteractWidgetInstance->bIsHighlighted)
			InteractWidgetInstance->OnNotInteractable();
		else
			InteractWidgetInstance->OnNotInteractableOrHighlighted();
	}
}

void USCInteractComponent::OnHoldSpecialMoveFailed(ASCCharacter* Interactor)
{
	if (Interactor)
	{
		Interactor->CLIENT_CancelRepairInteract();
	}
}

float USCInteractComponent::GetHoldTimeLimit(const ASCCharacter* Interactor) const
{
	if ((InteractMethods & (int32)EILLInteractMethod::Hold) && Interactor->IsA<ASCCounselorCharacter>())
	{
		const ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Interactor);
		if (Counselor->IsAbilityActive(EAbilityType::Repair))
		{
			return FMath::Clamp(HoldTimeLimit - Counselor->GetAbilityModifier(EAbilityType::Repair), 1.f, HoldTimeLimit);
		}
	}

	return HoldTimeLimit;
}

void USCInteractComponent::SetDrawAtLocation(const bool bDrawAtLocation, const FVector2D ScreenLocationRatio /* = FVector2D::UnitVector*/)
{
	if (InteractWidgetInstance)
	{
		InteractWidgetInstance->ScreenLocationRatio = ScreenLocationRatio;
		InteractWidgetInstance->bDrawInExactScreenSpace = bDrawAtLocation;
	}
}

bool USCInteractComponent::CanAffectInteractWidget(AActor* Interactor) const
{
	ASCCharacter* CharacterInteractor = Cast<ASCCharacter>(Interactor);
	return (CharacterInteractor && CharacterInteractor->Controller && CharacterInteractor->Controller->IsLocalPlayerController());
}

void USCInteractComponent::AutoCreateInteractWidget(AActor* Interactor)
{
	if (!InteractWidgetInstance && CanAffectInteractWidget(Interactor))
	{
		ASCCharacter* Character = Cast<ASCCharacter>(Interactor);
		APlayerController* Controller = Cast<APlayerController>(Character ? Character->GetCharacterController() : nullptr);
		if (InteractionWidgetClass && Controller)
		{
			InteractWidgetInstance = NewObject<USCInteractWidget>(this, InteractionWidgetClass);
			InteractWidgetInstance->LinkedInteractable = this;
			InteractWidgetInstance->InteractionIconWidgetClass = InteractionIconWidgetClass;
			InteractWidgetInstance->SetPlayerContext(Controller);
			InteractWidgetInstance->Initialize();
			InteractWidgetInstance->AddToViewport(-1);
			SetComponentTickEnabled(true);
		}
	}
	// reset destruction timer
	WidgetDestructionTimer = 0.0f;
}

void USCInteractComponent::PlayInteractionCamera(ASCCharacter* Interactor)
{
	if (AttachedSplineCameras.Num() <= 0)
		return;

	// Cameras sorted by best match to worst <dot product, camera>
	TMap<float, USCSplineCamera*> SortedSplineCameras;
	FRotator ControlRotation = Interactor->GetControlRotation();
	for (USCSplineCamera* Cam : AttachedSplineCameras)
	{
		float dot = ControlRotation.Vector().GetSafeNormal() | Cam->GetFirstFrameRotation().Vector().GetSafeNormal();
		SortedSplineCameras.Add(dot, Cam);
	}

	// Sort by dot product descending
	SortedSplineCameras.KeySort([](float A, float B) { return B < A; } );

	// Store off the absolute best angle just in case no cameras return valid.
	PlayingCamera = nullptr;

	// Test validity of best case first
	for (auto& Elem : SortedSplineCameras)
	{
		if (!PlayingCamera)
			PlayingCamera = Elem.Value;

		if (Elem.Value->IsCameraValid(nullptr))
		{
			PlayingCamera = Elem.Value;
			break;
		}
	}

	// No valid cameras found just send the best rotation match possible
	if (PlayingCamera)
	{
		MakeOnlyActiveCamera(this->GetOwner(), PlayingCamera);
		PlayingCamera->ActivateCamera();
		Interactor->TakeCameraControl(this->GetOwner(), BlendInTime, VTBlend_EaseInOut, 2.f, bForceNewCameraRotation);
	}

	if (Pips.Num() > 0 && Interactor->IsLocallyControlled())
	{
		bHoldInteracting = true;
		if (InteractWidgetInstance)
		{
			InteractWidgetInstance->HideWidget();
			WidgetDestructionTimer = 10.0f;
		}
	}
}

void USCInteractComponent::StopInteractionCamera(ASCCharacter* Interactor)
{
	if (!Interactor)
		return;

	Interactor->ReturnCameraToCharacter(BlendBackTime, VTBlend_EaseInOut, 2.0f, true, false);
	
	// Stop camera anim.
	if (PlayingCamera)
	{
		PlayingCamera->DeactivateCamera();
		PlayingCamera = nullptr;
	}

	if (Pips.Num() > 0 && Interactor->IsLocallyControlled())
	{
		bHoldInteracting = false;
		BroadcastBecameBest(Interactor);
	}
}

bool USCInteractComponent::TryAutoReturnCamera(ASCCharacter* Interactor)
{
	if (bAutoReturnCamera)
		StopInteractionCamera(Interactor);

	return bAutoReturnCamera;
}

bool USCInteractComponent::AcceptsPendingCharacterAI(const ASCCharacterAIController* CharacterAI, const bool bCheckIfCloser /*= false*/) const
{
	const float TimeSeconds = GetWorld()->GetTimeSeconds();

	// Cool down for pathing failures
	if (CharacterNextAllowedPathTime.Contains(CharacterAI) && TimeSeconds-CharacterNextAllowedPathTime[CharacterAI] < 0.f)
		return false;

	if (PendingCharacterAI)
	{
		// Check to see if the requesting character is closer
		if (bCheckIfCloser && PendingCharacterAI != CharacterAI)
		{
			const AActor* OwningActor = GetOwner();
			const ASCCharacter* PendingActor = Cast<ASCCharacter>(PendingCharacterAI->GetPawn());
			const ASCCharacter* CharacterActor = Cast<ASCCharacter>(CharacterAI->GetPawn());

			if (OwningActor && PendingActor && CharacterActor)
			{
				const float DistanceToPendingCharacter = OwningActor->GetSquaredDistanceTo(PendingActor);
				const float DistanceToNewCharacter = OwningActor->GetSquaredDistanceTo(CharacterActor);

				return DistanceToNewCharacter < DistanceToPendingCharacter;
			}
		}

		// Only allow them when it's the same as our already pending character
		return (PendingCharacterAI == CharacterAI);
	}

	// Prevent AI from getting fixated
	if (CharacterAI == LastCharacterAI && FixatedNextAllowedTime != 0.f && TimeSeconds-FixatedNextAllowedTime < 0.f)
		return false;

	return true;
}

bool USCInteractComponent::BuildAIInteractionLocation(ASCCharacterAIController* CharacterAI, FVector& OutLocation)
{
	// Basic interaction location
	FVector NavigationLocation = GetComponentLocation();
	if (bUseYawLimit)
	{
		NavigationLocation += GetComponentRotation().Vector() * DistanceLimit * .75f;
	}

	// Location within DistanceLimit
	UWorld* World = GetWorld();
	UNavigationSystem* NavSystem = UNavigationSystem::GetCurrent(World);
	FNavLocation ProjectedLocation;
	if (NavSystem->GetRandomReachablePointInRadius(GetComponentLocation(), DistanceLimit, ProjectedLocation))
	{
		NavigationLocation = ProjectedLocation.Location;
	}
	else if (NavSystem->GetRandomPointInNavigableRadius(GetComponentLocation(), DistanceLimit, ProjectedLocation))
	{
		NavigationLocation = ProjectedLocation.Location;
	}

	// Make sure it projects
	const FNavAgentProperties& AgentProps = CharacterAI->GetNavAgentPropertiesRef();
	if (NavSystem->ProjectPointToNavigation(NavigationLocation, ProjectedLocation, INVALID_NAVEXTENT, &AgentProps))
	{
		OutLocation = ProjectedLocation.Location;
		return true;
	}

	return false;
}

void USCInteractComponent::PendingCharacterAIMoveFinished(ASCCharacterAIController* CharacterAI, const EBTNodeResult::Type TaskResult, const bool bAlreadyAtGoal)
{
	check(PendingCharacterAI == CharacterAI);

	UWorld* World = GetWorld();
	if (!World)
		return;
	const float TimeSeconds = World->GetTimeSeconds();
	ASCCharacter* CharacterPawn = CharacterAI ? Cast<ASCCharacter>(CharacterAI->GetPawn()) : nullptr;
	if (!CharacterPawn)
		return;

	auto HandlePathingFailure = [&](const float TimePenalty, const TCHAR* FailureReason) -> void
	{
		// Flag as bad for a while
		UE_LOG(LogSCCharacterAI, Warning, TEXT("%s %s %s (%s), flagging as bad for %3.2f seconds!"), *GetNameSafe(CharacterAI), FailureReason, *GetNameSafe(this), *GetNameSafe(GetOwner()), TimePenalty);
		const float NextPathTime = TimeSeconds + TimePenalty;
		if (CharacterNextAllowedPathTime.Contains(CharacterAI))
			CharacterNextAllowedPathTime[CharacterAI] = NextPathTime;
		else
			CharacterNextAllowedPathTime.Add(CharacterAI, NextPathTime);

#if !UE_BUILD_SHIPPING
		DrawAIDebugLine(CharacterPawn->GetActorLocation(), TimePenalty);
#endif
	};

	if (TaskResult == EBTNodeResult::Succeeded)
	{
		if (bAlreadyAtGoal && CharacterPawn->GetInteractable() != this)
		{
			if (TimeSeconds-LastForcedAISnapTime >= 2.f) // FIXME: pjackson: Hard-coded
			{
				// We must be REALLY close, snap the rotate towards it
				const FRotator RotationToThis = (GetComponentLocation() - CharacterPawn->GetActorLocation()).GetSafeNormal().Rotation();
				CharacterPawn->Controller->SetControlRotation(RotationToThis);

				// Poll for interactions to start interacting immediately if possible
				CharacterPawn->GetInteractableManagerComponent()->PollInteractions();
			}
			else
			{
				// Tried to snap to rotate too many times, forget about this a while
				HandlePathingFailure(5.f, TEXT("failed to rotate to")); // FIXME: pjackson: Hard-coded TimePenalty
			}

			LastForcedAISnapTime = TimeSeconds;
		}
	}
	else if (TaskResult == EBTNodeResult::Failed)
	{
		// Only accept failures while not performing a special move or interaction, which may have failed this move request
		if (!CharacterPawn->IsInSpecialMove() && !CharacterPawn->bAttemptingInteract)
		{
			// Only accept multiple failures within a short time period, to avoid false-positives (allow the AI to hammer it twice)
			if (TimeSeconds-LastFailedPathTime < 2.f) // FIXME: pjackson: Hard-coded
			{
				HandlePathingFailure(5.f, TEXT("failed to path to")); // FIXME: pjackson: Hard-coded TimePenalty
			}
			LastFailedPathTime = TimeSeconds;
		}
	}
}

void USCInteractComponent::RemovePendingCharacterAI(ASCCharacterAIController* CharacterAI)
{
	if (ensureAlways(PendingCharacterAI == CharacterAI))
	{
		PendingCharacterAI = nullptr;

		// Prevent AI from getting fixated
		if (LastCharacterAI == CharacterAI)
		{
			if (++CharacterAIAttempts > 3)
			{
				const float TimePenalty = 15.f;
				UE_LOG(LogSCCharacterAI, Warning, TEXT("%s fixated on %s (%s), flagging as bad for %3.2f seconds!"), *GetNameSafe(CharacterAI), *GetNameSafe(this), *GetNameSafe(GetOwner()), TimePenalty);
				FixatedNextAllowedTime = GetWorld()->GetTimeSeconds() + TimePenalty;

#if !UE_BUILD_SHIPPING
				if (ASCCharacter* CharacterPawn = CharacterAI ? Cast<ASCCharacter>(CharacterAI->GetPawn()) : nullptr)
				{
					DrawAIDebugLine(CharacterPawn->GetActorLocation(), TimePenalty);
				}
#endif
			}
		}
		else
		{
			CharacterAIAttempts = 0;
		}
		LastCharacterAI = CharacterAI;
	}
}

void USCInteractComponent::SetPendingCharacterAI(ASCCharacterAIController* CharacterAI, const bool bForce /*= false*/)
{
	if (PendingCharacterAI != CharacterAI)
	{
		if (bForce && PendingCharacterAI)
		{
			UE_LOG(LogSCCharacterAI, Verbose, TEXT("%s is taking %s from %s."), *GetNameSafe(CharacterAI), *GetNameSafe(this), *GetNameSafe(PendingCharacterAI));
			PendingCharacterAI->DesiredInteractableTaken();
		}

		if (ensureAlways(!PendingCharacterAI))
		{
			PendingCharacterAI = CharacterAI;
		}
	}
}

#if !UE_BUILD_SHIPPING
void USCInteractComponent::DrawAIDebugLine(const FVector& StartLocation, const float Lifetime)
{
	UWorld* World = GetWorld();
	const FVector InteractionLocation = GetComponentLocation();
	const float Thickness = 2.f; // T H I C C
	DrawDebugBox(World, StartLocation, FVector(10.f), FQuat::Identity, FColor::Yellow, true, Lifetime, 0, Thickness);
	DrawDebugLine(World, StartLocation, InteractionLocation, FColor::Yellow, true, Lifetime, 0, Thickness);
	DrawDebugBox(World, InteractionLocation, FVector(10.f), FQuat::Identity, FColor::Red, true, Lifetime, 0, Thickness);
	DrawDebugBox(World, InteractionLocation, FVector(DistanceLimit), FQuat::Identity, FColor::Red, true, Lifetime, 0, Thickness);
}
#endif
