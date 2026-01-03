// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCVehicleAnimInstance.h"

#include "SCCounselorCharacter.h"
#include "SCDriveableVehicle.h"
#include "SCVehicleSeatComponent.h"
#include "SCWheeledVehicleMovementComponent.h"

USCVehicleAnimInstance::USCVehicleAnimInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, PotentialMaxSpeed(800.f)
{
}

void USCVehicleAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Vehicle = Cast<ASCDriveableVehicle>(GetOwningActor());
}

void USCVehicleAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!Vehicle)
		return;

	const FVector Velocity = Vehicle->GetVelocity();
	const FVector NormalizedVelocity = Velocity.GetSafeNormal();
	const FVector ForwardVector = Vehicle->GetActorForwardVector();
	const FVector RightVector = Vehicle->GetActorRightVector();

	Speed = Velocity.Size();

	TurnAlpha = FMath::Max(0.f, Speed - MinTurnSpeed) / (PotentialMaxSpeed - MinTurnSpeed);
}

void USCVehicleAnimInstance::OnEjectCounselors()
{
	if (Vehicle)
	{
		if (Vehicle->Role != ROLE_Authority)
			return;

		for (auto Seat : Vehicle->Seats)
		{
			if (Seat->CounselorInSeat && !Seat->CounselorInSeat->bLeavingSeat)
			{
				Vehicle->EjectCounselor(Seat->CounselorInSeat, true);
			}
		}
	}
}
