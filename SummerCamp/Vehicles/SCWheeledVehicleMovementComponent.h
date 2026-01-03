// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "WheeledVehicleMovementComponent4W.h"
#include "SCWheeledVehicleMovementComponent.generated.h"

/**
 * @class ASCDriveableVehicle
 */
UCLASS()
class SUMMERCAMP_API USCWheeledVehicleMovementComponent
: public UWheeledVehicleMovementComponent4W
{
	GENERATED_BODY()

public:
	USCWheeledVehicleMovementComponent(const FObjectInitializer& ObjectInitializer);

	// Begin UNavMovementComponent interface
	virtual void RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed) override {};
	virtual void RequestPathMove(const FVector& MoveInput) override {};
	// End UNavMovementComponent interface

	// Begin UWheeledVehicleMovementComponent interface
	virtual void TickVehicle(float DeltaTime) override;
	virtual void UpdateDrag(float DeltaTime) override;
	// End UWheeledVehicleMovementComponent interface

	/** Clear input and stop the vehicel */
	void FullStop();

	/** @return Current throttle input (0 to 1 range) */
	float GetThrottleInput() const { return ThrottleInput; }

	/** Update engine settings */
	void UpdateMaxRPM(const float InRPM);

private:
	/** @return the angle in degrees the wheels are turned */
	float GetSteerAngle() const;

	//////////////////////////////////////////////////
	// AI
public:
	// How far ahead/behind of the vehicle we should check for possible collisions
	UPROPERTY(EditDefaultsOnly, Category="AI")
	float CollisionTestRayLength;

	// Angle (in degrees) we should start our collision tests at
	UPROPERTY(EditDefaultsOnly, Category="AI")
	float CollisionTestStartAngle;

	// Angle (in degrees) we should end our collision tests at
	UPROPERTY(EditDefaultsOnly, Category="AI")
	float CollisionTestEndAngle;

	// Angle (in degrees) between each collision test
	UPROPERTY(EditDefaultsOnly, Category="AI")
	float CollisionTestAngleVaration;

	// Steering adjustment ((input) in degrees) to be applied per positive collision test result
	UPROPERTY(EditDefaultsOnly, Category="AI")
	float CollisionAvoidanceSteeringAdjustment;

	// Speed (cm/s) at which a vehicle is considered stuck
	UPROPERTY(EditDefaultsOnly, Category="AI")
	float StuckSpeed;

	// How long before a vehicle is considered stuck when it's speed is <= StuckSpeed
	UPROPERTY(EditDefaultsOnly, Category="AI")
	float StationaryStuckTime;

	// How long should the vehicle attempt to get unstuck before trying to path again
	UPROPERTY(EditDefaultsOnly, Category="AI")
	float AttemptUnstuckTime;

protected:

	/** Update desired steering and throttle input based on collision checks */
	void CheckForPossibleCollisions();

	/** Check to see if we are stuck and try to get unstuck */
	void UpdateStuck(const float DeltaTime);

	/** Updated desired steering and throttle input based on our current navigation goal */
	void UpdatePathing();

private:
	// How long have we been stuck
	float StuckTime;

	// Are we trying to get unstuck?
	bool bAttemptingToGetUnstuck;

	// Our current desired steering input
	float DesiredSteeringInput;

	// Our current desired throttle input
	float DesiredThrottleInput;
};
