// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCWindow.h"

#include "NavAreas/SCNavArea_BrokenWindow.h"
#include "NavAreas/SCNavArea_HighWindow.h"
#include "NavAreas/SCNavArea_Window.h"
#include "SCCounselorAnimInstance.h"
#include "SCContextKillComponent.h"
#include "SCCounselorCharacter.h"
#include "SCGameState_Hunt.h"
#include "SCInteractableManagerComponent.h"
#include "SCInteractComponent.h"
#include "SCKillerCharacter.h"
#include "SCPerkData.h"
#include "SCPlayerState.h"
#include "SCSpacerCapsuleComponent.h"
#include "SCSpecialMove_SoloMontage.h"
#include "SCSpecialMoveComponent.h"
#include "SCWeapon.h"

#include "AI/Navigation/NavModifierComponent.h"
#include "Animation/AnimMontage.h"

/*
				WARNING: THIS CLASS USES NET DORMANCY
	This means that if you need to update any replicated variables,
	logic, RPCs or other networking whatnot, you NEED to update the
	logic handling for dormancy.
*/

#define LOCTEXT_NAMESPACE "ILLFonic.F13.SCWindow"

namespace SCWindowNames
{
	static const FName NAME_HeightTrace(TEXT("FallDamageTrace"));
	static const FName NAME_FallingLoop(TEXT("FallLoop"));
	static const FName NAME_ExitFalling(TEXT("Exit"));
	static const FName NAME_VaultHigh(TEXT("EnterHigh"));
	static const FName NAME_VaultNormal(TEXT("EnterNormal"));
	static const FName NAME_DiveEnter(TEXT("Enter"));
}

ASCWindow::ASCWindow(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, bWoundCounselorForDiving(true)
, bTakeVaultDamage(true)
, WoundHeight(150.f)
, FallHeight(50.f)
, DiveDamage(20.f)
, VaultDamage(15.f)
, HeightDamage(50.f)
, CameraReturnLerpTime(0.5f)
, FallingMinTime(0.3f)
, bBrokenByDive(false)
{
	// Undo settings changed by ANavLinkProxy
	SetActorEnableCollision(true);
	bCanBeDamaged = true;
	bHidden = false;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;
	bAlwaysRelevant = false;
	NetUpdateFrequency = 30.0f;
	//NetCullDistanceSquared = FMath::Square(1000.f); // Comment in to test relevance locally

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	FrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Frame Mesh"));
	FrameMesh->SetupAttachment(RootComponent);
	FrameMesh->bGenerateOverlapEvents = false;

	MoveablePaneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Moveable Pane"));
	MoveablePaneMesh->SetupAttachment(FrameMesh);
	MoveablePaneMesh->bGenerateOverlapEvents = false;

	LeftHandIKLocation = CreateDefaultSubobject<USceneComponent>(TEXT("Left Hand IK Location"));
	LeftHandIKLocation->SetupAttachment(MoveablePaneMesh);

	RightHandIKLocation = CreateDefaultSubobject<USceneComponent>(TEXT("Right Hand IK Location"));
	RightHandIKLocation->SetupAttachment(MoveablePaneMesh);

	InteractComponent = CreateDefaultSubobject<USCInteractComponent>(TEXT("Indoor Interactable"));
	InteractComponent->InteractMethods |= (int32)EILLInteractMethod::Hold;
	InteractComponent->HoldTimeLimit = 0.5f;
	InteractComponent->SetupAttachment(RootComponent);

	ContextKillComponent = CreateDefaultSubobject<USCContextKillComponent>(TEXT("ContextKillComponent"));
	ContextKillComponent->SetupAttachment(RootComponent);

	OuterContextKillComponent = CreateDefaultSubobject<USCContextKillComponent>(TEXT("OuterContextKillComponent"));
	OuterContextKillComponent->SetupAttachment(RootComponent);

	MovementDriver = CreateDefaultSubobject<USCAnimDriverComponent>(TEXT("MovementDriver"));
	MovementDriver->SetNotifyName(TEXT("WindowMoveDriver"));

	IKDriver = CreateDefaultSubobject<USCAnimDriverComponent>(TEXT("IKDriver"));
	IKDriver->SetNotifyName(TEXT("WindowIKDriver"));

	BreakDriver = CreateDefaultSubobject<USCAnimDriverComponent>(TEXT("BreakDriver"));
	BreakDriver->SetNotifyName(TEXT("WindowBreakDriver"));

	IndoorCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Indoor Camera"));
	IndoorCamera->SetupAttachment(RootComponent);

	OutdoorCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Outdoor Camera"));
	OutdoorCamera->SetupAttachment(RootComponent);

	FallDriver = CreateDefaultSubobject<USCAnimDriverComponent>(TEXT("Fall Driver"));
	FallDriver->SetNotifyName(TEXT("FallDriver"));

	InsideBlockerCapsule = CreateDefaultSubobject<USCSpacerCapsuleComponent>(TEXT("IndoorBlockerCapsule"));
	InsideBlockerCapsule->SetupAttachment(RootComponent);

	OutsideBlockerCapsule= CreateDefaultSubobject<USCSpacerCapsuleComponent>(TEXT("OutdoorBlockerCapsule"));
	OutsideBlockerCapsule->SetupAttachment(RootComponent);

	NavigationModifier = CreateDefaultSubobject<UNavModifierComponent>(TEXT("NavigationModifier"));

	DestroyWindowInside_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("DestroyWindowInside_SpecialMoveHandler"));
	DestroyWindowInside_SpecialMoveHandler->SetupAttachment(RootComponent);
	DestroyWindowInside_SpecialMoveHandler->bTakeCameraControl = false;
	
	DestroyWindowOutside_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("DestroyWindowOutside_SpecialMoveHandler"));
	DestroyWindowOutside_SpecialMoveHandler->SetupAttachment(RootComponent);
	DestroyWindowOutside_SpecialMoveHandler->bTakeCameraControl = false;

	DestructionDriver = CreateDefaultSubobject<USCAnimDriverComponent>(TEXT("DestructionDriver"));
	DestructionDriver->SetNotifyName(TEXT("DestroyWindow"));

	// Create default counselor Special move components
	CounselorOpenFromInsideComponent = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("CounselorOpenInside"));
	CounselorOpenFromInsideComponent->SetupAttachment(RootComponent);

	CounselorOpenFromOutsideComponent = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("CounselorOpenOutside"));
	CounselorOpenFromOutsideComponent->SetupAttachment(RootComponent);

	CounselorCloseFromInsideComponent = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("CounselorCloseInside"));
	CounselorCloseFromInsideComponent->SetupAttachment(RootComponent);

	CounselorCloseFromOutsideComponent = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("CounselorCloseOutside"));
	CounselorCloseFromOutsideComponent->SetupAttachment(RootComponent);

	CounselorVaultFromInsideComponent = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("CounselorVaultInside"));
	CounselorVaultFromInsideComponent->SetupAttachment(RootComponent);

	CounselorVaultFromOutsideComponent = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("CounselorVaultOutside"));
	CounselorVaultFromOutsideComponent->SetupAttachment(RootComponent);

	CounselorDiveFromInsideComponent = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("CounselorDiveInside"));
	CounselorDiveFromInsideComponent->SetupAttachment(RootComponent);

	CounselorDiveFromOutsideComponent = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("CounselorDiveOutside"));
	CounselorDiveFromOutsideComponent->SetupAttachment(RootComponent);
}

