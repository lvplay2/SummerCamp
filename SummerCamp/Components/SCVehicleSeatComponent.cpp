// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCVehicleSeatComponent.h"

#include "Animation/AnimInstance.h"

#include "SCContextKillComponent.h"
#include "SCCounselorAnimInstance.h"
#include "SCCounselorCharacter.h"
#include "SCDriveableVehicle.h"
#include "SCInteractableManagerComponent.h"
#include "SCKillerCharacter.h"
#include "SCSpacerCapsuleComponent.h"
#include "SCSpecialMoveComponent.h"
#include "SCWheeledVehicleMovementComponent.h"

USCVehicleSeatComponent::USCVehicleSeatComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, AttachSocket(NAME_None)
{
	InteractMethods = (int32)EILLInteractMethod::Press;
}

void USCVehicleSeatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USCVehicleSeatComponent, RequiredPartClass);
}

void USCVehicleSeatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwnerRole() == ROLE_Authority)
	{
		GetOwner()->ForceNetUpdate();
		bIsEnabled = false;

		SpecialMoveComponents.Empty();
		KillComponents.Empty();
		SpacerComponents.Empty();

		TArray<USceneComponent*> ChildComponents;
		GetChildrenComponents(true, ChildComponents);
		for (USceneComponent* Comp : ChildComponents)
		{
			if (USCSpecialMoveComponent* SpecialMove = Cast<USCSpecialMoveComponent>(Comp))
			{
				SpecialMoveComponents.Add(SpecialMove);
			}
			else if (USCContextKillComponent* Kill = Cast<USCContextKillComponent>(Comp))
			{
				KillComponents.Add(Kill);
			}
			else if (USCSpacerCapsuleComponent* Spacer = Cast<USCSpacerCapsuleComponent>(Comp))
			{
				SpacerComponents.Add(Spacer);
			}
		}
	}

	ParentVehicle = Cast<ASCDriveableVehicle>(GetOwner());
}

void USCVehicleSeatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_ExitSeat);
	}

	Super::EndPlay(EndPlayReason);
}

