// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"

#include "SCProjectile.generated.h"

class USCScoringEvent;

/**
 * @class ASCProjectile
 */
UCLASS()
class SUMMERCAMP_API ASCProjectile
: public AActor
{
	GENERATED_UCLASS_BODY()

	// Begin AActor interface
	virtual void PostInitializeComponents() override;
	virtual void Destroyed() override;
	// End AActor interface

	void SetIgnoredActor(AActor* Actor);
	void RemoveIgnoredActor(AActor* Actor);

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USphereComponent* Root;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Particles")
	UParticleSystemComponent* ParticleSystem;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	UProjectileMovementComponent* ProjectileMovement;

	UFUNCTION(BlueprintCallable, Category = "Movement")
	virtual void InitVelocity(const FVector& Direction);

	UFUNCTION(BlueprintCallable, Category = "Scoring")
	void TriggerScoreEvent(TSubclassOf<USCScoringEvent> ScoreEventClass) const;
};