void ASCWindow::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCWindow, bIsOpen);
	DOREPLIFETIME(ASCWindow, bIsDestroyed);
}

void ASCWindow::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (InteractComponent)
	{
		InteractComponent->OnInteract.AddDynamic(this, &ASCWindow::OnInteract);
		InteractComponent->OnCanInteractWith.BindDynamic(this, &ASCWindow::CanInteractWith);
		InteractComponent->OnHoldStateChanged.AddDynamic(this, &ASCWindow::OnHoldStateChanged);
	}

	if (MovementDriver)
	{
		FAnimNotifyEventDelegate BeginDelegate;
		FAnimNotifyEventDelegate TickDelegate;
		BeginDelegate.BindDynamic(this, &ASCWindow::DeactivateBlockers);
		TickDelegate.BindDynamic(this, &ASCWindow::AnimateWindow);
		MovementDriver->InitializeBeginTick(BeginDelegate, TickDelegate);
	}

	if (IKDriver)
	{
		FAnimNotifyEventDelegate TickDelegate;
		TickDelegate.BindDynamic(this, &ASCWindow::UpdateWindowIK);
		IKDriver->InitializeBeginTick(TickDelegate, TickDelegate);
	}

	if (BreakDriver)
	{
		FAnimNotifyEventDelegate BeginDelegate;
		BeginDelegate.BindDynamic(this, &ASCWindow::BreakWindow);
		BreakDriver->InitializeBegin(BeginDelegate);
	}

	if (FallDriver)
	{
		FAnimNotifyEventDelegate BeginDelegate;
		BeginDelegate.BindDynamic(this, &ASCWindow::FallingCheck);
		FallDriver->InitializeBegin(BeginDelegate);
	}

	if (DestroyWindowInside_SpecialMoveHandler)
	{
		DestroyWindowInside_SpecialMoveHandler->SpecialStarted.AddDynamic(this, &ASCWindow::DestroyWindowStarted);
		DestroyWindowInside_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCWindow::DestroyWindowComplete);
		DestroyWindowInside_SpecialMoveHandler->SpecialAborted.AddDynamic(this, &ASCWindow::DestroyWindowAborted);
	}

	if (DestroyWindowOutside_SpecialMoveHandler)
	{
		DestroyWindowOutside_SpecialMoveHandler->SpecialStarted.AddDynamic(this, &ASCWindow::DestroyWindowStarted);
		DestroyWindowOutside_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCWindow::DestroyWindowComplete);
		DestroyWindowOutside_SpecialMoveHandler->SpecialAborted.AddDynamic(this, &ASCWindow::DestroyWindowAborted);
	}

	if (DestructionDriver)
	{
		FAnimNotifyEventDelegate BeginDelegate;
		BeginDelegate.BindDynamic(this, &ASCWindow::DestroyWindow);
		DestructionDriver->InitializeBegin(BeginDelegate);
	}

	// Setup Special move delegates
	if (CounselorOpenFromInsideComponent)
	{
		CounselorOpenFromInsideComponent->DestinationReached.AddDynamic(this, &ASCWindow::NativeOpenDestinationReached);
		CounselorOpenFromInsideComponent->SpecialComplete.AddDynamic(this, &ASCWindow::NativeOpenCompleted);
		CounselorOpenFromInsideComponent->SpecialAborted.AddDynamic(this, &ASCWindow::NativeOpenAborted);
	}

	if (CounselorOpenFromOutsideComponent)
	{
		CounselorOpenFromOutsideComponent->DestinationReached.AddDynamic(this, &ASCWindow::NativeOpenDestinationReached);
		CounselorOpenFromOutsideComponent->SpecialComplete.AddDynamic(this, &ASCWindow::NativeOpenCompleted);
		CounselorOpenFromOutsideComponent->SpecialAborted.AddDynamic(this, &ASCWindow::NativeOpenAborted);
	}

	if (CounselorCloseFromInsideComponent)
	{
		CounselorCloseFromInsideComponent->DestinationReached.AddDynamic(this, &ASCWindow::NativeCloseDestinationReached);
		CounselorCloseFromInsideComponent->SpecialComplete.AddDynamic(this, &ASCWindow::NativeCloseCompleted);
		CounselorCloseFromInsideComponent->SpecialAborted.AddDynamic(this, &ASCWindow::NativeCloseAborted);
	}

	if (CounselorCloseFromOutsideComponent)
	{
		CounselorCloseFromOutsideComponent->DestinationReached.AddDynamic(this, &ASCWindow::NativeCloseDestinationReached);
		CounselorCloseFromOutsideComponent->SpecialComplete.AddDynamic(this, &ASCWindow::NativeCloseCompleted);
		CounselorCloseFromOutsideComponent->SpecialAborted.AddDynamic(this, &ASCWindow::NativeCloseAborted);
	}

	if (CounselorVaultFromInsideComponent)
	{
		CounselorVaultFromInsideComponent->SpecialCreated.AddDynamic(this, &ASCWindow::NativeVaultDestinationReached);
		CounselorVaultFromInsideComponent->SpecialStarted.AddDynamic(this, &ASCWindow::NativeVaultStarted);
		CounselorVaultFromInsideComponent->SpecialComplete.AddDynamic(this, &ASCWindow::NativeVaultCompleted);
		CounselorVaultFromInsideComponent->SpecialAborted.AddDynamic(this, &ASCWindow::NativeVaultAborted);
	}

	if (CounselorVaultFromOutsideComponent)
	{
		CounselorVaultFromOutsideComponent->SpecialCreated.AddDynamic(this, &ASCWindow::NativeVaultDestinationReached);
		CounselorVaultFromOutsideComponent->SpecialStarted.AddDynamic(this, &ASCWindow::NativeVaultStarted);
		CounselorVaultFromOutsideComponent->SpecialComplete.AddDynamic(this, &ASCWindow::NativeVaultCompleted);
		CounselorVaultFromOutsideComponent->SpecialAborted.AddDynamic(this, &ASCWindow::NativeVaultAborted);
	}

	if (CounselorDiveFromInsideComponent)
	{
		CounselorDiveFromInsideComponent->DestinationReached.AddDynamic(this, &ASCWindow::NativeDiveDestinationReached);
		CounselorDiveFromInsideComponent->SpecialStarted.AddDynamic(this, &ASCWindow::NativeDiveStarted);
		CounselorDiveFromInsideComponent->SpecialComplete.AddDynamic(this, &ASCWindow::NativeDiveCompleted);
		CounselorDiveFromInsideComponent->SpecialAborted.AddDynamic(this, &ASCWindow::NativeDiveAborted);
	}

	if (CounselorDiveFromOutsideComponent)
	{
		CounselorDiveFromOutsideComponent->DestinationReached.AddDynamic(this, &ASCWindow::NativeDiveDestinationReached);
		CounselorDiveFromOutsideComponent->SpecialStarted.AddDynamic(this, &ASCWindow::NativeDiveStarted);
		CounselorDiveFromOutsideComponent->SpecialComplete.AddDynamic(this, &ASCWindow::NativeDiveCompleted);
		CounselorDiveFromOutsideComponent->SpecialAborted.AddDynamic(this, &ASCWindow::NativeDiveAborted);
	}

	ContextKillComponent->KillAbort.AddDynamic(this, &ASCWindow::KillAborted);
	OuterContextKillComponent->KillAbort.AddDynamic(this, &ASCWindow::KillAborted);
}

