// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCHairAnimInstance.generated.h"

/**
 * @class USCHairAnimInstance
 */
UCLASS()
class SUMMERCAMP_API USCHairAnimInstance
: public UAnimInstance
{
	GENERATED_BODY()

public:
	USCHairAnimInstance(const FObjectInitializer& ObjectInitializer);

	// Begin UAnimInstance interface
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	// End UAnimInstance interface

protected:
	// Target FPS to run the simulation at (ScaledAlpha will be adjusted based on the current frame rate)
	UPROPERTY(EditDefaultsOnly, Category = "Hair Sim")
	float TargetFramerate;

	// Max value to pass to the physics simulation
	UPROPERTY(EditDefaultsOnly, Category = "Hair Sim")
	float MaxAlpha;

	// Alpha value to pass to the physics simulation (unsmoothed)
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Hair Sim")
	float ScaledAlpha;

	// 1 / TargetFramerate, basically the max DT for our simulation
	float InvFramerate;
};