void USCVehicleSeatComponent::OnInteractBroadcast(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	Super::OnInteractBroadcast(Interactor, InteractMethod);

	// If we're a counselor
	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Interactor))
	{
		switch (InteractMethod)
		{
		case EILLInteractMethod::Press:

			// High lag situation handling Server/Client don't agree but this hits on the server and will tell the Counselor to fuck off
			if (ParentVehicle && ParentVehicle->bJasonFlippingBoat)
			{
				Counselor->GetInteractableManagerComponent()->UnlockInteraction(this);
				return;
			}

			// No one in seat
			if (!CounselorInSeat)
			{
				// Stop any montage trying to play
				Counselor->ForceStopInteractAnim();

				float BestDistanceSq = FLT_MAX;
				USCSpecialMoveComponent* BestSpecialMove = nullptr;
				for (USCSpecialMoveComponent* SpecialMove : SpecialMoveComponents)
				{
					if (TSubclassOf<USCSpecialMove_SoloMontage> SpecialMoveClass = SpecialMove->GetDesiredSpecialMove(Counselor))
					{
						if (const USCSpecialMove_SoloMontage* const DefaultSpecialMove = Cast<USCSpecialMove_SoloMontage>(SpecialMoveClass->GetDefaultObject()))
						{
							if (!DefaultSpecialMove->IsKillerSpecialMove())
							{
								FVector CounselorRelativeLocation = ParentVehicle->GetTransform().InverseTransformPosition(Counselor->GetActorLocation());
								FVector SpecialMoveRelativeLocation = ParentVehicle->GetTransform().InverseTransformPosition(SpecialMove->GetComponentLocation());
								
								if (Counselor->bInWater)
								{
									SpecialMoveRelativeLocation.X = CounselorRelativeLocation.X;
								}

								float DistanceSq = (CounselorRelativeLocation - SpecialMoveRelativeLocation).SizeSquared();
								if (DistanceSq < BestDistanceSq)
								{
									BestDistanceSq = DistanceSq;
									BestSpecialMove = SpecialMove;
								}
							}
						}
					}
				}

				if (ParentVehicle)
				{
					Counselor->MULTICAST_EnterVehicle(ParentVehicle, this, BestSpecialMove);
				}
			}
			else if (CounselorInSeat == Counselor && Counselor->GetCurrentSeat() == this) // Someone in the seat, paranoid check that the interactor is the character in the seat
			{
				if (ParentVehicle && ParentVehicle->IsBeingSlammed() == false)
				{
					Counselor->MULTICAST_ExitVehicle(IsSeatBlocked());
				}
			}
			break;
		}
	}
	else if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Interactor)) // Otherwise we're probably a killer
	{
		ASCCounselorCharacter* CounselorToGrab = CounselorInSeat;
		// If this seat is empty but interaction is available, there's a good chance we can grab the counselor in the seat next to this one
		if (!CounselorToGrab)
		{
			for (USCVehicleSeatComponent* Seat : ParentVehicle->Seats)
			{
				if (Seat == this)
					continue;

				if (Seat->IsFrontSeat() == IsFrontSeat())
				{
					CounselorToGrab = Seat->CounselorInSeat;
					break;
				}
			}

			// Paranoid check
			if (!CounselorToGrab)
			{
				Killer->GetInteractableManagerComponent()->UnlockInteraction(Killer->GetInteractableManagerComponent()->GetLockedInteractable());
				return;
			}
		}

		switch (InteractMethod)
		{
		case EILLInteractMethod::Press:
			// Perform special move and rip that fucking counselor out of the damn vehicle
			float BestDistanceSq = FLT_MAX;
			USCContextKillComponent* BestKillComp = nullptr;
			for (USCContextKillComponent* KillComp : KillComponents)
			{
				float DistanceSq = (Killer->GetActorLocation() - KillComp->GetComponentLocation()).SizeSquared();
				if (DistanceSq < BestDistanceSq)
				{
					BestDistanceSq = DistanceSq;
					BestKillComp = KillComp;
				}
			}

			// Can always grab people out of a car, only perform a grab on a counselor in a boat if the killer isn't underwater
			if (ParentVehicle->VehicleType == EDriveableVehicleType::Car || (ParentVehicle->VehicleType == EDriveableVehicleType::Boat && !Killer->bUnderWater))
			{
				Killer->GrabCounselorOutOfVehicle(CounselorToGrab, BestKillComp);
			}

			Killer->GetInteractableManagerComponent()->UnlockInteraction(Killer->GetInteractableManagerComponent()->GetLockedInteractable());
			break;
		}
	}
}

