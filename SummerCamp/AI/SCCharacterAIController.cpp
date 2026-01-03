// Copyright (C) 2015-2018 IllFonic, LLC.

#include "SummerCamp.h"
#include "SCCharacterAIController.h"

#include "AI/Navigation/NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeManager.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BrainComponent.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "NetworkingDistanceConstants.h"
#include "Perception/PawnSensingComponent.h"

#if WITH_EDITOR
# include "MessageLog.h"
# include "UObjectToken.h"
#endif

#include "SCBridgeButton.h"
#include "SCBridgeCore.h"
#include "SCBridgeNavUnit.h"
#include "SCCabinet.h"
#include "SCCBRadio.h"
#include "SCCharacterAIProperties.h"
#include "SCCharacterMovement.h"
#include "SCCounselorCharacter.h"
#include "SCCrowdFollowingComponent.h"
#include "SCDoor.h"
#include "SCDriveableVehicle.h"
#include "SCEscapePod.h"
#include "SCGameMode.h"
#include "SCDriveableVehicle.h"
#include "SCFuseBox.h"
#include "SCHidingSpot.h"
#include "SCItem.h"
#include "SCInteractableManagerComponent.h"
#include "SCInteractComponent.h"
#include "SCKillerCharacter.h"
#include "SCPhoneJunctionBox.h"
#include "SCPolicePhone.h"
#include "SCRadio.h"
#include "SCRepairComponent.h"
#include "SCWindow.h"

DEFINE_LOG_CATEGORY(LogSCCharacterAI);

namespace FSCCharacterBlackboard
{
	const FName NAME_AreBehaviorsBlocked(TEXT("AreBehaviorsBlocked"));
	const FName NAME_CurrentWaypoint(TEXT("CurrentWaypoint"));
	const FName NAME_DesiredInteractable(TEXT("DesiredInteractable"));
}

ASCCharacterAIController::ASCCharacterAIController(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer.SetDefaultSubobjectClass<USCCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	bWantsPlayerState = true;

	ThrottledTickDelay = 1.f/10.f; // Throttle updates to 10FPS

	NavigationInvokerRadius = 5000.f;
	NavigationInvokerTileRemovalRadiusScale = 1.f + (2.f/3.f);

	PawnSensingComponent = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("PawnSensingComponent"));
	PawnSensingComponent->SensingInterval = .4f; // Half ASCCharacter::NoiseEmitter->NoiseLifetime

	InteractionHoldTime = 0.f;
}

void ASCCharacterAIController::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	PawnSensingComponent->OnSeePawn.AddDynamic(this, &ThisClass::OnSensorSeePawn);
	PawnSensingComponent->OnHearNoise.AddDynamic(this, &ThisClass::OnSensorHearNoise);
}

void ASCCharacterAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Tick throttling
	if (Blackboard && CharacterPropertiesClass && GetPawn())
	{
		const float TimeSeconds = GetWorld()->GetTimeSeconds();
		if (TimeSeconds-LastThrottledTickTime >= ThrottledTickDelay)
		{
			ThrottledTick(TimeSeconds);
		}
	}
}

void ASCCharacterAIController::SetPawn(APawn* InPawn)
{
	if (GetPawn())
	{
		// Unregister our current pawn
		UNavigationSystem::UnregisterNavigationInvoker(*GetPawn());
	}

	Super::SetPawn(InPawn);

	if (GetPawn())
	{
		UNavigationSystem::RegisterNavigationInvoker(*GetPawn(), NavigationInvokerRadius, NavigationInvokerRadius*NavigationInvokerTileRemovalRadiusScale);

		// Vehicles should use base PathFollowing
		if (UCrowdFollowingComponent* CrowdFollowComp = Cast<UCrowdFollowingComponent>(GetPathFollowingComponent()))
		{
			CrowdFollowComp->SetCrowdSimulationState(GetPawn()->IsA<ASCDriveableVehicle>() ? ECrowdSimulationState::ObstacleOnly : ECrowdSimulationState::Enabled);
		}
	}

	if (ASCCharacter* MyCharacter = Cast<ASCCharacter>(GetPawn()))
	{
		SetBotProperties(MyCharacter->AIProperties);

		if (MyCharacter->BehaviorTree)
		{
			// Use the character's set BehaviorTree
			RunBehaviorTree(MyCharacter->BehaviorTree);
			UE_LOG(LogSCCharacterAI, Verbose, TEXT("%s is using the BehaviorTree (%s) specified in it's character class"), *MyCharacter->GetName(), *MyCharacter->BehaviorTree->GetName());
		}
	}
	else if (ASCDriveableVehicle* MyVehicle = Cast<ASCDriveableVehicle>(GetPawn()))
	{
		// Grab bot properties off of the vehicle's driver
		if (ASCCharacter* Driver = Cast<ASCCharacter>(MyVehicle->Driver))
		{
			SetBotProperties(Driver->AIProperties);
		}
	}
}

