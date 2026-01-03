// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCImpactEffects.generated.h"

class ASCWeapon;

/**
 * @struct FSplashDecalData
 */
USTRUCT()
struct FSplashDecalData
{
	GENERATED_USTRUCT_BODY()
	
	FSplashDecalData() 
	: bApplyRandomRotation(true)
	, DecalSize(5.f, 32.f, 32.f)
	, LifeSpan(10.f)
	, FadeScreenSize(0.01f) {}

	// Array of decals that can be spawned. One decal from the array will be chosen
	UPROPERTY(EditDefaultsOnly, Category = "Decal")
	TArray<UMaterialInterface*> Decals;

	// Apply random roll rotation to the decal
	UPROPERTY(EditDefaultsOnly, Category = "Decal")
	bool bApplyRandomRotation;

	// Decal size
	UPROPERTY(EditDefaultsOnly, Category = "Decal")
	FVector DecalSize;

	// Lifespan
	UPROPERTY(EditDefaultsOnly, Category = "Decal")
	float LifeSpan;

	// FadeScreenSize setting for decals spawned
	UPROPERTY(EditDefaultsOnly, Category = "Decal")
	float FadeScreenSize;

	// Direction to test
	UPROPERTY(EditDefaultsOnly, Category = "RayCast")
	FVector RayDirection;

	// Length of the ray
	UPROPERTY(EditDefaultsOnly, Category = "RayCast")
	float RayLength;

	// Should the ray trace use the instigator's forward vector
	UPROPERTY(EditDefaultsOnly, Category = "RayCast")
	bool bUseInstigatorForwardVector;
};

/**
 * @struct FDecalData
 */
USTRUCT()
struct FDecalData
{
	GENERATED_USTRUCT_BODY()
	
	FDecalData()
	: bApplyRandomRotation(true)
	, DecalSize(5.f, 32.f, 32.f)
	, LifeSpan(10.f)
	, FadeScreenSize(0.01f) {}

	// Material
	UPROPERTY(EditDefaultsOnly, Category = "Decal")
	UMaterialInterface* DecalMaterial;

	// Apply random roll rotation to the decal
	UPROPERTY(EditDefaultsOnly, Category = "Decal")
	bool bApplyRandomRotation;

	// Decal size
	UPROPERTY(EditDefaultsOnly, Category = "Decal")
	FVector DecalSize;

	// Lifespan
	UPROPERTY(EditDefaultsOnly, Category = "Decal")
	float LifeSpan;

	// FadeScreenSize setting for decals spawned
	UPROPERTY(EditDefaultsOnly, Category = "Decal")
	float FadeScreenSize;

	// If true, once a raytest passes, only that decal will be spawned
	UPROPERTY(EditDefaultsOnly, Category = "SplashDecals")
	bool bSingleSplashDecal;

	// Additional decals to spawn around this decal
	UPROPERTY(EditDefaultsOnly, Category = "SplashDecals")
	TArray<FSplashDecalData> SplashDecals;
};

/**
 * @class USCImpactEffect
 *
 * Defines Particle System, Sound, and Decal to use per surface type
 */