void ASCWindow::BeginPlay()
{
	Super::BeginPlay();

	SetActorTickEnabled(false);

	FCollisionQueryParams CollisionQueryParams(SCWindowNames::NAME_HeightTrace);
	CollisionQueryParams.bTraceAsyncScene = true;
	CollisionQueryParams.IgnoreMask = ECC_Pawn | ECC_PhysicsBody | ECC_Vehicle;

	FHitResult Hit;
	const FVector TraceStart = OutsideBlockerCapsule->GetComponentLocation() + FVector::UpVector * 50.f;
	const FVector TraceEnd = OutsideBlockerCapsule->GetComponentLocation() - FVector::UpVector * 1000.f;
	bool bHitSomething = false;
	int32 MaxIterations = 16;
	while (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECollisionChannel::ECC_Visibility, CollisionQueryParams))
	{
		if (--MaxIterations <= 0)
			break;

		if ((Hit.GetComponent()->GetCollisionObjectType() & CollisionQueryParams.IgnoreMask) > 0)
		{
			CollisionQueryParams.AddIgnoredActor(Hit.GetActor());
			continue;
		}

		bHitSomething = true;
		// Subtract the half meter we added to the start of our trace
		const float ActualTraceDistance = FMath::Max(Hit.Distance - 50.f, 0.f);
		if (ActualTraceDistance > FallHeight)
			bFallOnExit = true;
		if (ActualTraceDistance > WoundHeight)
			bWoundOnExit = true;

		const FVector NewLocation = Hit.ImpactPoint + FVector::UpVector * OutsideBlockerCapsule->GetScaledCapsuleHalfHeight();
		OutsideBlockerCapsule->SetWorldLocation(NewLocation);

		// Update the left (outside) side of our navlink
		if (bFallOnExit && PointLinks.Num() > 0)
		{
			FVector HitLocalPos = GetTransform().InverseTransformPosition(Hit.ImpactPoint);
			HitLocalPos.X = PointLinks[0].Left.X; // Keep our forward value
			PointLinks[0].Left = HitLocalPos;
			PointLinks[0].Direction = ENavLinkDirection::RightToLeft; // Can't climb back in

			if (bWoundOnExit)
			{
				// Make sure AI know it will hurt if they go through this window
				PointLinks[0].SetAreaClass(USCNavArea_HighWindow::StaticClass());
				NavigationModifier->SetAreaClass(USCNavArea_HighWindow::StaticClass());
			}

			// Refresh nav data
			UNavigationSystem::UpdateActorAndComponentsInNavOctree(*this);
		}

		break;
	}

	if (!bHitSomething)
	{
		bFallOnExit = true;
		bWoundOnExit = true;
	}

	// Start out asleep
	SetNetDormancy(DORM_DormantAll);

}