bool ASCCharacterAIController::InitializeBlackboard(UBlackboardComponent& BlackboardComp, UBlackboardData& BlackboardAsset)
{
	if (Super::InitializeBlackboard(BlackboardComp, BlackboardAsset))
	{
		AreBehaviorsBlockedKey					= BlackboardAsset.GetKeyID(FSCCharacterBlackboard::NAME_AreBehaviorsBlocked);
		CurrentWaypointKey						= BlackboardAsset.GetKeyID(FSCCharacterBlackboard::NAME_CurrentWaypoint);
		DesiredInteractableKey					= BlackboardAsset.GetKeyID(FSCCharacterBlackboard::NAME_DesiredInteractable);

		return true;
	}

	return false;
}

bool ASCCharacterAIController::ShouldPostponePathUpdates() const
{
	if (UPathFollowingComponent* PathFollowingComp = GetPathFollowingComponent())
	{
		// Don't postpone a path update if we are crossing a navlink and our path gets invalidated, recalculate now!
		if (!PathFollowingComp->HasValidPath() && PathFollowingComp->HasStartedNavLinkMove())
			return false;
	}

	return Super::ShouldPostponePathUpdates();
}

bool ASCCharacterAIController::LineOfSightTo(const AActor* Other, FVector ViewPoint/* = FVector(ForceInit)*/, bool bAlternateChecks/* = false*/) const
{
	if (Other == nullptr)
	{
		return false;
	}

	if (ViewPoint.IsZero())
	{
		FRotator ViewRotation;
		GetActorEyesViewPoint(ViewPoint, ViewRotation);

		// if we still don't have a view point we simply fail
		if (ViewPoint.IsZero())
		{
			return false;
		}
	}

	static FName NAME_LineOfSight(TEXT("LineOfSight"));
	FVector TargetLocation = Other->GetTargetLocation(GetPawn());
	UWorld* World = GetWorld();

	FCollisionQueryParams CollisionParams(NAME_LineOfSight, true, GetPawn());
	CollisionParams.AddIgnoredActor(Other);

	// Handle non-Pawn objects
	const APawn * OtherPawn = Cast<const APawn>(Other);
	if (!OtherPawn && !Cast<UCapsuleComponent>(Other->GetRootComponent()))
	{
		// No capsule component, do a trace to the actor target location
		bool bHit = World->LineTraceTestByChannel(ViewPoint, TargetLocation, ECC_Visibility, CollisionParams);
		if (!bHit)
		{
			return true;
		}

		return false;
	}

	// Check distance limits
	const FVector OtherActorLocation = Other->GetActorLocation();
	const float DistSq = (OtherActorLocation - ViewPoint).SizeSquared();
	if (DistSq > FARSIGHTTHRESHOLDSQUARED)
	{
		return false;
	}
	if (!OtherPawn && (DistSq > NEARSIGHTTHRESHOLDSQUARED))
	{
		return false;
	}

	if (DistSq <= NEARSIGHTTHRESHOLDSQUARED)
	{
		// Check along the height of their capsule, when enough traces pass they are considered visible
		float OtherRadius, OtherHeight;
		Other->GetSimpleCollisionCylinder(OtherRadius, OtherHeight);

		// We need to ensure a good amount of the character can be seen, so stare them down
		int32 MissCount = 0;
		if (!World->LineTraceTestByChannel(ViewPoint, OtherActorLocation + FVector(0.f, 0.f, OtherHeight*.95f), ECC_Visibility, CollisionParams)) ++MissCount;
		if (!World->LineTraceTestByChannel(ViewPoint, OtherActorLocation + FVector(0.f, 0.f, OtherHeight*.5f), ECC_Visibility, CollisionParams)) ++MissCount;
		if (MissCount >= 2)
			return true;

		if (!World->LineTraceTestByChannel(ViewPoint, OtherActorLocation, ECC_Visibility, CollisionParams)) ++MissCount;
		if (MissCount >= 2)
			return true;

		if (!World->LineTraceTestByChannel(ViewPoint, OtherActorLocation - FVector(0.f, 0.f, OtherHeight*.5f), ECC_Visibility, CollisionParams)) ++MissCount;
		if (MissCount >= 2)
			return true;

		if (!World->LineTraceTestByChannel(ViewPoint, OtherActorLocation - FVector(0.f, 0.f, OtherHeight*.95f), ECC_Visibility, CollisionParams)) ++MissCount;
		if (MissCount >= 2)
			return true;
	}
	else if (bAlternateChecks)
	{
		// Perform simpler testing at a long distance
		if (!World->LineTraceTestByChannel(ViewPoint, OtherActorLocation, ECC_Visibility, CollisionParams))
			return true;
	}

	return false;
}

