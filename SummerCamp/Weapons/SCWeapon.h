// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCItem.h"
#include "SCTypes.h"
#include "SCWeapon.generated.h"

class USCImpactEffect;

USTRUCT()
struct FWeaponStats
{
	GENERATED_USTRUCT_BODY()

	FWeaponStats()
	: MaximumStuckAngle(15.0f)
	, MaximumRecoilAngle(35.0f)
	{
	}

	/** The amount of wear this weapon can take. */
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	float Durability;

	/** The amount of wear to apply when attacking. */
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	float AttackingWear;

	/** The amount of wear to apply when blocking. */
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	float BlockingWear;

	/** The amount of damage this weapon does when attacking. */
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	float Damage;

	/** The chance for this weapon to stun [0-100]. */
	UPROPERTY(EditDefaultsOnly, Category = "Default", meta = (ClampMin = 0, UIMin = 0,  ClampMax = 100, UIMax = 100))
	int StunChance;

	/** The chance for this weapon to get stuck while attacking [0-100]. */
	UPROPERTY(EditDefaultsOnly, Category = "Default", meta = (ClampMin = 0, UIMin = 0, ClampMax = 100, UIMax = 100))
	int StuckChance;

	/** Surface type this weapon can get stuck in */
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	TArray<TEnumAsByte<EPhysicalSurface>> StuckSurfaceTypes;

	/** Minimum penetration in a surface for this weapon to get stuck */
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	float MinimumSurfacePenetration;

	/** Maximum angle (in degrees) from the character's forward direction that the weapon can get stuck  */
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	float MaximumStuckAngle;

	/** Maximum angle (in degrees) from the character's forward direction that the weapon can recoil  */
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	float MaximumRecoilAngle;

	/** Blueprint damage type for weapon */
	UPROPERTY(EditDefaultsOnly, Category = "DamageTypes")
	TSubclassOf<UDamageType> WeaponDamageClass;

	/** Blueprint damage type for weapon stun */
	UPROPERTY(EditDefaultsOnly, Category = "DamageTypes")
	TSubclassOf<UDamageType> WeaponStunClass;
};

/**
 * @class ASCWeapon
 */
UCLASS()
class SUMMERCAMP_API ASCWeapon
: public ASCItem
{
	GENERATED_UCLASS_BODY()

public:
	// Begin ASCItem interface
	virtual bool Use(bool bFromInput) override;
	virtual void PostInitializeComponents() override;
	virtual void OnDropped(ASCCharacter* DroppingCharacter, FVector AtLocation = FVector::ZeroVector) override;
	virtual void OwningCharacterChanged(ASCCharacter* OldOwner) override;
	// End ASCItem interface

	/** Performs a sweep to look for appropriate blocking hits against components that require a response (impact, recoil, etc.) */
	void GetSweepInfo(TArray<FHitResult>& OutResults, const FVector& PreviousLocation, const FRotator& PreviousRotation, const TArray<AActor*>& ActorsToIgnore = {}, const TArray<UPrimitiveComponent*>& ComponentsToIgnore = {}) const;

	/** Can this weapon get stuck in the specified surface type */
	bool CanGetStuckIn(const AActor* HitActor, const float PenetrationDepth, const float ImpactAngle, const TEnumAsByte<EPhysicalSurface> SurfaceType) const;

	/** Returns the minimum surface penetration needed for a weapon to get stuck */
	FORCEINLINE float GetMinimumStuckPenetration() const { return WeaponStats.MinimumSurfacePenetration; }

	/** Returns the maximum angle (in degrees) from the character's forward direction that a weapon can get stuck */
	FORCEINLINE float GetMaximumStuckAngle() const { return WeaponStats.MaximumStuckAngle; }

	/** Returns the maximum angle (in degrees) from the character's forward direction that a weapon can recoil */
	FORCEINLINE float GetMaximumRecoilAngle() const { return WeaponStats.MaximumRecoilAngle; }

	/** If true, this weapon takes two hands */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	uint32 bIsTwoHanded : 1;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UCapsuleComponent* KillVolume;

	/** Base weapon stats. Can be further modified by Character Stats */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FWeaponStats WeaponStats;

	/** The top or default broken mesh for this weapon */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Broken")
	TArray<UStaticMesh*> BrokenParts;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Killer")
	TSubclassOf<class ASCContextKillActor> DefaultGrabKill;

	/////////////////////////////////////////////////////////////////////////////
	// Animation
public:

	/** List of assets for all possible users of this item, use GetNeutralAttack() to find a matching asset for your character */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Animation")
	TArray<FAttackMontage> NeutralAttack;

	/** List of assets for all possible users of this item, use GetForwardAttack() to find a matching asset for your character */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Animation")
	TArray<FAttackMontage> ForwardAttack;

	/** Finds the best neutral attack for the passed in skeleton, may return an empty struct if no match is found */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weapon|Animation")
	FORCEINLINE FAttackMontage GetNeutralAttack(const USkeleton* Skeleton, const bool bIsFemale) const { return GetBestAsset(Skeleton, bIsFemale, NeutralAttack); }

	/** Finds the best forward attack for the passed in skeleton, may return an empty struct if no match is found */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weapon|Animation")
	FORCEINLINE FAttackMontage GetForwardAttack(const USkeleton* Skeleton, const bool bIsFemale) const { return GetBestAsset(Skeleton, bIsFemale, ForwardAttack); }

	/** List of assets for male users of this item, use GetCombatIdlePose() to find a matching asset for your character */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Animation")
	TArray<UAnimSequenceBase*> CombatIdleAsset;

	/** List of assets for female users of this item, use GetCombatIdlePose() to find a matching asset for your character */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Animation")
	TArray<UAnimSequenceBase*> FemaleCombatIdleAsset;

	/** Finds the best idle pose for the passed in skeleton, may return None if no match is found */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weapon|Animation")
	FORCEINLINE UAnimSequenceBase* GetCombatIdlePose(const USkeleton* Skeleton, const bool bIsFemale) const 
	{ 
		return ASCItem::GetBestAsset(Skeleton, bIsFemale && FemaleCombatIdleAsset.Num() > 0 ? FemaleCombatIdleAsset : CombatIdleAsset); 
	}

protected:
	/** Handles an array of montages and finds a best fit for the passed in skeleton */
	FAttackMontage GetBestAsset(const USkeleton* Skeleton, const bool bIsFemale, const TArray<FAttackMontage>& MontageArray) const;

	// ~Animation
	/////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////
	// Effects
public:
	UFUNCTION()
	void DestroyWeapon();

	void BreakWeapon();

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_BreakWeapon();

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_BreakWeapon();

	void SpawnHitEffect(const FHitResult& Hit);

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_SpawnHitEffect(const FHitResult& Hit);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_SpawnHitEffect(const FHitResult& Hit);

	void WearDurability(bool FromAttack);

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Stats")
	float CurrentDurability;

	// Impact effect
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TSubclassOf<USCImpactEffect> ImpactEffectClass;

	// ~Effects
	/////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////
	// UI
public:
	FORCEINLINE UTexture2D* GetThumbnailImage() const { return ThumbnailImage; }

protected:
	// Character thumbnail image
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	UTexture2D* ThumbnailImage;

	// ~UI
	/////////////////////////////////////////////////////////////////////////////

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DLC")
	FString DLCEntitlement;
};
