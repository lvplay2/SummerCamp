// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLCharacterMovement.h"
#include "SCCharacterMovement.generated.h"

/**
 * @class USCCharacterMovement
 */
UCLASS()
class SUMMERCAMP_API USCCharacterMovement
: public UILLCharacterMovement
{
	GENERATED_UCLASS_BODY()

public:
	// Begin CharacterMovementComponent interface
	virtual float GetMaxSpeed() const override;
	virtual void PhysSwimming(float deltaTime, int32 Iterations) override;
	virtual float ComputeAnalogInputModifier() const override { return Acceleration.SizeSquared() > 0.f ? 1.f : 0.f; }
	// End CharacterMovementComponent interface

	virtual bool IsSprinting() const;
	virtual bool IsRunning() const;

	// The maximum ground speed when sprinting
	UPROPERTY(Category = "Character Movement", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float MaxSprintSpeed;

	// The maximum ground speed when Running
	UPROPERTY(Category = "Character Movement", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float MaxRunSpeed;

	// The maximum ground speed when stamina is depleated
	UPROPERTY(Category = "Character Movement", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float MaxSlowRunSpeed;

	// The maximum ground speed while in combat
	UPROPERTY(Category = "Character Movement", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float MaxCombatSpeed;
};