void ASCCharacterAIController::GameHasEnded(class AActor* EndGameFocus /*= NULL*/, bool bIsWinner /*= false*/)
{
	if (UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(BrainComponent))
	{
		BTComp->StopTree();
	}

	Super::GameHasEnded(EndGameFocus, bIsWinner);
}

void ASCCharacterAIController::UpdateControlRotation(float DeltaTime, bool bUpdatePawn/* = true*/)
{
	// NOTE: Copy of AAIController::UpdateControlRotation with focal point smoothing and taking the movement direction as an override
	if (ASCCharacter* const MyCharacter = Cast<ASCCharacter>(GetPawn()))
	{
		const FRotator InitialControlRotation = GetControlRotation();
		FRotator NewControlRotation = InitialControlRotation;

		// Look toward focus
		const FVector FocalPoint = GetFocalPoint();
		if (FAISystem::IsValidLocation(FocalPoint))
		{
			NewControlRotation = (FocalPoint - MyCharacter->GetPawnViewLocation()).Rotation();

			// Smooth out focus changes
			if (DeltaTime > 0.f)
			{
				NewControlRotation = FMath::Lerp(InitialControlRotation, NewControlRotation, DeltaTime);
			}
		}
		else if (bSetControlRotationFromPawnOrientation)
		{
			NewControlRotation = MyCharacter->GetActorRotation();
		}

		// Take acceleration/velocity as the control rotation when present
		USCCharacterMovement* MovementComp = CastChecked<USCCharacterMovement>(MyCharacter->GetMovementComponent());
		FRotator DeltaRot;
		NewControlRotation = MovementComp->ComputeOrientToMovementRotation(NewControlRotation, DeltaTime, DeltaRot);

		// Don't pitch view unless looking at another pawn
		if (NewControlRotation.Pitch != 0 && Cast<APawn>(GetFocusActor()) == nullptr)
		{
			NewControlRotation.Pitch = 0.f;
		}

		// Smooth out all control rotation changes
		if (DeltaTime > 0.f)
		{
			NewControlRotation = FMath::Lerp(InitialControlRotation, NewControlRotation, DeltaTime*4.f);
		}

		if (InitialControlRotation.Equals(NewControlRotation, 1e-3f) == false)
		{
			SetControlRotation(NewControlRotation);

			if (bUpdatePawn)
			{
				MyCharacter->FaceRotation(NewControlRotation, DeltaTime);
			}
		}

		return;
	}
}

FVector ASCCharacterAIController::GetFocalPointOnActor(const AActor *Actor) const
{
	FVector FocalPoint = FNavigationSystem::InvalidLocation;
	if (Actor)
	{
		if (const APawn* ActorPawn = Cast<APawn>(Actor))
			FocalPoint = ActorPawn->GetPawnViewLocation();
		else
			FocalPoint = Actor->GetActorLocation();
	}

	return FocalPoint;
}