UCLASS(Blueprintable)
class SUMMERCAMP_API USCImpactEffect
: public UDataAsset
{
	GENERATED_UCLASS_BODY()

private:
	// Begin UObject interface
	virtual UWorld* GetWorld() const override;
	// End UObject interface

public:
	void SpawnFX(const FHitResult& Hit, const AActor* Instigator, const TSubclassOf<ASCWeapon> WeaponClass, const bool bIsHeavyImpact);

private:
	// Location of the weapon impact
	FVector ImpactLocation;

	// Rotation of the weapon impact
	FRotator ImpactRotation;

	//////////////////////////////////////////////////////////////////////////
	// Particle effects
protected:
	/** @return FX for SurfaceType */
	TAssetPtr<UParticleSystem> GetParticleSystem(TEnumAsByte<EPhysicalSurface> SurfaceType, AActor* HitActor) const;

	/** Callback for when an impact effect has streamed in to play it. */
	void DeferredSpawnParticleSystem();

	// Default impact FX used when material specific override doesn't exist
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TAssetPtr<UParticleSystem> DefaultFX;

	// Counselor impact FX
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TAssetPtr<UParticleSystem> CounselorFX;

	// Jason impact FX
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TAssetPtr<UParticleSystem> JasonFX;

	// WoodHeavy impact FX
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TAssetPtr<UParticleSystem> WoodHeavyFX;

	// WoodMedium impact FX
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TAssetPtr<UParticleSystem> WoodMediumFX;

	// WoodLight impact FX
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TAssetPtr<UParticleSystem> WoodLightFX;

	// WoodHollow impact FX
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TAssetPtr<UParticleSystem> WoodHollowFX;

	// Sand impact FX
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TAssetPtr<UParticleSystem> SandFX;

	// Gravel impact FX
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TAssetPtr<UParticleSystem> GravelFX;

	// DirtDry impact FX
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TAssetPtr<UParticleSystem> DirtDryFX;

	// DirtWet impact FX
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TAssetPtr<UParticleSystem> DirtWetFX;

	// Mud impact FX
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TAssetPtr<UParticleSystem> MudFX;

	// Grass impact FX
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TAssetPtr<UParticleSystem> GrassFX;

	// LeafLitter impact FX
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TAssetPtr<UParticleSystem> LeafLitterFX;

	// WaterDeep impact FX
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TAssetPtr<UParticleSystem> WaterDeepFX;

	// WaterShallow impact FX
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TAssetPtr<UParticleSystem> WaterShallowFX;

	// MetalThick impact FX
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TAssetPtr<UParticleSystem> MetalThickFX;

	// MetalMedium impact FX
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TAssetPtr<UParticleSystem> MetalMediumFX;

	// MetalLight impact FX
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TAssetPtr<UParticleSystem> MetalLightFX;

	// MetalHollow impact FX
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TAssetPtr<UParticleSystem> MetalHollowFX;

	// Rock impact FX
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TAssetPtr<UParticleSystem> RockFX;

	// LargeFoliage impact FX
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TAssetPtr<UParticleSystem> LargeFoliageFX;

	// SmallFoliage impact FX
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TAssetPtr<UParticleSystem> SmallFoliageFX;

	// Cloth impact FX
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TAssetPtr<UParticleSystem> ClothFX;

	// WoodDoor impact FX
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TAssetPtr<UParticleSystem> WoodDoorFX;

	// Impact effect that will play when it's streamed in
	TAssetPtr<UParticleSystem> PendingParticleSystem;

	//////////////////////////////////////////////////////////////////////////
	// SoundCues
protected:
	/** @return Sound for SurfaceType */
	TAssetPtr<USoundCue> GetSoundCue(TEnumAsByte<EPhysicalSurface> SurfaceType, AActor* HitActor, bool HeavyImpact) const;

	/** Callback for when an impact sound has streamed in to play it. */
	void DeferredPlaySoundCue();

	// Default impact Sound used when material specific override doesn't exist
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TAssetPtr<USoundCue> DefaultSound;

	// Default impact Sound used when material specific override doesn't exist and it was a heavy attack
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TAssetPtr<USoundCue> DefaultSoundHeavy;

	// Impact sound if character hit is blocking
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TAssetPtr<USoundCue> BlockingSound;

	// WoodHeavy impact Sound
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TAssetPtr<USoundCue> WoodHeavySound;

	// WoodMedium impact Sound
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TAssetPtr<USoundCue> WoodMediumSound;

	// WoodLight impact Sound
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TAssetPtr<USoundCue> WoodLightSound;

	// WoodHollow impact Sound
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TAssetPtr<USoundCue> WoodHollowSound;

	// Sand impact Sound
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TAssetPtr<USoundCue> SandSound;

	// Gravel impact Sound
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TAssetPtr<USoundCue> GravelSound;

	// DirtDry impact Sound
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TAssetPtr<USoundCue> DirtDrySound;

	// DirtWet impact Sound
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TAssetPtr<USoundCue> DirtWetSound;

	// Mud impact Sound
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TAssetPtr<USoundCue> MudSound;

	// Grass impact Sound
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TAssetPtr<USoundCue> GrassSound;

	// LeafLitter impact Sound
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TAssetPtr<USoundCue> LeafLitterSound;

	// WaterDeep impact Sound
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TAssetPtr<USoundCue> WaterDeepSound;

	// WaterShallow impact Sound
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TAssetPtr<USoundCue> WaterShallowSound;

	// MetalThick impact Sound
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TAssetPtr<USoundCue> MetalThickSound;

	// MetalMedium impact Sound
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TAssetPtr<USoundCue> MetalMediumSound;

	// MetalLight impact Sound
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TAssetPtr<USoundCue> MetalLightSound;

	// MetalHollow impact Sound
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TAssetPtr<USoundCue> MetalHollowSound;

	// Rock impact Sound
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TAssetPtr<USoundCue> RockSound;

	// LargeFoliage impact Sound
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TAssetPtr<USoundCue> LargeFoliageSound;

	// SmallFoliage impact Sound
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TAssetPtr<USoundCue> SmallFoliageSound;

	// Cloth impact Sound
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TAssetPtr<USoundCue> ClothSound;

	// WoodDoor impact Sound
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TAssetPtr<USoundCue> WoodDoorSound;

	// Impact sound that will play when it's streamed in
	TAssetPtr<USoundCue> PendingSoundCue;

	//////////////////////////////////////////////////////////////////////////
	// Decals
protected:
	/** @return Sound for SurfaceType */
	const FDecalData& GetDecal(const TSubclassOf<ASCWeapon> WeaponClass, TEnumAsByte<EPhysicalSurface> SurfaceType, const FHitResult& Hit) const;

	// Default impact decal used when material specific override doesn't exist
	UPROPERTY(EditDefaultsOnly, Category = "Decals")
	FDecalData DefaultDecal;

	// Counselor impact decal
	UPROPERTY(EditDefaultsOnly, Category = "Decals")
	FDecalData CounselorDecal;

	// Jason impact decal
	UPROPERTY(EditDefaultsOnly, Category = "Decals")
	FDecalData JasonDecal;

	// WoodHeavy impact decal
	UPROPERTY(EditDefaultsOnly, Category = "Decals")
	FDecalData WoodHeavyDecal;

	// WoodMedium impact decal
	UPROPERTY(EditDefaultsOnly, Category = "Decals")
	FDecalData WoodMediumDecal;

	// WoodLight impact decal
	UPROPERTY(EditDefaultsOnly, Category = "Decals")
	FDecalData WoodLightDecal;

	// WoodHollow impact decal
	UPROPERTY(EditDefaultsOnly, Category = "Decals")
	FDecalData WoodHollowDecal;

	// Sand impact decal
	UPROPERTY(EditDefaultsOnly, Category = "Decals")
	FDecalData SandDecal;

	// Gravel impact decal
	UPROPERTY(EditDefaultsOnly, Category = "Decals")
	FDecalData GravelDecal;

	// DirtDry impact decal
	UPROPERTY(EditDefaultsOnly, Category = "Decals")
	FDecalData DirtDryDecal;

	// DirtWet impact decal
	UPROPERTY(EditDefaultsOnly, Category = "Decals")
	FDecalData DirtWetDecal;

	// Mud impact decal
	UPROPERTY(EditDefaultsOnly, Category = "Decals")
	FDecalData MudDecal;

	// Grass impact decal
	UPROPERTY(EditDefaultsOnly, Category = "Decals")
	FDecalData GrassDecal;

	// LeafLitter impact decal
	UPROPERTY(EditDefaultsOnly, Category = "Decals")
	FDecalData LeafLitterDecal;

	// WaterDeep impact decal
	UPROPERTY(EditDefaultsOnly, Category = "Decals")
	FDecalData WaterDeepDecal;

	// WaterShallow impact decal
	UPROPERTY(EditDefaultsOnly, Category = "Decals")
	FDecalData WaterShallowDecal;

	// MetalThick impact decal
	UPROPERTY(EditDefaultsOnly, Category = "Decals")
	FDecalData MetalThickDecal;

	// MetalMedium impact decal
	UPROPERTY(EditDefaultsOnly, Category = "Decals")
	FDecalData MetalMediumDecal;

	// MetalLight impact decal
	UPROPERTY(EditDefaultsOnly, Category = "Decals")
	FDecalData MetalLightDecal;

	// MetalHollow impact decal
	UPROPERTY(EditDefaultsOnly, Category = "Decals")
	FDecalData MetalHollowDecal;

	// Rock impact decal
	UPROPERTY(EditDefaultsOnly, Category = "Decals")
	FDecalData RockDecal;

	// LargeFoliage impact decal
	UPROPERTY(EditDefaultsOnly, Category = "Decals")
	FDecalData LargeFoliageDecal;

	// SmallFoliage impact decal
	UPROPERTY(EditDefaultsOnly, Category = "Decals")
	FDecalData SmallFoliageDecal;

	// Cloth impact decal
	UPROPERTY(EditDefaultsOnly, Category = "Decals")
	FDecalData ClothDecal;

	// WoodDoor impact decal
	UPROPERTY(EditDefaultsOnly, Category = "Decals")
	FDecalData WoodDoorDecal;
};
