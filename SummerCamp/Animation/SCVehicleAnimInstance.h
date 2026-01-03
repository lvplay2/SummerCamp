// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "VehicleAnimInstance.h"
#include "SCVehicleAnimInstance.generated.h"

class ASCDriveableVehicle;

/**
 * @class USCVehicleAnimInstance
 */
UCLASS(Transient)
class SUMMERCAMP_API USCVehicleAnimInstance
: public UVehicleAnimInstance
{
	GENERATED_UCLASS_BODY()

public:
	// Begin UAnimInstance interface
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	// End UAnimInstance interface

	/** Upcast of our vehicle owner */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Default")
	ASCDriveableVehicle* Vehicle;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Movement")
	float Speed;

	/** If true, use procedural turns (e.g. Boats banking) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	uint32 bUseProceduralTurning : 1;

	/** Minimum speed for leaning to blend in */
	UPROPERTY(Transient, EditDefaultsOnly, Category = "Movement")
	float MinTurnSpeed;

	/** Amount of turn to apply to the vehicle based on speed */
	UPROPERTY(Transient, EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float TurnAlpha;

	/** Fudge number for the vehicle's max speed cm/s (boat banking math based on this */
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float PotentialMaxSpeed;

	UFUNCTION(BlueprintCallable, Category = "Default")
	void OnEjectCounselors();
};