void ASCCharacterAIController::OnPawnDied()
{
	// Stop invoking navmesh generation
	UNavigationSystem::UnregisterNavigationInvoker(*GetPawn());

	// Stop our BehaviorTree
	if (UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(BrainComponent))
		BTComp->StopTree();

	// Clear our interactables
	ClearInteractables();
}

void ASCCharacterAIController::ThrottledTick(float TimeSeconds/* = 0.f */)
{
	ASCCharacter* MyCharacter = Cast<ASCCharacter>(GetPawn());
	ASCDriveableVehicle* Vehicle = Cast<ASCDriveableVehicle>(GetPawn());
	if (!MyCharacter && Vehicle)
	{
		MyCharacter = Cast<ASCCharacter>(Vehicle->Driver);
	}

	if (!MyCharacter || !Blackboard)
		return;

	ASCCounselorCharacter* MyCounselor = Cast<ASCCounselorCharacter>(MyCharacter);
	const USCCharacterAIProperties* const MyProperties = GetBotPropertiesInstance<USCCharacterAIProperties>();
	UWorld* World = GetWorld();
	if (TimeSeconds == 0.f)
		TimeSeconds = World->GetTimeSeconds();
	const float DeltaTime = TimeSeconds-LastThrottledTickTime;
	LastThrottledTickTime = TimeSeconds;
	ASCGameMode* GameMode = World->GetAuthGameMode<ASCGameMode>();

	// Make sure we aren't trying to interact with nothing...
	if (MyCharacter->IsHoldInteractButtonPressed())
	{
		InteractionHoldTime += DeltaTime;

		if (InteractionHoldTime >= 3.f && !MyCharacter->bAttemptingInteract && !MyCharacter->bHoldInteractStarted)
			MyCharacter->OnInteract1Released();
	}
	else
	{
		InteractionHoldTime = 0.f;
	}

	// Attempt to break out of a grab/stun/trap
	if (MyCharacter->bWiggleGameActive)
	{
		if (WiggleDelay == 0.f)
		{
			WiggleDelay = FMath::RandRange(MyProperties->MinWiggleInterval, MyProperties->MaxWiggleInterval);

			// Incorporate difficulty
			WiggleDelay *= GameMode->GetCurrentDifficultyDownwardAlpha(.25f);

			// Incorporate composure and strength
			if (MyCounselor)
			{
				WiggleDelay *= FMath::Lerp(1.25f, 1.f, MyCounselor->GetStatAlpha(ECounselorStats::Composure) + MyCounselor->GetStatAlpha(ECounselorStats::Strength));
			}
		}

		WiggleDelta += DeltaTime;
		bIsWiggling = (WiggleDelta >= WiggleDelay);

		if (bIsWiggling)
		{
			WiggleDelta = 0.f;
		}
	}
	else if (WiggleDelta > 0.f || bIsWiggling)
	{
		WiggleDelay = 0.f;
		WiggleDelta = 0.f;
		bIsWiggling = false;
	}

	// Update behavior blockage
	const bool bInteractionLocked = (MyCharacter->GetInteractable() && MyCharacter->GetInteractable()->IsLockedInUse());
	bBehaviorsBlocked = (!MyCharacter->IsAlive() || MyCharacter->bAttemptingInteract || MyCharacter->IsInSpecialMove() || bInteractionLocked || MyCharacter->bWiggleGameActive);
	if (!bBehaviorsBlocked && MyCounselor)
	{
		if (MyCounselor->bIsGrabbed || MyCounselor->GetCurrentHidingSpot() || MyCounselor->IsHoldInteractButtonPressed())
		{
			bBehaviorsBlocked = true;
		}
	}
	Blackboard->SetValue<UBlackboardKeyType_Bool>(AreBehaviorsBlockedKey, bBehaviorsBlocked);

	// Update interactions
	if (!bBehaviorsBlocked || bInteractionLocked)
	{
		UpdateInteractables(DeltaTime);
	}

	// Track time without movement
	if (!bBehaviorsBlocked && MyCharacter->GetVelocity().IsNearlyZero())
	{
		TimeWithoutVelocity += DeltaTime;
	}
	else
	{
		TimeWithoutVelocity = 0.f;
	}
}

