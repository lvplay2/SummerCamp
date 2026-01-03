// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCTypes.generated.h"

class ASCWeapon;

class USCSpecialMove_SoloMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnActivateDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeactivateDelegate);

#define ECC_MorphBlockers ECC_GameTraceChannel1
#define ECC_DynamicContextBlocker ECC_GameTraceChannel2
#define ECC_Killer ECC_GameTraceChannel3
#define ECC_PowerdLight ECC_GameTraceChannel4
#define ECC_MorphPoint ECC_GameTraceChannel5
#define ECC_AnimCameraBlocker ECC_GameTraceChannel7
#define ECC_Foliage ECC_GameTraceChannel8
#define ECC_Boat ECC_GameTraceChannel9
#define ECC_Escape ECC_GameTraceChannel10

/**
 * @struct FAttackMontage
 */
USTRUCT(BlueprintType)
struct FAttackMontage
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FAttackMontage")
	UAnimMontage* AttackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FAttackMontage")
	UAnimMontage* RecoilMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FAttackMontage")
	UAnimMontage* ImpactMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FAttackMontage")
	bool bIsFemaleMontages;

	FAttackMontage()
		: AttackMontage(nullptr)
		, RecoilMontage(nullptr)
		, ImpactMontage(nullptr)
		, bIsFemaleMontages(false)
	{
	}

	FAttackMontage(const FAttackMontage& Other)
		: AttackMontage(Other.AttackMontage)
		, RecoilMontage(Other.RecoilMontage)
		, ImpactMontage(Other.ImpactMontage)
		, bIsFemaleMontages(Other.bIsFemaleMontages)
	{
	}

	FAttackMontage& operator=(const FAttackMontage& Other)
	{
		if (this != &Other)
		{
			AttackMontage = Other.AttackMontage;
			RecoilMontage = Other.RecoilMontage;
			ImpactMontage = Other.ImpactMontage;
			bIsFemaleMontages = Other.bIsFemaleMontages;
		}

		return *this;
	}
};

/**
 * @enum EKillerAbilities
 */
UENUM(BlueprintType)
enum class EKillerAbilities : uint8
{
	Morph	UMETA(DisplayName = "Morph"),
	Sense	UMETA(DisplayName = "Sense"),
	Shift	UMETA(DisplayName = "Shift"),
	Stalk	UMETA(DisplayName = "Stalk"),

	MAX		UMETA(Hidden)
};

/**
 * @enum ESCCharacterHitDirection
 */
UENUM(BlueprintType)
enum class ESCCharacterHitDirection : uint8
{
	Front,
	Back,
	Left,
	Right,

	MAX UMETA(Hidden)
};

/**
 * @enum ESCPhysMaterialType
 * @brief Keep in sync with SCImpactEffect!
 */
UENUM()
namespace ESCPhysMaterialType
{
	enum Type
	{
		WoodHeavy,
		WoodMedium,
		WoodLight,
		WoodHollow,
		Sand,
		Wood,
		Gravel,
		DirtDry,
		DirtWet,
		Mud,
		Grass,
		LeafLitter,
		WaterDeep,
		WaterShallow,
		MetalThick,
		MetalMedium,
		MetalLight,
		MetalHollow,
		Rock,
		LargeFoliage,
		SmallFoliage,
		WoodDoor,
	};
}

#define SC_SURFACE_Default		SurfaceType_Default
#define SC_SURFACE_WoodHeavy	SurfaceType1
#define SC_SURFACE_WoodMedium	SurfaceType2
#define SC_SURFACE_WoodLight	SurfaceType3
#define SC_SURFACE_WoodHollow	SurfaceType4
#define SC_SURFACE_Sand			SurfaceType5
#define SC_SURFACE_Gravel		SurfaceType6
#define SC_SURFACE_DirtDry		SurfaceType7
#define SC_SURFACE_DirtWet		SurfaceType8
#define SC_SURFACE_Mud			SurfaceType9
#define SC_SURFACE_Grass		SurfaceType10
#define SC_SURFACE_LeafLitter	SurfaceType11
#define SC_SURFACE_WaterDeep	SurfaceType12
#define SC_SURFACE_WaterShallow	SurfaceType13
#define SC_SURFACE_MetalThick	SurfaceType14
#define SC_SURFACE_MetalMedium	SurfaceType15
#define SC_SURFACE_MetalLight	SurfaceType16
#define SC_SURFACE_MetalHollow	SurfaceType17
#define SC_SURFACE_Rock			SurfaceType18
#define SC_SURFACE_LargeFoliage	SurfaceType19
#define SC_SURFACE_SmallFoliage	SurfaceType20
#define SC_SURFACE_Cloth		SurfaceType21
#define SC_SURFACE_WoodDoor		SurfaceType22


/**
 * @struct FWeaponSpecificSpecial
 * @brief Used to track weapon specific special moves
 */
USTRUCT()
struct FWeaponSpecificSpecial
{
	GENERATED_USTRUCT_BODY()

	/** Weapon specific special move. */
	UPROPERTY(EditAnywhere)
	TSubclassOf<USCSpecialMove_SoloMontage> SpecialMove;

	/** Weapon class this special move is to be played with. */
	UPROPERTY(EditAnywhere)
	TSubclassOf<ASCWeapon> WeaponClass;

	/** A list of camera names this particular special move should use. */
	UPROPERTY(EditAnywhere)
	TArray<FName> KillCameraNames;
};
