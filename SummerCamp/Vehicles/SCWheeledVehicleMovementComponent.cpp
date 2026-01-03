// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCWheeledVehicleMovementComponent.h"

#include "SCCharacterAIController.h"
#include "SCDriveableVehicle.h"
#include "SCGameMode.h"

extern TAutoConsoleVariable<int32> CVarDebugVehicles;

USCWheeledVehicleMovementComponent::USCWheeledVehicleMovementComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, CollisionTestRayLength(300.f)
, CollisionTestStartAngle(-60.f)
, CollisionTestEndAngle(60.f)
, CollisionTestAngleVaration(15.f)
, CollisionAvoidanceSteeringAdjustment(15.f)
, StuckSpeed(10.f)
, StationaryStuckTime(1.f)
, AttemptUnstuckTime(2.f)
{
}

void USCWheeledVehicleMovementComponent::TickVehicle(float DeltaTime)
{
	Super::TickVehicle(DeltaTime);

	if (ASCDriveableVehicle* VehicleOwner = UpdatedComponent ? Cast<ASCDriveableVehicle>(UpdatedComponent->GetOwner()) : nullptr)
	{
		// Update our AI
		if (ASCCharacterAIController* AIController = Cast<ASCCharacterAIController>(VehicleOwner->GetController()))
		{
			DesiredSteeringInput = 0.f;
			DesiredThrottleInput = bAttemptingToGetUnstuck ? FMath::Clamp((float)GetCurrentGear(), -1.f, 1.f) : 1.f;

			// Perform ray tests to see if we are going to run into something
			CheckForPossibleCollisions();

			// Make sure we aren't stuck
			UpdateStuck(DeltaTime);

			// Path towards our goal
			UpdatePathing();

			// Update our steering and throttle input with our desired values
			VehicleOwner->SetThrottle(DesiredThrottleInput);
			VehicleOwner->SetSteering(DesiredSteeringInput);
		}

		if (CVarDebugVehicles.GetValueOnGameThread() > 0)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Vehicle: %s\nThrottle: %.2f\nSteering: %.2f\nSpeed: %.2f cm/s\nGear: %d\n"), *GetNameSafe(VehicleOwner), RawThrottleInput, RawSteeringInput, GetForwardSpeed(), GetCurrentGear()));
		}
	}
}

void USCWheeledVehicleMovementComponent::UpdateDrag(float DeltaTime)
{
	APawn* Owner = UpdatedComponent ? Cast<APawn>(UpdatedComponent->GetOwner()) : nullptr;
	if (Owner && Owner->GetController())
	{
		Super::UpdateDrag(DeltaTime);
	}
}

void USCWheeledVehicleMovementComponent::FullStop()
{
	ClearInput();
	ServerUpdateState(0.f, 0.f, 0.f, 0.f, GetCurrentGear());
}

void USCWheeledVehicleMovementComponent::UpdateMaxRPM(const float InRPM)
{
	FVehicleEngineData NewEngineSetup = EngineSetup; // Store default Engine Setup values
	NewEngineSetup.MaxRPM = InRPM; // Change the MaxRPM
	UpdateEngineSetup(NewEngineSetup); // Tell it to update
	EngineSetup = NewEngineSetup; // Track the change you cucks!
	ComputeConstants(); // Sets the dumb shit in the base movement component that should probably be deprecated
}

float USCWheeledVehicleMovementComponent::GetSteerAngle() const
{
	for (UVehicleWheel* Wheel : Wheels)
	{
		if (Wheel && Wheel->SteerAngle > 0.f)
			return Wheel->SteerAngle;
	}

	return 0.f;
}