void ASCWindow::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (InteractingCounselor)
	{
		 if (bIsFalling && InteractingCounselor->GetCharacterMovement()->IsFalling() == false
			&& GetWorld()->GetTimeSeconds() - FallingStart_Timestamp > FallingMinTime)
		{
			float TimeLeft = 0.f;
			if (UAnimInstance* AnimInstance = InteractingCounselor->GetMesh()->GetAnimInstance())
			{
				AnimInstance->Montage_JumpToSection(SCWindowNames::NAME_ExitFalling);

				const UAnimMontage* Montage = AnimInstance->GetCurrentActiveMontage();
				TimeLeft = Montage ? Montage->GetSectionLength(Montage->GetSectionIndex(SCWindowNames::NAME_ExitFalling)) / (GetMontagePlayRate(InteractingCounselor) * Montage->RateScale): 0.f;
			}

			SetActorTickEnabled(false);
			bIsFalling = false;

			if (InteractingCounselor && InteractingCounselor->GetActiveSCSpecialMove())
				Cast<USCSpecialMove_SoloMontage>(InteractingCounselor->GetActiveSpecialMove())->ResetTimer(TimeLeft);
		}
	}
}

float ASCWindow::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser)
{
	// Replace mesh with smashed window
	MULTICAST_NativeOnWindowDestroyed();

	// Make AI noise
	if (ASCCharacter* DestroyingCharacter = Cast<ASCCharacter>(DamageCauser))
		MakeNoise(1.f, DestroyingCharacter);

	return Damage;
}

void ASCWindow::MULTICAST_NativeOnWindowDestroyed_Implementation()
{
	if (!bIsDestroyed)
	{
		ForceNetUpdate();
		bIsDestroyed = true;
		OnWindowDestroyed();

		// Let the game state know this actor is dead
		UWorld* World = GetWorld();
		if (ASCGameState* GameState = Cast<ASCGameState>(World ? World->GetGameState() : nullptr))
			GameState->ActorBroken(this);

		// Update the nav area so AI knows this is a broken window
		if (NavigationModifier->AreaClass == USCNavArea_Window::StaticClass())
		{
			NavigationModifier->SetAreaClass(USCNavArea_BrokenWindow::StaticClass());
			if (PointLinks.Num() > 0)
			{
				PointLinks[0].SetAreaClass(USCNavArea_BrokenWindow::StaticClass());
			}
		}
	}
}

bool ASCWindow::IsInteractorInside(AActor* Interactor)
{
	if (Interactor == nullptr)
		return false;

	return ((Interactor->GetActorLocation() - GetActorLocation()).GetSafeNormal() | GetActorForwardVector()) > 0;
}

int32 ASCWindow::CanInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	// Protection against using the window while using the window
	if (Character->IsInSpecialMove())
		return 0;

	// Don't interact with a window if it's out of reach
	if (!IsInteractorInside(Character) && bFallOnExit)
		return 0;

	// Don't allow characters in wheelchairs to interact
	if (ASCCharacter* SCCharacter = Cast<ASCCharacter>(Character))
	{
		if (SCCharacter->IsInWheelchair())
			return 0;
	}

	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Character))
	{
		// we are in the middle of grabbing and the pocket knife might hit, wait
		if (Killer->GetGrabbedCounselor() && !Killer->CanGrabKill())
			return 0;

		if (Killer->GetGrabbedCounselor() && Killer->CanGrabKill())
		{
			if (InteractingCounselor)
			{
				return 0;
			}
			else
			{

				const float DistToContextKill = FVector::DistSquared(Killer->GetActorLocation(), ContextKillComponent->GetComponentLocation());
				const float DistToOuterContextKill = FVector::DistSquared(Killer->GetActorLocation(), OuterContextKillComponent->GetComponentLocation());
				if (DistToContextKill < DistToOuterContextKill)
				{
					return ContextKillComponent->CanInteractWith(Character, ViewLocation, ViewRotation);
				}
				else
				{
					return OuterContextKillComponent->CanInteractWith(Character, ViewLocation, ViewRotation);
				}
			}
		}
		else
		{
			// Can't destroy a destroyed window
			return (bIsDestroyed || Killer->IsStunned()) ? 0 : (int32)EILLInteractMethod::Press;
		}
	}

	if (bIsDestroyed || !bIsOpen)
		return (int32)EILLInteractMethod::Press;

	return InteractComponent->InteractMethods;
}

bool ASCWindow::ShouldHighlight(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	return CanInteractWith(Character, ViewLocation, ViewRotation) > 0;
}

void ASCWindow::OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	ASCCharacter* Character = Cast<ASCCharacter>(Interactor);
	check(Character);

	switch (InteractMethod)
	{
	case EILLInteractMethod::Press:
		// Murder
		if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Interactor))
		{
			if (CanThrow(Interactor))
			{
				const float DistToContextKill = FVector::DistSquared(Killer->GetActorLocation(), ContextKillComponent->GetComponentLocation());
				const float DistToOuterContextKill = FVector::DistSquared(Killer->GetActorLocation(), OuterContextKillComponent->GetComponentLocation());
				if (DistToContextKill < DistToOuterContextKill)
				{
					ContextKillComponent->ActivateGrabKill(Killer);
				}
				else
				{
					OuterContextKillComponent->ActivateGrabKill(Killer);
				}
			}
			else if (!bIsDestroyed && Killer->GetGrabbedCounselor() == nullptr) // Paranoid check
			{
				// We don't have a grabbed counselor to throw through the window, let's just smash it!
				const float DistToSpecialMove = FVector::DistSquared(Killer->GetActorLocation(), DestroyWindowInside_SpecialMoveHandler->GetComponentLocation());
				const float DistToOuterSpecialMove = FVector::DistSquared(Killer->GetActorLocation(), DestroyWindowOutside_SpecialMoveHandler->GetComponentLocation());
				if (DistToSpecialMove < DistToOuterSpecialMove)
				{
					DestroyWindowInside_SpecialMoveHandler->ActivateSpecial(Killer);
				}
				else
				{
					DestroyWindowOutside_SpecialMoveHandler->ActivateSpecial(Killer);
				}
			}
			else // Give it up
			{
				Killer->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
			}
		}
		else if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Interactor))
		{
			// Get montage play rate
			const float MontagePlayRate = GetMontagePlayRate(Counselor);

			// Sprint
			if ((Counselor->IsSprinting() || Counselor->IsSprintPressed()) && CanDive(Interactor))
			{
				if (UpdateBlockers(Counselor))
				{
					Counselor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
					DeactivateBlockers(FAnimNotifyData());
					return;
				}

				if (IsInteractorInside(Counselor))
				{
					CounselorDiveFromInsideComponent->ActivateSpecial(Counselor);
				}
				else
				{
					CounselorDiveFromOutsideComponent->ActivateSpecial(Counselor);
				}
			}
			// Open
			else if (CanOpen())
			{
				if (IsInteractorInside(Interactor))
				{
					CounselorOpenFromInsideComponent->ActivateSpecial(Counselor, MontagePlayRate);
				}
				else
				{
					CounselorOpenFromOutsideComponent->ActivateSpecial(Counselor, MontagePlayRate);
				}
			}
			// Climb
			else if (CanVault(Interactor))
			{
				if (UpdateBlockers(Counselor))
				{
					Counselor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
					DeactivateBlockers(FAnimNotifyData());
					return;
				}

				if (IsInteractorInside(Counselor))
				{
					CounselorVaultFromInsideComponent->ActivateSpecial(Counselor, MontagePlayRate);
				}
				else
				{
					CounselorVaultFromOutsideComponent->ActivateSpecial(Counselor, MontagePlayRate);
				}
			}
			// Give up
			else
			{
				Counselor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
			}
		}
		break;

	// Holding closes the window
	case EILLInteractMethod::Hold:
		if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Interactor))
		{
			// Get montage play rate
			const float MontagePlayRate = GetMontagePlayRate(Counselor);

			// Close
			if (CanClose())
			{
				if (IsInteractorInside(Interactor))
				{
					CounselorCloseFromInsideComponent->ActivateSpecial(Counselor, MontagePlayRate);
				}
				else
				{
					CounselorCloseFromOutsideComponent->ActivateSpecial(Counselor, MontagePlayRate);
				}
			}
		}
		break;
	}
}

