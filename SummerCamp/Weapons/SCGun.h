// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Weapons/SCWeapon.h"
#include "SCGun.generated.h"

class AILLCharacter;

class USCImpactEffect;
class USCScoringEvent;

/**
 * @enum ESCGunType
 * @brief Allows animation to pick the correct stances for a given weapon
 */
UENUM(BlueprintType)
enum class ESCGunType : uint8
{
	Flare,
	HunterShotgun,
	GruntRifle
};

/**
 * @class ASCGun
 */
UCLASS()
class SUMMERCAMP_API ASCGun
: public ASCWeapon
{
	GENERATED_BODY()

public:
	ASCGun(const FObjectInitializer& ObjectInitializer);

	// BEGIN UObject interface
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// END UObject interface

	// Begin ASCItem interface
	virtual void EnableInteraction(bool bEnable) override;
	virtual void SetShimmerEnabled(const bool bEnabled) override;
	virtual bool Use(bool bFromInput) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UAnimSequenceBase* GetIdlePose(const USkeleton* Skeleton) const override;
	virtual int32 CanInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation) override;
	virtual void OnDropped(ASCCharacter* DroppingCharacter, FVector AtLocation = FVector::ZeroVector) override;
	// End ASCItem interface

	// Weapon type for animation
	UPROPERTY(EditDefaultsOnly)
	ESCGunType GunType;

	// Max spread angle for pellets in degrees.
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	float MaxSpreadAngle;

	UPROPERTY(EditDefaultsOnly, Category = "Default")
	uint8 NumPellets;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USceneComponent* Muzzle;

	// Actor hit, and an array of hit locations for decals and tracking number of hits.
	TMap<AActor*, TArray<FVector>> HitActors;

	UPROPERTY(Replicated)
	bool bHasAmmunition;

	/** pointer to the muzzle flash particle instantiated so we only play it once. */
	UParticleSystemComponent* MuzzleFlashParicles;

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_SpawnRibbon(const FVector& ImpactPoint);

	FTimerHandle TimerHandle_MeleeSwap;

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_SpawnPelletImpact(const FHitResult& Impact);

	UPROPERTY(EditDefaultsOnly, Category = "Default")
	UParticleSystem* PelletRibbon;

	UPROPERTY(EditDefaultsOnly, Category = "Default")
	UParticleSystem* MuzzleFlash;

	// Pellet impact effect
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TSubclassOf<USCImpactEffect> PelletImpactTemplate;

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_PlayGunAnim();

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_AimGun(bool Aiming);

protected:
	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsAiming;

	// Idle animation to play while the shotgun is loaded
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimSequenceBase* LoadedIdle;

	// The projectile class we should fire for each "pellet"
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class ASCProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly)
	float ProjectileLifespan;

	UPROPERTY(EditDefaultsOnly)
	float ProjectileVelocity;

	UPROPERTY(EditDefaultsOnly)
	bool ProjectileFireOnly;

	// How much of the accumulated damage (damage from each pellet) do we want to apply to a counselor who is shot? Default = 200, Kill 'em
	UPROPERTY(EditDefaultsOnly)
	float CounselorDamageMultiplier;

	UFUNCTION()
	void FireProjectile(class ASCCharacter* Character, FVector InitialVelocity);

public:
	UFUNCTION()
	void AimGun(bool Aiming);

	UFUNCTION()
	bool IsAiming() const { return bIsAiming; };

	UFUNCTION()
	bool HasAmmunition() const { return bHasAmmunition; }

	UFUNCTION()
	void Fire();

	UPROPERTY(EditDefaultsOnly, Category = "Default")
	float FireDistance;

	UPROPERTY(EditDefaultsOnly, Category = "Default")
	float FireImpulse;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* FireMontage;

	// Score event for stunning the killer with a flare
	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	TSubclassOf<USCScoringEvent> StunScoreEventClass;

	void SetAimingInfo(FVector CameraLocation, FRotator CameraForward) { CachedCameraLocation = CameraLocation; CachedCameraForward = CameraForward; }

protected:
	FVector CachedCameraLocation;
	FRotator CachedCameraForward;
};
