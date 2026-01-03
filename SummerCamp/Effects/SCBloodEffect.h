// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "SCBloodEffect.generated.h"

/**
* @class ASCBloodEffect
*/
UCLASS()
class SUMMERCAMP_API ASCBloodEffect 
	: public AActor
{
	GENERATED_UCLASS_BODY()
public:

	// Begin AActor interface
	virtual void Tick(float DeltaSeconds) override;
	// End AActor interface

	// Begin UObject interface
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// End UObject interface

	UFUNCTION(BlueprintCallable, Category = "Decal")
	void SpawnEffect();

	/* The blood particle effect to play on spawn. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Blood")
	TAssetPtr<UParticleSystem> Particle;

	/* The blood decal to be placed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Decal")
	UMaterialInterface* DecalMaterial;

	/* The direction the decal should attempt to spawn towards. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UArrowComponent* DecalSpawnDirection;

	/* The max distance the decal should attempt to be placed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Decal")
	float RaytraceDistance;

	/* The size of the decal when spawned. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Decal")
	float DecalSize;

	/* The amount of time the decal should be shown before being deleted. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Decal")
	float DecalLifetime;

	/* If the decal should scale after spawning. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Decal")
	bool bShouldDecalScale;

	/* The amount of time the decal should scale for. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = bShouldDecalScale), Category = "Decal")
	float DecalScaleTime;

	/* The size of the decal after the DecalScaleTime is reached. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = bShouldDecalScale), Category = "Decal")
	float DecalMaxSize;

	/* How much time after the particle is spawned before the decal is spawned. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Decal")
	float DecalSpawnDelay;

protected:
	/** Callback for when Particle has streamed in to play it. */
	void DeferredPlayParticle();

	UPROPERTY(Transient)
	UDecalComponent* Decal;

	UPROPERTY(BlueprintReadWrite, Category = "Decal")
	class ASCCharacter* IgnoreActor;

	FTimerHandle TimerHandle_SpawnDecal;

	void SpawnDecal();

private:
	float CurrentScaleTime;
	FVector DecalForward;
	FVector DecalRight;
};