void ASCWindow::OnHoldStateChanged(AActor* Interactor, EILLHoldInteractionState NewState)
{
	if (NewState == EILLHoldInteractionState::Canceled)
	{
		if (ASCCharacter* Character = Cast<ASCCharacter>(Interactor))
			Character->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
	}
}

void ASCWindow::SetOpen(const bool bOpen)
{
	check(Role == ROLE_Authority)

	ForceNetUpdate();
	bIsOpen = bOpen;
	OnRep_IsOpen();
}

void ASCWindow::OnRep_IsOpen()
{
	if (bIsOpen && bIsDestroyed == false)
		MoveablePaneMesh->SetRelativeLocation(OpenLocation);
	else
		MoveablePaneMesh->SetRelativeLocation(ClosedLocation);

	MovementDriver->SetNotifyOwner(nullptr);
}

void ASCWindow::OnRep_IsDestroyed()
{
	if (bIsDestroyed)
	{
		OnWindowDestroyed(false);
	}
}

bool ASCWindow::CanVault(const AActor* Interactor) const
{
	if (Interactor->IsA(ASCCounselorCharacter::StaticClass()))
		return bIsOpen || bIsDestroyed;

	return false;
}

bool ASCWindow::CanDive(const AActor* Interactor) const
{
	if (const ASCCounselorCharacter* Counselor = Cast<const ASCCounselorCharacter>(Interactor))
		return !Counselor->IsWounded();

	return false;
}

bool ASCWindow::CanThrow(const AActor* Interactor) const
{
	if (const ASCKillerCharacter* Killer = Cast<const ASCKillerCharacter>(Interactor))
	{
		if (Killer->CanGrabKill())
			return true;
	}

	return false;
}

void ASCWindow::KillAborted(ASCCharacter* Interactor, ASCCharacter* Victim)
{
	if (Interactor && Interactor->IsLocallyControlled())
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
}

void ASCWindow::DestroyWindowStarted(ASCCharacter* Interactor)
{
	DestructionDriver->SetNotifyOwner(Interactor);
}

void ASCWindow::DestroyWindowComplete(ASCCharacter* Interactor)
{
	if (Interactor && Interactor->IsLocallyControlled())
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);

	DestructionDriver->SetNotifyOwner(nullptr);
}

void ASCWindow::DestroyWindowAborted(ASCCharacter* Interactor)
{
	if (Interactor && Interactor->IsLocallyControlled())
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
}

void ASCWindow::DestroyWindow(FAnimNotifyData NotifyData)
{
	// Only perform damage on the window on the Server
	if (Role != ROLE_Authority)
		return;

	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(NotifyData.CharacterMesh->GetAttachmentRootActor()))
	{
		if (ASCWeapon* KillerWeapon = Killer->GetCurrentWeapon())
		{
			TakeDamage(KillerWeapon->WeaponStats.Damage * Killer->DamageWorldModifier, FDamageEvent(), Killer->GetCharacterController(), Killer);
		}
	}
}

// OPEN FUNCTIONALITY ----------------------------------------------------
void ASCWindow::NativeOpenDestinationReached(ASCCharacter* Interactor)
{
	OpenDestinationReached();

	// Play the open sound effect
	UGameplayStatics::PlaySoundAtLocation(GetWorld(), WindowOpenSound, GetActorLocation());

	MovementDriver->SetNotifyOwner(Interactor);
	IKDriver->SetNotifyOwner(Interactor);
	InteractingCounselor = Cast<ASCCounselorCharacter>(Interactor);
}

void ASCWindow::NativeOpenCompleted(ASCCharacter* Interactor)
{
	OpenCompleted();

	if (Interactor && Interactor->IsLocallyControlled())
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);

	MoveablePaneMesh->SetRelativeLocation(OpenLocation);
	MovementDriver->SetNotifyOwner(nullptr);
	IKDriver->SetNotifyOwner(nullptr);

	if (Role == ROLE_Authority)
		SetOpen(true);

	InteractingCounselor = nullptr;
}

void ASCWindow::NativeOpenAborted(ASCCharacter* Interactor)
{
	MovementDriver->SetNotifyOwner(nullptr);
	IKDriver->SetNotifyOwner(nullptr);
	InteractingCounselor = nullptr;

	if (Role == ROLE_Authority)
		SetOpen(Interactor->GetActiveSCSpecialMove()->GetTimePercentage() > 0.5f);

	if (Interactor && Interactor->IsLocallyControlled())
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
}

