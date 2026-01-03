// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Components/SkeletalMeshComponent.h"
#include "SCCharacterBodyPartComponent.generated.h"

struct FSCCounselorOutfit;

/**
 * @enum ELimb
 */
UENUM()
enum class ELimb : uint8
{
	HEAD		UMETA(DisplayName = "Head"),
	TORSO		UMETA(DisplayName = "Torso"),
	LEFT_ARM	UMETA(DisplayName = "Left Arm"),
	RIGHT_ARM	UMETA(DisplayName = "Right Arm"),
	LEFT_LEG	UMETA(DisplayName = "Left Leg"),
	RIGHT_LEG	UMETA(DisplayName = "Right Leg"),

	MAX			UMETA(Hidden)
};

/**
 * @struct FSCWeaponDamageGoreInfo
 */
USTRUCT()
struct FSCWeaponDamageGoreInfo
{
	GENERATED_USTRUCT_BODY()
	
	// The weapon type linked to the blood mask
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class ASCWeapon> WeaponType;

	// Texture that maps where blood should be
	UPROPERTY(EditDefaultsOnly)
	TAssetPtr<UTexture> BloodMask;
};

/**
 * @struct FSCWeaponDamageGoreInfo
 */
USTRUCT()
struct FSCLimbMeshSwapInfo
{
	GENERATED_USTRUCT_BODY()
	
	// The name of the swap
	UPROPERTY(EditDefaultsOnly)
	FName SwapName;

	// The mesh to swap to
	UPROPERTY(EditDefaultsOnly)
	TAssetPtr<USkeletalMesh> SwapMesh;
};

/**
 * @class USCCharacterBodyPartComponent
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SUMMERCAMP_API USCCharacterBodyPartComponent 
: public USkeletalMeshComponent
{
	GENERATED_UCLASS_BODY()

public:
	// Begin UActorComponent interface
	virtual void InitializeComponent() override;
	// End UActorComponent interface

	/** Hides the parent limb and shows the body part, optionally switching the mesh with the one passed in */
	UFUNCTION(BlueprintCallable, Category = "Limb")
	void ShowLimb(const FName SwapName = NAME_None, TAssetPtr<USkeletalMesh> MeshToSwap = nullptr);
	
	/** Detach the limb */
	UFUNCTION(BlueprintCallable, Category = "Limb")
	void DetachLimb(const FVector& Impulse = FVector::ZeroVector, TAssetPtr<UTexture> BloodMaskOverride = nullptr, const bool bShowLimb = true);

	/** Show blood on the limb */
	UFUNCTION(BlueprintCallable, Category = "Limb")
	void ShowBlood(TAssetPtr<UTexture> BloodMaskOverride = nullptr, FName TextureParameterOverride = NAME_None);

	/** Get gore data for damage from a specific weapon */
	void GetDamageGoreInfo(TSubclassOf<class ASCWeapon> WeaponType, struct FGoreMaskData& DamageGore) const;

	/** Remove blood on the limb */
	void RemoveBlood();

	/** Applys the passed in outfit to this dismemberment limb */
	void ApplyOutfit(const FSCCounselorOutfit* Outfit);

	// Set the limb type
	FORCEINLINE void SetLimbType(ELimb Limb) { LimbType = Limb; }

	// Backup bone name (if no attach socket is found) to hide when detatching this limb
	UPROPERTY(EditDefaultsOnly, Category = "Limb")
	FName BoneToHide;

	// The type of limb this is
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Limb")
	ELimb LimbType;

	// Gore cap attached to this limb
	UPROPERTY(EditDefaultsOnly, Category = "Limb")
	TAssetPtr<UStaticMesh> LimbGoreCap;

	// The socket on the limb the gore cap attaches to
	UPROPERTY(EditDefaultsOnly, Category = "Limb")
	FName LimbGoreCapSocket;

	// Gore cap attached to the parent limb this limb was detached from
	UPROPERTY(EditDefaultsOnly, Category = "Limb")
	TAssetPtr<USkeletalMesh> ParentLimbGoreCap;

	// The socket on the parent limb the gore cap attaches to
	UPROPERTY(EditDefaultsOnly, Category = "Limb")
	FName ParentLimbGoreCapSocket;

	// UV Texture that maps where blood should be
	UPROPERTY(EditDefaultsOnly, Category = "Limb")
	TAssetPtr<UTexture> LimbUVTexture;

	// Array of blood textures linked to weapons, that are applied when damaged from combat
	UPROPERTY(EditDefaultsOnly, Category = "Limb")
	TArray<FSCWeaponDamageGoreInfo> CombatDamageGoreInfo;

	// The name of the texture parameter the LimbUVTexture should be applied to
	UPROPERTY(EditDefaultsOnly, Category = "Limb")
	FName TextureParameterName;

	// The material indices on the limb the LimbUVTexture should be applied to
	UPROPERTY(EditDefaultsOnly, Category = "Limb")
	TArray<int32> LimbMaterialElementIndices;

	// The material indices on the parent limb the LimbUVTexture should be applied to
	UPROPERTY(EditDefaultsOnly, Category = "Limb")
	TArray<int32> ParentMaterialElementIndices;

	// Effect attached to the detached limb
	UPROPERTY(EditDefaultsOnly, Category = "Limb|Particle")
	TAssetPtr<UParticleSystem> LimbBloodEffect;

	// The socket on the limb the emitter attaches to
	UPROPERTY(EditDefaultsOnly, Category = "Limb|Particle")
	FName LimbEmitterSocket;

	// Effect attached to the parent limb this limb was detached from
	UPROPERTY(EditDefaultsOnly, Category = "Limb|Particle")
	TAssetPtr<UParticleSystem> ParentLimbBloodEffect;

	// The socket on the parent limb the emitter attaches to
	UPROPERTY(EditDefaultsOnly, Category = "Limb|Particle")
	FName ParentLimbEmitterSocket;

	// List of mesh swaps possible for different kills
	UPROPERTY(EditDefaultsOnly, Category = "Limb")
	TArray<FSCLimbMeshSwapInfo> MeshSwaps;

	/** Hide the gore cap for debugging limb dismemberment */
	void HideLimbGoreCaps();

protected:
	/** Callback for when the mesh was skeletal mesh has streamed in */
	void DeferredShowLimb();

	/** Callback for when LimbGoreCap has streamed in to play it. */
	void DeferredShowLimbGoreCap();

	/** Callback for when ParentLimbGoreCap has streamed in to play it. */
	void DeferredShowParentLimbGoreCap();

	// Skeletal mesh this limb is/was attached to
	UPROPERTY()
	USkeletalMeshComponent* ParentSkeletalMesh;

	// Skeletal mesh for the clothing attached to this limb
	UPROPERTY(Transient)
	USkeletalMeshComponent* ClothingMesh;

	/** Callback for when LimbBloodEffect has streamed in to play it. */
	void DeferredPlayLimbBloodEffect();

	/** Callback for when ParentLimbBloodEffect has streamed in to play it. */
	void DeferredPlayParentLimbBloodEffect();

	/** Callback for when PendingBloodMask has streamed in. */
	void DeferredApplyBloodMask();

	// Blood mask pending load in DeferredApplBloodMask
	TAssetPtr<UTexture> PendingBloodMask;
	FName PendingBloodParameter;

	// SkeletalMesh pending load in DeferredShowLimb
	TAssetPtr<USkeletalMesh> PendingMeshSwap;

	// Spawned component instances
	UPROPERTY(Transient)
	UStaticMeshComponent* LimbGoreCapComponent;
	UPROPERTY(Transient)
	USkeletalMeshComponent* ParentLimbGoreCapComponent;
};
