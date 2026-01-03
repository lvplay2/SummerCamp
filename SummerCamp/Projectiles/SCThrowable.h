// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCProjectile.h"
#include "SCWeapon.h"
#include "SCThrowable.generated.h"

class USCImpactEffect;
class USCStatBadge;

/**
 * @class ASCThrowable
 */
UCLASS()
class SUMMERCAMP_API ASCThrowable
: public ASCProjectile
{
	GENERATED_UCLASS_BODY()

public:
	// BEGIN AActor Interface
	virtual void PostInitializeComponents() override;
	// END AActor Interface

	/**
	 * Triggered on component overlap begin
	 * @param Actor overlapping
	 * @param Other component
	 * @param Bone index
	 * @param From sweep
	 * @param Sweep hit results
	 */
	UFUNCTION()
	virtual void OnStop(const FHitResult& HitResult);

	UFUNCTION()
	void Use(AActor* Interactor);

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_Use(AActor* Interactor, FVector_NetQuantize NewVelocity, FVector_NetQuantize LaunchLocation);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_Use(AActor* Interactor, FVector_NetQuantize NewVelocity, FVector_NetQuantize LaunchLocation);

	/** Calculate throw velocity */
	FVector GetThrowVelocity(AActor* Interactor, const bool bAsController);

	// Base weapon stats. Can be further modified by Character Stats
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	FWeaponStats WeaponStats;

	// Static mesh component
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UStaticMeshComponent* Mesh;

	// Impact effect
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	TSubclassOf<USCImpactEffect> ImpactTemplate;

	UPROPERTY(EditDefaultsOnly, Category = "Default")
	UAudioComponent* ThrowAudioComponent;

	// Target to launch projectile at
	UPROPERTY()
	AActor* Target;

	// ASCCharacter reference to player who threw the throwable
	UPROPERTY(BlueprintReadOnly, Category = "Default")
	class ASCCharacter* ThrownBy;

	UPROPERTY(EditDefaultsOnly, Category = "Default")
	float TimeToDestroy;

	UPROPERTY(EditDefaultsOnly, Category = "Default")
	USoundCue* JasonImpactNoise;

	UPROPERTY(EditDefaultsOnly, Category = "Scoring|Badges")
	TSubclassOf<USCStatBadge> HitCounselorBadge;
};