// CLOSE FUNCTIONALITY ----------------------------------------------------
void ASCWindow::NativeCloseDestinationReached(ASCCharacter* Interactor)
{
	CloseDestinationReached();

	// Play the close sound effect
	UGameplayStatics::PlaySoundAtLocation(GetWorld(), WindowCloseSound, GetActorLocation());

	MovementDriver->SetNotifyOwner(Interactor);
	IKDriver->SetNotifyOwner(Interactor);
	InteractingCounselor = Cast<ASCCounselorCharacter>(Interactor);
}

void ASCWindow::NativeCloseCompleted(ASCCharacter* Interactor)
{
	CloseCompleted();

	if (Interactor && Interactor->IsLocallyControlled())
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);

	MoveablePaneMesh->SetRelativeLocation(ClosedLocation);
	MovementDriver->SetNotifyOwner(nullptr);
	IKDriver->SetNotifyOwner(nullptr);

	if (Role == ROLE_Authority)
		SetOpen(false);

	InteractingCounselor = nullptr;
}

void ASCWindow::NativeCloseAborted(ASCCharacter* Interactor)
{
	MovementDriver->SetNotifyOwner(nullptr);
	IKDriver->SetNotifyOwner(nullptr);
	InteractingCounselor = nullptr;

	if (Role == ROLE_Authority)
		SetOpen(Interactor->GetActiveSCSpecialMove()->GetTimePercentage() <= 0.5f);

	if (Interactor && Interactor->IsLocallyControlled())
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
}

// VAULT FUNCTIONALITY ----------------------------------------------------
void ASCWindow::NativeVaultDestinationReached(ASCCharacter* Interactor)
{
	if (!Interactor)
		return;

	InteractingCounselor = Cast<ASCCounselorCharacter>(Interactor);

	VaultDestinationReached();
	Interactor->MoveIgnoreActorChainAdd(this);
	Interactor->SetGravityEnabled(false);

	FallDriver->SetNotifyOwner(Interactor);
	MovementDriver->SetNotifyOwner(Interactor);
}

void ASCWindow::NativeVaultStarted(ASCCharacter* Interactor)
{
	// FAK!
	if (!Interactor)
		return;

	if (UAnimInstance* AnimInstance = Interactor->GetMesh()->GetAnimInstance())
	{
		if (const UAnimMontage* Montage = AnimInstance->GetCurrentActiveMontage())
		{
			// Is this window tall?
			if (bFallOnExit && IsInteractorInside(Interactor))
			{
				float TimeLeft = Montage ? Montage->GetSectionLength(Montage->GetSectionIndex(SCWindowNames::NAME_VaultHigh)) / (GetMontagePlayRate(InteractingCounselor) * Montage->RateScale) : 0.f;
				if (USCSpecialMove_SoloMontage* SM = Cast<USCSpecialMove_SoloMontage>(Interactor->GetActiveSpecialMove()))
				{
					SM->ResetTimer(TimeLeft);
				}
			}
			else
			{
				AnimInstance->Montage_JumpToSection(SCWindowNames::NAME_VaultNormal);
				float TimeLeft = Montage ? Montage->GetSectionLength(Montage->GetSectionIndex(SCWindowNames::NAME_VaultNormal)) / (GetMontagePlayRate(InteractingCounselor) * Montage->RateScale) : 0.f;
				if (USCSpecialMove_SoloMontage* SM = Cast<USCSpecialMove_SoloMontage>(Interactor->GetActiveSpecialMove()))
				{
					SM->ResetTimer(TimeLeft);
				}
			}
		}
	}
}

void ASCWindow::NativeVaultCompleted(ASCCharacter* Interactor)
{
	DeactivateBlockers(FAnimNotifyData());

	if (Interactor != nullptr)
	{
		VaultCompleted();
		Interactor->MoveIgnoreActorChainRemove(this);

		if (Interactor->Controller)
		{
			FRotator Rotation = IsInteractorInside(Interactor) ? IndoorCamera->GetComponentRotation() : OutdoorCamera->GetComponentRotation();
			Rotation.Roll = 0.f;
			Interactor->Controller->SetControlRotation(Rotation);
		}

		Interactor->ReturnCameraToCharacter(CameraReturnLerpTime, VTBlend_EaseInOut, 2.f);

		if (Interactor->Role == ROLE_Authority)
		{
			if (bWoundOnExit && !IsInteractorInside(Interactor))
			{
				Interactor->TakeDamage(HeightDamage, FDamageEvent(USCWindowDiveDamageType::StaticClass()), Interactor->Controller, this);

				// Give this player a badge for jumping out a high window
				if (ASCPlayerState* PS = Interactor->GetPlayerState())
					PS->EarnedBadge(SecondStoryWindowJumpBadge);
			}

			if (bTakeVaultDamage && bIsDestroyed)
			{
				Interactor->TakeDamage(VaultDamage, FDamageEvent(USCWindowDiveDamageType::StaticClass()), Interactor->Controller, this);
			}

			// for some reason the dive fucks up the overlaps, I suspect because of the fly
			Interactor->ForceIndoors(IsInteractorInside(Interactor));
		}

		if (Interactor && Interactor->IsLocallyControlled())
			Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);

		Interactor->SetGravityEnabled(true);
	}

	FallDriver->SetNotifyOwner(nullptr);
	MovementDriver->SetNotifyOwner(nullptr);
	InteractingCounselor = nullptr;
}