void ASCCharacterAIController::OnMoveTaskFinished(const EBTNodeResult::Type TaskResult, const bool bAlreadyAtGoal)
{
	if (DesiredInteractable)
	{
		DesiredInteractable->PendingCharacterAIMoveFinished(this, TaskResult, bAlreadyAtGoal);
	}
}

bool ASCCharacterAIController::CanSeePawn(APawn* PawnToSee) const
{
	if (PawnSensingComponent && PawnToSee)
	{
		return PawnSensingComponent->CouldSeePawn(PawnToSee);
	}

	return false;
}

void ASCCharacterAIController::OnSensorSeePawn(APawn* SeenPawn)
{
}

void ASCCharacterAIController::OnSensorHearNoise(APawn* HeardPawn, const FVector& Location, float Volume)
{
}

void ASCCharacterAIController::PawnInteractAccepted(USCInteractComponent* Interactable)
{
	// Make sure we aren't holding the interact button
	ASCCharacter* MyCharacter = Cast<ASCCharacter>(GetPawn());
	if (MyCharacter && MyCharacter->IsHoldInteractButtonPressed() && MyCharacter->InteractTimerPassedMinHoldTime())
		MyCharacter->OnInteract1Released();
}

bool ASCCharacterAIController::SetDesiredInteractable(const AActor* ObjectToInteractWith, const FName BlackboardKey, const bool bBarricadeDoor /*= false*/)
{
	if (ObjectToInteractWith)
	{
		// Find the interact component on the object
		TArray<UActorComponent*> InteractComponents = ObjectToInteractWith->GetComponentsByClass(USCInteractComponent::StaticClass());
		for (UActorComponent* Component : InteractComponents)
		{
			if (USCInteractComponent* InteractComponent = Cast<USCInteractComponent>(Component))
			{
				// Make sure we can interact with it and set our desired interactable
				if (InteractComponent->AcceptsPendingCharacterAI(this))
				{
					SetDesiredInteractable(InteractComponent, bBarricadeDoor);

					// Set blackboard entry if specified
					if (!BlackboardKey.IsNone() && Blackboard)
					{
						FBlackboard::FKey KeyID = Blackboard->GetKeyID(BlackboardKey);
						if (KeyID != FBlackboard::InvalidKey)
						{
							// Make sure it's a vector type
							const auto KeyType = Blackboard->GetKeyType(KeyID);
							if (KeyType == UBlackboardKeyType_Vector::StaticClass())
							{
								FVector InteractLocation;
								InteractComponent->BuildAIInteractionLocation(this, InteractLocation);

								// Make sure it's a valid location
								if (FNavigationSystem::IsValidLocation(InteractLocation))
									Blackboard->SetValue<UBlackboardKeyType_Vector>(KeyID, InteractLocation);
							}
						}
					}

					return true;
				}
			}
		}
	}

	return false;
}

void ASCCharacterAIController::SetDesiredInteractable(USCInteractComponent* Interactable, const bool bBarricadeDoor /*= false*/, const bool bForce /*= false*/)
{
	if (DesiredInteractable == Interactable)
		return;

	UE_LOG(LogSCCharacterAI, Verbose, TEXT("%s::SetDesiredInteractable: %s (%s)"), *GetNameSafe(this), *GetNameSafe(Interactable), Interactable ? *GetNameSafe(Interactable->GetOwner()) : TEXT("None"));

	// Make sure we aren't holding the interact button
	ASCCharacter* MyCharacter = Cast<ASCCharacter>(GetPawn());
	if (MyCharacter && MyCharacter->IsHoldInteractButtonPressed() && MyCharacter->InteractTimerPassedMinHoldTime())
		MyCharacter->OnInteract1Released();

	// Cleanup existing desired interaction
	if (DesiredInteractable)
	{
		DesiredInteractable->RemovePendingCharacterAI(this);
	}

	// Take Interactable
	DesiredInteractable = Interactable;
	bBarricadeDesiredInteractable = bBarricadeDoor;
	if (DesiredInteractable)
	{
		if (!bForce)
			check(DesiredInteractable->AcceptsPendingCharacterAI(this));
		DesiredInteractable->SetPendingCharacterAI(this, bForce);
	}

	if (Blackboard)
		Blackboard->SetValue<UBlackboardKeyType_Object>(DesiredInteractableKey, DesiredInteractable ? DesiredInteractable->GetOwner() : nullptr); // Use the owner for a better description of the interactable

	// Update Blackboard
	ThrottledTick();
}

