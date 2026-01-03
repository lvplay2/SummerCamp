// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCItem.h"

#include "SCNoiseMaker.generated.h"

class USCScoringEvent;

/**
 * @class ASCNoiseMaker
 */
UCLASS()
class SUMMERCAMP_API ASCNoiseMaker
: public ASCItem
{
	GENERATED_UCLASS_BODY()

public:
	// BEGIN AActor Interface
	virtual void PostInitializeComponents() override;
	// END AActor Interface

	// BEGIN ASCItem interface
	virtual bool Use(bool bFromInput) override;
	// END ASCItem interface

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<class ASCNoiseMaker_Projectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	TSubclassOf<USCScoringEvent> UseScoreEvent;

protected:
	/** Launch the projectile */
	UFUNCTION()
	void ThrowProjectile();

	/** Delay between USE and THROW so that the character has some time to play the animation before the throw is implemented. */
	UPROPERTY(EditDefaultsOnly)
	float ThrowDelayTime;

	FTimerHandle TimerHandle_ThrowTimer;

	/** 
	 * Curve controlling min and max pitch and velocity at said pitch.
	 * X Axis is initial velocity.
	 * Y Axis is pitch angle.
	 */
	UPROPERTY(EditDefaultsOnly)
	UCurveFloat* VelocityPitchCurve;

	/** Cached pitch constraints from the VelocityPitchCurve */
	FVector2D PitchConstraints;
};