void ASCWindow::NativeVaultAborted(ASCCharacter* Interactor)
{
	NativeVaultCompleted(Interactor);
	
	// Only adjust character location on the server
	if (Role == ROLE_Authority)
	{
		ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Interactor);
		if (!Counselor || Counselor->bIsGrabbed)
			return;

		const FVector CharacterLocation = Counselor->GetActorLocation();
		const float InsideDistSq = FVector::DistSquaredXY(CharacterLocation, InsideBlockerCapsule->GetComponentLocation());
		const float OutsideDistSq = FVector::DistSquaredXY(CharacterLocation, OutsideBlockerCapsule->GetComponentLocation());

		// NOTE: If we aborted a special move due to getting trapped,
		// the player is likely to not end up on the correct side
		// of the Window/Wall.  Therefore, if we have a non-null
		// TriggeredTrap, just send the character directly to its intended
		// destination. 
		if (Interactor->TriggeredTrap != nullptr)
		{
			if (CounselorVaultFromInsideComponent && CounselorVaultFromInsideComponent->IsSpecialMoveActive())
			{
				Counselor->SetActorLocation(OutsideBlockerCapsule->GetComponentLocation());
			}
			else if (CounselorVaultFromOutsideComponent && CounselorVaultFromOutsideComponent->IsSpecialMoveActive())
			{
				Counselor->SetActorLocation(InsideBlockerCapsule->GetComponentLocation());
			}
		}
		// Inside is closer
		else if (InsideDistSq < OutsideDistSq)
		{
			Counselor->SetActorLocation(InsideBlockerCapsule->GetComponentLocation());
		}
		else
		{
			// Outside is closer, make sure we don't jump to the ground if we're a second story window though
			const FVector CapsuleLocation = OutsideBlockerCapsule->GetComponentLocation();
			if (bFallOnExit)
			{
				Counselor->SetActorLocation(FVector(CapsuleLocation.X, CapsuleLocation.Y, CharacterLocation.Z));
			}
			else
			{
				Counselor->SetActorLocation(CapsuleLocation);
			}
		}

		// for some reason the dive fucks up the overlaps, I suspect because of the fly
		Counselor->ForceIndoors(IsInteractorInside(Counselor));

		// Make sure we reset the blockers if we abort
		DeactivateBlockers(FAnimNotifyData());
	}
}

// DIVE FUNCTIONALITY ----------------------------------------------------
void ASCWindow::NativeDiveDestinationReached(ASCCharacter* Interactor)
{
	if (!Interactor)
		return;

	InteractingCounselor = Cast<ASCCounselorCharacter>(Interactor);

	DiveDestinationReached();
	Interactor->MoveIgnoreActorChainAdd(this);
	Interactor->SetGravityEnabled(false);

	//Interactor->TakeCameraControl(this, 0.25f, EViewTargetBlendFunction::VTBlend_EaseInOut, 2.f, true);
	FallDriver->SetNotifyOwner(Interactor);

	BreakDriver->SetNotifyOwner(Interactor);
	MovementDriver->SetNotifyOwner(Interactor);
}

void ASCWindow::NativeDiveStarted(ASCCharacter* Interactor)
{
	// FAK!
	if (!Interactor)
		return;

	if (UAnimInstance* AnimInstance = Interactor->GetMesh()->GetAnimInstance())
	{
		if (const UAnimMontage* Montage = AnimInstance->GetCurrentActiveMontage())
		{
			float TimeLeft = Montage ? Montage->GetSectionLength(Montage->GetSectionIndex(SCWindowNames::NAME_DiveEnter)) + Montage->GetSectionLength(Montage->GetSectionIndex(SCWindowNames::NAME_ExitFalling)) : 0.f;
			if (USCSpecialMove_SoloMontage* SM = Cast<USCSpecialMove_SoloMontage>(Interactor->GetActiveSpecialMove()))
			{
				SM->ResetTimer(TimeLeft);
			}
		}
	}

	// deactivate the blockers once we're cleared to dive.
	DeactivateBlockers(FAnimNotifyData());
}

void ASCWindow::NativeDiveCompleted(ASCCharacter* Interactor)
{
	if (Interactor != nullptr)
	{
		DiveCompleted();

		if (Role == ROLE_Authority)
		{
			DealDamageToInteractor(Interactor);

			// Track the number of window dives
			if (ASCGameState_Hunt* GS = GetWorld()->GetGameState<ASCGameState_Hunt>())
			{
				GS->WindowDiveCount++;
			}

			// Give this player a badge for jumping out a high window
			if (bWoundOnExit)
			{
				if (ASCPlayerState* PS = Interactor->GetPlayerState())
					PS->EarnedBadge(SecondStoryWindowJumpBadge);
			}

			// for some reason the dive fucks up the overlaps, I suspect because of the fly
			Interactor->ForceIndoors(IsInteractorInside(Interactor));
		}

		if (Interactor->IsLocallyControlled())
			Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
	}

	if (!bIsOpen)
	{
		MULTICAST_NativeOnWindowDestroyed();
	}

	ResetDiveInteraction(Interactor);
}

void ASCWindow::NativeDiveAborted(ASCCharacter* Interactor)
{
	// Only adjust character location on the server
	if (Role == ROLE_Authority)
	{
		ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Interactor);
		if (!Counselor || Counselor->bIsGrabbed)
		{
			ResetDiveInteraction(Interactor);
			return;
		}

		const FVector CharacterLocation = Counselor->GetActorLocation();
		const float InsideDistSq = FVector::DistSquaredXY(CharacterLocation, InsideBlockerCapsule->GetComponentLocation());
		const float OutsideDistSq = FVector::DistSquaredXY(CharacterLocation, OutsideBlockerCapsule->GetComponentLocation());

		// If aborted during the dive, take damage if we went through the glass.
		if (bBrokenByDive)
		{
			DealDamageToInteractor(Interactor);
		}

		Counselor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);

		// Inside is closer
		if (InsideDistSq < OutsideDistSq)
		{
			Counselor->SetActorLocation(InsideBlockerCapsule->GetComponentLocation());
		}
		else
		{
			// Outside is closer, make sure we don't jump to the ground if we're a second story window though
			const FVector CapsuleLocation = OutsideBlockerCapsule->GetComponentLocation();
			if (bFallOnExit)
			{
				Counselor->SetActorLocation(FVector(CapsuleLocation.X, CapsuleLocation.Y, CharacterLocation.Z));
			}
			else
			{
				Counselor->SetActorLocation(CapsuleLocation);
			}
		}

		// for some reason the dive fucks up the overlaps, I suspect because of the fly
		Counselor->ForceIndoors(IsInteractorInside(Counselor));
	}

	ResetDiveInteraction(Interactor);
}

