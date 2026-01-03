// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "GameFramework/CharacterMovementComponent.h"
#include "ILLCharacterMovement.generated.h"

/**
 * @class UILLCharacterMovement
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLCharacterMovement
: public UCharacterMovementComponent
{
	GENERATED_UCLASS_BODY()

	// Begin UMovementComponent Interface
	virtual float GetMaxSpeed() const override;
	virtual float GetMaxAcceleration() const override;
	// End UMovementComponent Interface

	/** Simple blueprint-accessible multiplier for GetMaxSpeed. */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite, AdvancedDisplay)
	float CustomSpeedMultiplier;

	/** Simple blueprint-accessible multiplier for GetMaxAcceleration. */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite, AdvancedDisplay)
	float CustomAccelerationMultiplier;
};