int32 USCVehicleSeatComponent::CanInteractWithBroadcast(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	// Paranoid
	if (!ParentVehicle)
		return 0;

	// Why would you get out? You won the game!
	if (ParentVehicle->bEscaping)
		return 0;

	const int32 Flags = Super::CanInteractWithBroadcast(Character, ViewLocation, ViewRotation);
	if (!Flags)
		return 0;

	// Can't enter/exit while being slammed
	if (ParentVehicle->IsBeingSlammed())
		return 0;

	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character))
	{
		// You'll be out of your seat soon. . .
		if (ParentVehicle->bJasonFlippingBoat)
			return 0;

		if (CounselorInSeat)
		{
			// NOT MY COUNSELOR!
			if (CounselorInSeat != Counselor)
				return 0;

			// Counselor is either entering or exiting a seat already
			if (CounselorInSeat->PendingSeat)
				return 0;

			// Potentially flagged first in high lag situations
			if (CounselorInSeat->IsInContextKill())
				return 0;

			// Prevent the counselor from getting out of the car when the door is blocked
			if (IsSeatBlocked())
			{
				// But only if there is a driver
				if (ParentVehicle->Driver)
					return 0;

				// If the seat is blocked and we have no driver, make sure we have an empty spot to put the counselor
				const FVector EmptyCapsuleLocation = ParentVehicle->GetEmptyCapsuleLocation(CounselorInSeat);
				// All exit/eject locations are occupied
				if (EmptyCapsuleLocation.IsZero())
					return 0;
			}
		}
		else
		{
			// If no counselor in the seat, but the interactor is potentially still in another seat, block interaction
			if (USCCounselorAnimInstance* AnimInst = Cast<USCCounselorAnimInstance>(Counselor->GetMesh()->GetAnimInstance()))
			{
				if (AnimInst->bIsInCar || AnimInst->bIsInBoat)
				{
					return 0;
				}
			}

			// Don't let drowning people into the boat
			if (Counselor->IsInContextKill())
				return 0;
		}

		if (RequiredPartClass)
		{
			if (Counselor->HasSmallRepairPart(RequiredPartClass))
				return Flags;

			if (ASCItem* EquippedItem = Counselor->GetEquippedItem())
			{
				if (EquippedItem->IsA(RequiredPartClass))
					return Flags;
			}

			return 0;
		}

		if (!ParentVehicle->CanEnterExitVehicle())
			return 0;

		// Only do the falling check for boats so that counselors fall all of the way into the water before allowing them back in the boat
		if (ParentVehicle->VehicleType == EDriveableVehicleType::Boat && Counselor->GetCharacterMovement()->IsFalling())
			return 0;
	}
	else if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Character))
	{
		// Can't interact with a seat if we already have a grabbed counselor
		if (Killer->GetGrabbedCounselor())
			return 0;

		// If there's no counselor in this seat, see if there's a counselor in the seat next to it
		if (!CounselorInSeat || !CounselorInSeat->IsInVehicle() || CounselorInSeat->bLeavingSeat)
		{
			bool bFoundValidCounselor = false;
			for (auto Seat : ParentVehicle->Seats)
			{
				// Skip this seat
				if (Seat == this)
					continue;

				// The seat is across the vehicle (next to "this" seat)
				if (Seat->IsFrontSeat() == IsFrontSeat())
				{
					// Found a valid counselor!
					bFoundValidCounselor = Seat->CounselorInSeat && Seat->CounselorInSeat->IsInVehicle() && !Seat->CounselorInSeat->bLeavingSeat;
					break;
				}
			}

			if (!bFoundValidCounselor)
				return 0;
		}

		// Jason cannot interact with boat seats while under water
		if (ParentVehicle->VehicleType == EDriveableVehicleType::Boat && Killer->bUnderWater)
			return 0;

		if (USCWheeledVehicleMovementComponent* Movement = Cast<USCWheeledVehicleMovementComponent>(ParentVehicle->GetVehicleMovement()))
		{
			if ((FMath::Abs(Movement->GetThrottleInput()) > 0.f && ParentVehicle->GetValidThrottle()) || FMath::Abs(Movement->GetForwardSpeed()) > ParentVehicle->GetMinUseCarDoorSpeed())
				return 0;
		}
	}

	return Flags;
}

void USCVehicleSeatComponent::EnableExitSpacers(ASCCharacter* Interactor, const bool bEnabled) const
{
	for (USCSpacerCapsuleComponent* Spacer : SpacerComponents)
	{
		if (bEnabled)
			Spacer->ActivateSpacer(Interactor, 0.5f);
		else
			Spacer->DeactivateSpacer();
	}
}

bool USCVehicleSeatComponent::IsSeatBlocked() const
{
	// Prevent the counselor from getting out of the car when the door is blocked
	static const FName NAME_VehicleExitTrace(TEXT("VehicleExitTrace"));
	FCollisionQueryParams QueryParams(NAME_VehicleExitTrace, true, ParentVehicle);
	QueryParams.AddIgnoredActor(CounselorInSeat);

	const FVector Start = GetComponentLocation();
	const FVector End = Start + GetComponentRotation().Vector() * 150.f;

	return GetWorld()->SweepTestByChannel(Start, End, FQuat::Identity, ECC_WorldDynamic, FCollisionShape::MakeSphere(35.f), QueryParams);
}
