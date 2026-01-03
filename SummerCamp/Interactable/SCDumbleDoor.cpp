// Copyright (C) 2015-2017 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCDumbleDoor.h"

#include "AI/Navigation/NavAreas/NavArea_Default.h"
#include "AI/Navigation/NavModifierComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

#include "ILLCharacterAnimInstance.h"
#include "ILLPlayerInput.h"

#include "NavAreas/SCNavArea_LockedDoorMultiAgent.h"
#include "NavAreas/SCNavArea_UnlockedDoor.h"
#include "SCBarricade.h"
#include "SCContextKillComponent.h"
#include "SCCounselorAIController.h"
#include "SCCounselorCharacter.h"
#include "SCGameState_Hunt.h"
#include "SCInteractableManagerComponent.h"
#include "SCInteractComponent.h"
#include "SCKillerCharacter.h"
#include "SCPlayerState.h"
#include "SCSpecialMoveComponent.h"
#include "SCStatusIconComponent.h"
#include "SCWeapon.h"
#include "SCGameState_Paranoia.h"

static const FName NAME_DoorTransform(TEXT("DoorRotation"));	// for now use something that exists

ASCDumbleDoor::ASCDumbleDoor(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
,	bIsDoorTransformDriverActive(false)
,	StartLerp(0.0f)
,	TargetLerp(0.0f)
{
	Mesh2 = CreateDefaultSubobject<UDestructibleComponent>(TEXT("StaticMesh2"));
	Mesh2->SetupAttachment(Pivot);
	Mesh2->bGenerateOverlapEvents = false;

	DoorCollision2 = CreateDefaultSubobject<UBoxComponent>(TEXT("DoorCollision2"));
	DoorCollision2->SetupAttachment(Mesh2);
	DoorCollision2->bGenerateOverlapEvents = false;

}

void ASCDumbleDoor::BeginPlay()
{
	ClosedLocation = Mesh->GetRelativeTransform().GetLocation();
	ClosedLocation2 = Mesh2->GetRelativeTransform().GetLocation();

	Super::BeginPlay();
}

float ASCDumbleDoor::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float PreviousDestructionRatio = (float)Health / (float)BaseDoorHealth;
	int RetDamageAmount = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);


	if (Health > 0)
	{
		const float DestructionRatio = (float)Health / (float)BaseDoorHealth;

		if (DestructionRatio <= HeavyDestructionThreshold && PreviousDestructionRatio > HeavyDestructionThreshold)
		{
			UpdateDestructionStateExtra(HeavyDestructionMesh2);
		}
		else if (DestructionRatio <= MediumDestructionThreshold && PreviousDestructionRatio > MediumDestructionThreshold)
		{
			UpdateDestructionStateExtra(MediumDestructionMesh2);
		}
		else if (DestructionRatio <= LightDestructionThreshold && PreviousDestructionRatio > LightDestructionThreshold)
		{
			UpdateDestructionStateExtra(LightDestructionMesh2);
		}
		else
		{
			if (LinkedBarricade)
			{
				LinkedBarricade->BreakAndDestroy();
			}
		}
	}

	return RetDamageAmount;
}

void ASCDumbleDoor::OnRep_Health(int32 OldHealth)
{
	if (Health > 0)
	{
		const float PreviousDestructionRatio = (float)OldHealth / (float)BaseDoorHealth;
		const float DestructionRatio = (float)Health / (float)BaseDoorHealth;
		if (DestructionRatio <= HeavyDestructionThreshold && PreviousDestructionRatio > HeavyDestructionThreshold)
		{
			UpdateDestructionState(HeavyDestructionMesh);
			UpdateDestructionStateExtra(HeavyDestructionMesh2);
		}
		else if (DestructionRatio <= MediumDestructionThreshold && PreviousDestructionRatio > MediumDestructionThreshold)
		{
			UpdateDestructionState(MediumDestructionMesh);
			UpdateDestructionStateExtra(MediumDestructionMesh2);
		}
		else if (DestructionRatio <= LightDestructionThreshold && PreviousDestructionRatio > LightDestructionThreshold)
		{
			UpdateDestructionState(LightDestructionMesh);
			UpdateDestructionStateExtra(LightDestructionMesh2);
		}

		// Don't play the sound if the door is healing. A.K.A. the initial on rep at the beginning of the match.
		if (HasActorBegunPlay() && Health != BaseDoorHealth)
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), DoorHitSoundCue, GetActorLocation());
	}
	else if (bIsDestroyed == false)
	{
		// This happened due to relevancy change, destroy the door anew!
		Local_DestroyDoor(nullptr, BreakExplosionForce);
	}
}

// ~AActor Interface
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// State

void ASCDumbleDoor::OnRep_IsOpen()
{
	//bIsDoorDriverActive = false;

	// Stop animating the door and set the final orientation
	bIsDoorTransformDriverActive = false;
	Mesh->SetRelativeLocation(bIsOpen ? (ClosedLocation + OpenDelta) : ClosedLocation);
	Mesh2->SetRelativeLocation(bIsOpen ? (ClosedLocation2 + OpenDelta2) : ClosedLocation2);
}