void ASCCharacterAIController::DesiredInteractableTaken()
{
	// Clear our interactable
	SetDesiredInteractable(nullptr);

	// Stop our current MoveTo so our BT reruns and finds a new objective
	StopMovement();
}

void ASCCharacterAIController::SimulateInteractButtonPress()
{
	if (ASCCharacter* MyCharacter = Cast<ASCCharacter>(GetPawn()))
	{
		MyCharacter->OnInteract1Pressed();
		MyCharacter->OnInteract1Released();
	}
}

void ASCCharacterAIController::UpdateInteractables(const float DeltaTime)
{
	ASCCharacter* MyCharacter = Cast<ASCCharacter>(GetPawn());
	if (MyCharacter)
	{
		if (USCInteractComponent* BestInteractable = MyCharacter->GetInteractable())
		{
			if (const AActor* InteractableOwner = BestInteractable->GetOwner())
			{
				// Door/Window interaction
				if (InteractableOwner->IsA<ASCDoor>() || InteractableOwner->IsA<ASCWindow>())
				{
					if (!BestInteractable->IsLockedInUse())
					{
						if (const ASCDoor* Door = Cast<ASCDoor>(InteractableOwner))
						{
							InteractWithDoor(BestInteractable, Door);
						}
						else if (const ASCWindow* Window = Cast<ASCWindow>(InteractableOwner))
						{
							InteractWithWindow(BestInteractable, Window);
						}
					}
				}
				// Hiding spot interaction
				else if (const ASCHidingSpot* HidingSpot = Cast<ASCHidingSpot>(InteractableOwner))
				{
					InteractWithHidingSpot(BestInteractable, HidingSpot);
				}
				// Cabinet interaction
				else if (const ASCCabinet* Cabinet = Cast<ASCCabinet>(InteractableOwner))
				{
					InteractWithCabinet(BestInteractable, Cabinet);
				}
				// Item interaction
				else if (const ASCItem* Item = Cast<ASCItem>(InteractableOwner))
				{
					if (MyCharacter->CanInteractAtAll())
						InteractWithItem(BestInteractable, Item);
				}
				// Repairable interaction
				else if (InteractableOwner->IsA<ASCDriveableVehicle>() || InteractableOwner->IsA<ASCPhoneJunctionBox>() || InteractableOwner->IsA<ASCFuseBox>()
					|| InteractableOwner->IsA<ASCBridgeCore>() || InteractableOwner->IsA<ASCBridgeNavUnit>())
				{
					if (MyCharacter->CanInteractAtAll())
						InteractWithRepairable(BestInteractable, InteractableOwner);
				}
				// Communication device
				else if (InteractableOwner->IsA<ASCPolicePhone>() || InteractableOwner->IsA<ASCCBRadio>())
				{
					if (MyCharacter->CanInteractAtAll())
						InteractWithCommunicationDevice(BestInteractable, InteractableOwner);
				}
				// Radio
				else if (InteractableOwner->IsA<ASCRadio>())
				{
					InteractWithRadio(BestInteractable, InteractableOwner);
				}
				// Escape Pod
				else if (InteractableOwner->IsA<ASCEscapePod>())
				{
					const ASCEscapePod* EscapePod = Cast<ASCEscapePod>(InteractableOwner);
					if (MyCharacter->CanInteractAtAll())
					{
						if (BestInteractable == EscapePod->RepairComponent)
							InteractWithRepairable(BestInteractable, InteractableOwner);
						else
							SimulateInteractButtonPress();
					}
				}
				// Grendel Bridge Buttons
				else if (InteractableOwner->IsA<ASCBridgeButton>())
				{
					if (MyCharacter->CanInteractAtAll())
						SimulateInteractButtonPress();
				}
			}
		}
	}
}