void ASCWindow::ResetDiveInteraction(ASCCharacter* const Interactor)
{
	DeactivateBlockers(FAnimNotifyData());

	if (Interactor != nullptr)
	{
		Interactor->MoveIgnoreActorChainRemove(this);
		Interactor->ReturnCameraToCharacter(CameraReturnLerpTime, VTBlend_EaseInOut, 2.f);
		Interactor->SetGravityEnabled(true);
	}
	
	FallDriver->SetNotifyOwner(nullptr);
	BreakDriver->SetNotifyOwner(nullptr);
	MovementDriver->SetNotifyOwner(nullptr);

	InteractingCounselor = nullptr;

	// No current need to reset this other than completeness, since broken windows can't be repaired.
	bBrokenByDive = false;
}

void ASCWindow::DealDamageToInteractor(ASCCharacter* const Interactor)
{
	if (bWoundCounselorForDiving && (bIsDestroyed || !bIsOpen))
	{
		Interactor->TakeDamage(DiveDamage, FDamageEvent(USCWindowDiveDamageType::StaticClass()), Interactor->Controller, this);
	}

	if (bWoundOnExit && !IsInteractorInside(Interactor))
	{
		Interactor->TakeDamage(HeightDamage, FDamageEvent(USCWindowDiveDamageType::StaticClass()), Interactor->Controller, this);
	}
}

void ASCWindow::AnimateWindow_Implementation(FAnimNotifyData NotifyData)
{
	if (!MovementDriver->GetNotifyOwner())
		return;

	if (USCCounselorAnimInstance* AnimInstance = Cast<USCCounselorAnimInstance>(NotifyData.AnimInstance))
	{
		// Use our animation curve (inverted for closing)
		float Alpha = AnimInstance->GetUnblendedCurveValue(MovementCurveName);
		if (bIsOpen == false)
			Alpha = 1.f - Alpha;

		const FVector NewRelLoc = FMath::Lerp<FVector>(OpenLocation, ClosedLocation, Alpha);
		MoveablePaneMesh->SetRelativeLocation(NewRelLoc);
	}
}

void ASCWindow::UpdateWindowIK_Implementation(FAnimNotifyData NotifyData)
{
	if (USCCounselorAnimInstance* AnimInstance = Cast <USCCounselorAnimInstance>(NotifyData.AnimInstance))
	{
		if (InteractingCounselor)
		{
			if (IsInteractorInside(InteractingCounselor))
			{
				AnimInstance->UpdateHandIK(LeftHandIKLocation->GetComponentLocation(), RightHandIKLocation->GetComponentLocation());
			}
			else
			{
				AnimInstance->UpdateHandIK(RightHandIKLocation->GetComponentLocation(), LeftHandIKLocation->GetComponentLocation());
			}
		}
	}
}

void ASCWindow::BreakWindow_Implementation(FAnimNotifyData NotifyData)
{
	if (!bIsOpen)
	{
		if (InteractingCounselor)
		{
			InteractingCounselor->ApplyGore(InteractingCounselor->WindowDamageGoreMask);

			if (!bIsDestroyed)
			{
				if (ASCPlayerState* PS = InteractingCounselor->GetPlayerState())
				{
					// NOTE: To properly assign damage for an aborted dive, we need to check to see if the 
					// glass was broken during the dive. 
					if ((CounselorDiveFromInsideComponent && CounselorDiveFromInsideComponent->IsSpecialMoveActive()) 
						|| (CounselorDiveFromOutsideComponent && CounselorDiveFromOutsideComponent->IsSpecialMoveActive()))
					{
						bBrokenByDive = true;
					}

					PS->JumpedThroughClosedWindow();
					PS->EarnedBadge(ClosedWindowJumpBadge);
				}
			}
		}

		MULTICAST_NativeOnWindowDestroyed();
	}
}

void ASCWindow::FallingCheck_Implementation(FAnimNotifyData NotifyData)
{
	// FAK!
	if (!NotifyData.CharacterMesh || !InteractingCounselor)
		return;

	InteractingCounselor->SetGravityEnabled(true);

	if (!IsInteractorInside(NotifyData.CharacterMesh->GetOwner()) && bFallOnExit)
	{
		NotifyData.AnimInstance->Montage_JumpToSection(SCWindowNames::NAME_FallingLoop);
		SetActorTickEnabled(true);
		bIsFalling = true;
		FallingStart_Timestamp = GetWorld()->GetTimeSeconds();
		InteractingCounselor->ReturnCameraToCharacter(0.25f, VTBlend_EaseInOut, 2.f);

		if (USCSpecialMove_SoloMontage* SM = Cast<USCSpecialMove_SoloMontage>(InteractingCounselor->GetActiveSpecialMove()))
		{
			SM->ResetTimer(-1.f);
		}
	}
}

bool ASCWindow::UpdateBlockers(ASCCharacter* Interactor)
{
	return InsideBlockerCapsule->ActivateSpacer(Interactor, 3.f) || OutsideBlockerCapsule->ActivateSpacer(Interactor, 3.f);
}

void ASCWindow::DeactivateBlockers(FAnimNotifyData NotifyData)
{
	InsideBlockerCapsule->DeactivateSpacer();
	OutsideBlockerCapsule->DeactivateSpacer();
}

float ASCWindow::GetMontagePlayRate(ASCCounselorCharacter* Counselor) const
{
	float PlayRate = 1.0f;

	// Apply perk modifiers to montage play rate for opening/going through windows
	if (ASCPlayerState* PS = Counselor ? Counselor->GetPlayerState() : nullptr)
	{
		for (const USCPerkData* Perk : PS->GetActivePerks())
		{
			check(Perk);

			if (!Perk)
				continue;

			PlayRate += Perk->WindowHidespotInteractionSpeedModifier;
		}
	}

	return PlayRate;
}

#undef LOCTEXT_NAMESPACE