void USCWheeledVehicleMovementComponent::CheckForPossibleCollisions()
{
	UWorld* World = GetWorld();
	if (const ASCGameMode* GameMode = World->GetAuthGameMode<ASCGameMode>())
	{
		if (ASCDriveableVehicle* VehicleOwner = UpdatedComponent ? Cast<ASCDriveableVehicle>(UpdatedComponent->GetOwner()) : nullptr)
		{
			auto PerformRayTest = [&](const FVector& RayStart, const FVector& RayDirection, const float RayLength) -> bool
			{
				if (World)
				{
					const FVector RayEnd = RayStart + (RayDirection * RayLength);
					const bool bHit = World->LineTraceTestByChannel(RayStart, RayEnd, ECC_Visibility);

					if (CVarDebugVehicles.GetValueOnGameThread() > 0)
					{
						DrawDebugLine(World, RayStart, RayEnd, bHit ? FColor::Red : FColor::Green, false, 0.f, 0, 1.25f);
					}

					return bHit;
				}

				return false;
			};


			const FVector ForwardDir = VehicleOwner->GetActorForwardVector();
			const FVector UpDir = VehicleOwner->GetActorUpVector();
			const FVector ForwardRayStart = VehicleOwner->GetFrontRayTraceStartPosition();
			const FVector RearRayStart = VehicleOwner->GetRearRayTraceStartPosition();
			const float RayTestLength = CollisionTestRayLength + ((GetForwardSpeed() * 0.25f) * GameMode->GetCurrentDifficultyUpwardAlpha());
			const bool bIsReversing = GetCurrentGear() < 0;

			// Check to see if there are any possible collisions ahead/behind us
			auto PerformCollisionTests = [&](float& StartTestAngle, const float EndTestAngle, int32& FrontHits, int32& RearHits) -> void
			{
				while (StartTestAngle < EndTestAngle)
				{
					const FVector RotatedRay = ForwardDir.RotateAngleAxis(StartTestAngle, UpDir);
					FrontHits += PerformRayTest(ForwardRayStart, RotatedRay, RayTestLength);
					if (bIsReversing)
						RearHits += PerformRayTest(RearRayStart, RotatedRay, -RayTestLength);

					StartTestAngle += CollisionTestAngleVaration;
				}
			};

			float TestAngle = CollisionTestStartAngle;
			float TestableAreaPerSide = (CollisionTestEndAngle - CollisionTestStartAngle) / 3.f;
			float EndAngle = TestAngle + TestableAreaPerSide;

			// Left Front/Rear
			int32 LeftFrontHits = 0, LeftRearHits = 0;
			PerformCollisionTests(TestAngle, EndAngle, LeftFrontHits, LeftRearHits);

			// Front/Rear
			int32 FrontHits = 0, RearHits = 0;
			EndAngle = TestAngle + TestableAreaPerSide;
			PerformCollisionTests(TestAngle, EndAngle, FrontHits, RearHits);

			// Right Front/Rear
			int32 RightFrontHits = 0, RightRearHits = 0;
			EndAngle = TestAngle + TestableAreaPerSide;
			PerformCollisionTests(TestAngle, EndAngle, RightFrontHits, RightRearHits);

			if (((LeftFrontHits && FrontHits && RightFrontHits) && !bAttemptingToGetUnstuck) || (LeftRearHits && RearHits && RightRearHits))
			{
				DesiredThrottleInput = bIsReversing ? 1.f : -1.f; // Put on the brakes!
			}
			else
			{
				// Adjust steering based on our ray test results
				if (LeftFrontHits || RightRearHits)
				{
					DesiredSteeringInput += LeftFrontHits * CollisionAvoidanceSteeringAdjustment;
					DesiredSteeringInput += RightRearHits * CollisionAvoidanceSteeringAdjustment;
				}
				if (RightFrontHits || LeftRearHits)
				{
					DesiredSteeringInput -= RightFrontHits * CollisionAvoidanceSteeringAdjustment;
					DesiredSteeringInput -= LeftRearHits * CollisionAvoidanceSteeringAdjustment;
				}

				DesiredSteeringInput = FMath::DegreesToRadians(DesiredSteeringInput);

				if ((FrontHits || RearHits) && DesiredSteeringInput == 0.f)
				{
					// Need to slow down
					DesiredThrottleInput = bIsReversing ? -0.1f : 0.1f;
				}
			}
		}
	}
}

void USCWheeledVehicleMovementComponent::UpdateStuck(const float DeltaTime)
{
	// Check to see if we might be stuck
	if (!bAttemptingToGetUnstuck && ThrottleInput && FMath::Abs(GetForwardSpeed()) <= StuckSpeed)
	{
		StuckTime += DeltaTime;

		if (StuckTime > StationaryStuckTime)
		{
			// Let's try going the opposite direction
			SetTargetGear(GetCurrentGear() >= 0 ? -1 : 1, true);
			DesiredThrottleInput = GetCurrentGear();
			DesiredSteeringInput *= -1.f;
			bAttemptingToGetUnstuck = true;
			StuckTime = 0.f;
		}
	}
	else if (bAttemptingToGetUnstuck)
	{
		StuckTime += DeltaTime;

		if (StuckTime > AttemptUnstuckTime)
		{
			// Try to go forward again
			SetTargetGear(GetCurrentGear() <= 0 ? 1 : -1, true);
			DesiredThrottleInput = GetCurrentGear();
			DesiredSteeringInput *= -1.f;
			bAttemptingToGetUnstuck = false;
			StuckTime = 0.f;
		}
	}
	else
	{
		StuckTime = 0.f;
	}
}

void USCWheeledVehicleMovementComponent::UpdatePathing()
{
	if (const ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
	{
		ASCDriveableVehicle* VehicleOwner = UpdatedComponent ? Cast<ASCDriveableVehicle>(UpdatedComponent->GetOwner()) : nullptr;
		if (ASCCharacterAIController* AIController = VehicleOwner ? Cast<ASCCharacterAIController>(VehicleOwner->GetController()) : nullptr)
		{
			UPathFollowingComponent* PathComp = AIController->GetPathFollowingComponent();
			if (PathComp && PathComp->HasValidPath())
			{
				const FVector TargetLocation = PathComp->GetCurrentTargetLocation();
				const FVector ToTarget = TargetLocation - VehicleOwner->GetActorLocation();

				// Set throttle input
				// If our speed will get us to the goal in the next second, let off the gas, lead foot!
				if (!bAttemptingToGetUnstuck)
				{
					const float DistanceToTargetSqrd = ToTarget.SizeSquared2D();
					const float SpeedSqrd = FMath::Square(GetForwardSpeed());
					if (DistanceToTargetSqrd > SpeedSqrd)
					{
						DesiredThrottleInput = 1.f;
					}
					else
					{
						switch (GameMode->GetModeDifficulty())
						{
						case ESCGameModeDifficulty::Easy:
							DesiredThrottleInput = 0.1f;
							break;
						case ESCGameModeDifficulty::Normal:
							DesiredThrottleInput = DistanceToTargetSqrd / SpeedSqrd;
							break;
						case ESCGameModeDifficulty::Hard:
							DesiredThrottleInput = 1.f;
							break;
						}
					}
				}

				// Set steering according to the right dot
				const float RightDot = ToTarget.GetSafeNormal() | VehicleOwner->GetActorRightVector();
				DesiredSteeringInput += FMath::Clamp(RightDot / FMath::Cos(FMath::DegreesToRadians(FMath::Max(0.f, 55.f - GetSteerAngle()))), -1.f, 1.f); // Normalize the steering a bit
				if (GetCurrentGear() < 0)
					DesiredSteeringInput *= -1; // Opposite steering input since we are going in reverse
			}
			else if (!bAttemptingToGetUnstuck)
			{
				DesiredThrottleInput = 0.f; // We aren't trying to go anywhere
			}
		}
	}
}