bool ASCCharacterAIController::IsMovingTowardInteractable(const USCInteractComponent* InteractableComp) const
{
	ASCCharacter* MyCharacter = Cast<ASCCharacter>(GetPawn());
	if (InteractableComp && MyCharacter)
	{
		// Make sure we are facing the object we want to interact with, and we are moving in that direction
		const float AcceptableDot = .6f;
		const FVector BotLocation = MyCharacter->GetActorLocation();
		const FVector BotVelocity = MyCharacter->GetVelocity();
		const FVector MovingDirection = MyCharacter->GetActorForwardVector();
		const FVector InteractionLocation = InteractableComp->GetComponentLocation();
		const FVector DirectionToInteraction = (InteractionLocation - BotLocation).GetSafeNormal2D();
		const float DotToInteraction = FVector::DotProduct(MovingDirection, DirectionToInteraction);
		if (DotToInteraction >= AcceptableDot)
			return true;
	}

	return false;
}

void ASCCharacterAIController::InteractWithDoor(USCInteractComponent* InteractableComp, const ASCDoor* Door)
{
	ASCCharacter* MyCharacter = Cast<ASCCharacter>(GetPawn());

	if (!MyCharacter || !Door)
		return;

	// Open the door
	if (Door->HasInteractComponent(InteractableComp) && !Door->IsOpen() && !Door->IsOpeningOrClosing() && !Door->IsLockedOrBarricaded() && IsMovingTowardInteractable(InteractableComp))
	{
		bWasOutsideWhenOpeningDoor = !MyCharacter->IsIndoors();
		SimulateInteractButtonPress();
	}
}

void ASCCharacterAIController::LookAtTarget(AActor* Actor)
{
	if (Actor)
		SetFocus(Actor);
	else
		ClearFocus(EAIFocusPriority::Gameplay);
}

void ASCCharacterAIController::LookAtLocation(const FVector Location)
{
	if (Location == FNavigationSystem::InvalidLocation)
		ClearFocus(EAIFocusPriority::Gameplay);
	else
		SetFocalPoint(Location);
}

void ASCCharacterAIController::ClearLookAt()
{
	ClearFocus(EAIFocusPriority::Gameplay);
}

void ASCCharacterAIController::IgnoreCollisionOnActor(AActor* OtherActor)
{
	if (APawn* MyPawn = GetPawn())
	{
		MyPawn->MoveIgnoreActorAdd(OtherActor);
	}
}

void ASCCharacterAIController::RemoveIgnoreCollisionOnActor(AActor* OtherActor)
{
	if (APawn* MyPawn = GetPawn())
	{
		MyPawn->MoveIgnoreActorRemove(OtherActor);
	}
}

void ASCCharacterAIController::SetBotProperties(TSubclassOf<USCCharacterAIProperties> PropertiesClass)
{
	CharacterPropertiesClass = PropertiesClass;

	// Verify we got a valid CharacterPropertiesClass
	if (!CharacterPropertiesClass)
	{
		if (GetPawn())
		{
#if WITH_EDITOR
			FMessageLog("PIE").Warning()
				->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("No character AI properties set for \"%s\""), *GetPawn()->GetName()))))
				->AddToken(FUObjectToken::Create(GetPawn()->GetClass()))
				->AddToken(FTextToken::Create(FText::FromString(TEXT(", using default values in code."))));
#endif
			UE_LOG(LogSCCharacterAI, Error, TEXT("ASCCharacterAIController::SetBotProperties: No character AI properties set for \"%s\", using default values in code."), *GetPawn()->GetName());
		}

		return;
	}

	// Update pawn sensing component with character properties
	const USCCharacterAIProperties* const MyProperties = GetBotPropertiesInstance<USCCharacterAIProperties>();
	if (PawnSensingComponent)
	{
		PawnSensingComponent->HearingThreshold = MyProperties->HearingThreshold;
		PawnSensingComponent->LOSHearingThreshold = MyProperties->LOSHearingThreshold;
		PawnSensingComponent->SightRadius = MyProperties->SightRadius;
		PawnSensingComponent->SetPeripheralVisionAngle(MyProperties->PeripheralVisionAngle);
	}
}