void ASCDumbleDoor::Local_DestroyDoor(AActor* DamageCauser, float Impulse)
{
	if (bIsDestroyed)
	{
		return;
	}

	// copy pasta for our extra meshes
	const FVector Facing = [this, DamageCauser]()
	{
		if (DamageCauser)
			return (DamageCauser->GetActorLocation() - InteractComponent->GetComponentLocation());

		return OutArrow->GetComponentRotation().Vector();
	}().GetSafeNormal2D();

	Mesh2->SetSimulatePhysics(true);

	// If we have a damage causer apply dustruction more accurately
	if (DamageCauser)
	{
		Mesh2->ApplyRadiusDamage(100.f, DamageCauser->GetActorLocation() + (Facing * 5.f), 3000.f, Impulse, true);
	}
	else
	{
		Mesh2->ApplyRadiusDamage(100.f, OutArrow->GetComponentLocation() + (Facing * 5.f), 3000.f, Impulse, true);
	}
	Mesh2->CastShadow = false;
	DoorCollision2->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// do the original after so callbacks are correctly timed
	Super::Local_DestroyDoor(DamageCauser, Impulse);
}

void ASCDumbleDoor::OnDestroyDoorMesh()
{
	Super::OnDestroyDoorMesh();

	Mesh2->DestroyComponent();
	Mesh2 = nullptr;
}

// ~State
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Utilities

void ASCDumbleDoor::OpenCloseDoorDriverBegin(FAnimNotifyData NotifyData)
{
	//bIsDoorDriverActive = true;

	bIsDoorTransformDriverActive = true;

	if (bIsOpen)
	{
		TargetLerp = 0.0f;
		StartLerp = 1.0f;
		// This will play the close whoosh
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), DoorsClosedSound, GetActorLocation());
	}
	else
	{
		TargetLerp = 1.0f;
		StartLerp = 0.0f;
		// This will play the open whoosh
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), DoorsOpenSound, GetActorLocation());
	}


}
void ASCDumbleDoor::OpenCloseDoorDriverTick(FAnimNotifyData NotifyData)
{
	// use our own new and improved driver bool... thingy... thing
	const UILLCharacterAnimInstance* AnimInstance = Cast<UILLCharacterAnimInstance>(NotifyData.AnimInstance);
	if (bIsDoorTransformDriverActive && AnimInstance)
	{
		const float LerpAlpha = FMath::Clamp(AnimInstance->GetUnblendedCurveValue(NAME_DoorTransform), 0.f, 1.f);
		const float LerpVal = FMath::Lerp(StartLerp, TargetLerp, LerpAlpha);

		// only need translation for now so lets do that
		Mesh->SetRelativeLocation(ClosedLocation + (OpenDelta * LerpVal));
		Mesh2->SetRelativeLocation(ClosedLocation2 + (OpenDelta2 * LerpVal));
	}
}


void ASCDumbleDoor::OpenCloseDoorCompleted(ASCCharacter* Interactor)
{
	if (bIsDoorTransformDriverActive)
	{
		// Snap the door to open or close (inverse of bIsOpen)
		Mesh->SetRelativeLocation(bIsOpen ? ClosedLocation :(ClosedLocation + OpenDelta));
		Mesh2->SetRelativeLocation(bIsOpen ? ClosedLocation2 :  (ClosedLocation2 + OpenDelta2));

	}

	Super::OpenCloseDoorCompleted(Interactor);
}

void ASCDumbleDoor::OpenCloseDoorDriverEnd(FAnimNotifyData NotifyData)
{
	bIsDoorTransformDriverActive = false;
}

void ASCDumbleDoor::DoorKillDriverTick(FAnimNotifyData NotifyData)
{
	// Can be set false by aborting a special move
	if (bIsDoorTransformDriverActive)
	{
		Mesh->SetRelativeLocation(ClosedLocation);
		Mesh2->SetRelativeLocation(ClosedLocation2);
	}
}

void ASCDumbleDoor::UpdateDestructionStateExtra(UDestructibleMesh* NewDestructionMesh)
{
	Mesh2->SetDestructibleMesh(NewDestructionMesh);

	// for the double door, do this on any destruction state
	if (LinkedBarricade)
	{
		LinkedBarricade->BreakAndDestroy();
	}

	OnUpdateDestructionState();
	OnBarricade(false);
}

void ASCDumbleDoor::DoorKillDriverEnd(FAnimNotifyData NotifyData)
{
	bIsDoorTransformDriverActive = false;
	Super::DoorKillDriverEnd(NotifyData);
}

void ASCDumbleDoor::OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	// beatwho: only override this path? not the most ideal way but dont want to modify super
	if (InteractMethod == EILLInteractMethod::Press && Cast<ASCKillerCharacter>(Interactor) == nullptr) // Press and not killer
	{
		ASCCharacter* Character = Cast<ASCCharacter>(Interactor);
		check(Character);

		// Can't open a locked door
		if (!CanOpenDoor())
		{
			for (USCInteractComponent* Interact : InteractComponents)
			{
				Character->GetInteractableManagerComponent()->UnlockInteraction(Interact);
			}
			MULTICAST_PlaySound(LockedDoorSound);
			return;
		}

		USCSpecialMoveComponent* SpecialMoveComp = nullptr;
		const bool bIsInside = IsInteractorInside(Interactor);
		if (bIsOpen)
		{
			SpecialMoveComp = bIsInside ? CloseDoorInsideForward_SpecialMoveHandler : CloseDoorOutsideForward_SpecialMoveHandler;
		}
		else
		{
			SpecialMoveComp = bIsInside ? OpenDoorInside_SpecialMoveHandler : Character->IsInWheelchair() ? OpenDoorWheelchairOutside_SpecialMoveHandler : OpenDoorOutside_SpecialMoveHandler;
		}

		SpecialMoveComp->ActivateSpecial(Character);

		// Set owner of the driver
		OpenCloseDoorDriver->SetNotifyOwner(Character);
		MULTICAST_SetDoorOpenCloseDriverOwner(Interactor);

		return;
	}
	
	Super::OnInteract(Interactor, InteractMethod);
}

void ASCDumbleDoor::ToggleLock(ASCCharacter* Interactor)
{
	Super::ToggleLock(Interactor);
}
