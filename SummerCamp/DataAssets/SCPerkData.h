// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "StatModifiedValue.h"

#include "SCPerkData.generated.h"

class ASCCharacter;
class ASCItem;
class ASCWeapon;
class USCPerkData;

/**
 * @struct FSCPerkModifier
 */
USTRUCT(Blueprintable, BlueprintType)
struct FSCPerkModifier
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier")
	FString ModifierVariableName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier")
	int32 ModifierPercentage;

	FSCPerkModifier()
	: ModifierPercentage(0) {}
};

/**
 * @struct FSCPerkBackendData
 */
USTRUCT(Blueprintable, BlueprintType)
struct FSCPerkBackendData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perk Backend Data")
	TSubclassOf<USCPerkData> PerkClass;
	
	/** Positive Perk modifiers as set from the backend. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perk Backend Data")
	FSCPerkModifier PositivePerkModifiers;

	/** Negative Perk modifiers as set from the backend. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perk Backend Data")
	FSCPerkModifier NegativePerkModifiers;

	/** Legendary Perk modifiers as set from the backend. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perk Backend Data")
	FSCPerkModifier LegendaryPerkModifiers;

	/** Amount of CP you get back when selling this Perk. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perk Backend Data")
	int32 BuyBackCP;

	/** Amount of bonus CP you get back when selling this Perk (Only for legendary perks). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perk Backend Data")
	int32 CashInBonusCP;

	/** Instance ID used for Cashing In Perk. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perk Backend Data")
	FString InstanceID;

	FSCPerkBackendData()
	: PerkClass(nullptr)
	, BuyBackCP(0)
	, CashInBonusCP(0)
	{}

	FORCEINLINE bool operator ==(const FSCPerkBackendData& other) const
	{
		return InstanceID == other.InstanceID;
	}
};

/**
 * @class USCPerkData
 */
UCLASS(Blueprintable, NotPlaceable)
class SUMMERCAMP_API USCPerkData
: public UDataAsset
{
	GENERATED_BODY()

public:
	/** Localized name of the perk */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default")
	FText PerkName;

	/** Localized description of the perk */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default")
	FText Description;

	/** Localized description of the positive perk modifiers */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default")
	FText PositiveDescription;

	/** Localized description of the negative perk modifiers */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default")
	FText NegativeDescription;

	/** Localized description of the legendary perk modifiers */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default")
	FText LegendaryDescription;

	/** Loadout icon */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UTexture2D* LoadoutIcon;

	UPROPERTY(EditDefaultsOnly, Category = "Default", meta = (UIMin = "-10.0", UIMax = "10.0"))
	float StatModifiers[(uint8)ECounselorStats::MAX];

	//////////////////////////////////////////////////
	// Fear

	/** Chance to avoid fear causing a stumble */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear", meta = (UIMin = "0.0", UIMax = "1.0"))
	float AvoidFearStumble;

	/** Fear modifier based on number of counselors around */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float CounselorsAroundFearModifier;

	/** Modifier for fear from darkness */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear", meta = (UIMin = "0.0", UIMax = "1.0"))
	float DarknessFearModifier;

	/** Modifier for fear when spotting a dead body */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float DeadBodyFearModifier;

	/** Modifier for fear */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float FearModifier;

	/** Fear modifier while in hiding spots */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear", meta = (UIMin = "0.0", UIMax = "1.0"))
	float HidespotFearModifier;

	// ~Fear
	//////////////////////////////////////////////////

	//////////////////////////////////////////////////
	// Items

	/** Extra uses for First Aid Spray */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Items", meta = (UIMin = "0.0"))
	int32 ExtraFirstAidUses;

	/** Increases the healing effect of the First Aid Spray */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Items", meta = (UIMin = "0.0"))
	float FirstAidHealingModifier;

	/** Firecracker stun radius modifier */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Items", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float FirecrackerStunRadiusModifier;

	/** Increases the duration of a flare projectile */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Items")
	float FlareLifeSpanIncrease;

	/** Stamina recharge modifier applied when within range of a radio playing music */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Items", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float RadioStaminaModifier;

	/** Items to start a match with */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Items")
	TArray<TSubclassOf<ASCItem>> StartingItems;

	/** Modifier for how long a trap stun lasts */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Items", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float TrapStunTimeModifier;

	// ~Items
	//////////////////////////////////////////////////

	//////////////////////////////////////////////////
	// Jason

	/** Chance to avoid Jason's Sense ability */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jason", meta = (UIMin = "0.0", UIMax = "1.0"))
	float AvoidSenseChance;

	/** Chance to avoid Jason's Sense ability while indoors */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jason", meta = (UIMin = "0.0", UIMax = "1.0"))
	float AvoidSenseIndoorsChance;

	/** Chance to avoid Jason's Sense ability while in a sleeping bag */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jason", meta = (UIMin = "0.0", UIMax = "1.0"))
	float AvoidSenseInSleepingBagChance;

	/** Modifies the duration of the break grab mini game */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jason", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float BreakGrabDurationModifier;

	/** Modifies the counselors current stamina when breaking free from Jason's grab */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jason", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float BreakGrabStaminaModifier;

	// ~Jason
	//////////////////////////////////////////////////

	//////////////////////////////////////////////////
	// Movement

	/** Modifies attack speed */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float AttackSpeedModifier;

	/** Modifies the speed of barricading a door */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float BarricadeSpeedModifier;

	/** Modifies Crouch movement speed */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float CrouchMoveSpeedModifier;

	/** Stamina regen modifier when crouched */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float CrouchingStaminaRegenModifier;

	/** Modifies speed of dodging in combat */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float DodgeSpeedModifier;

	/** Noise modifier */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float NoiseModifier;

	/** Modifier for noise while sprinting */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (UIMin = "0.0", UIMax = "1.0"))
	float SprintNoiseModifier;

	/** Sprint speed modifier */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float SprintSpeedModifier;

	/** Modifies stamina drain */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float StaminaDrainModifier;

	/** Stamina regen modifier when standing */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float StandingStaminaRegenModifier;

	/** Modifies window and hidespot interaction speed */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float WindowHidespotInteractionSpeedModifier;

	// ~Movement
	//////////////////////////////////////////////////

	//////////////////////////////////////////////////
	// Police

	/** Modifies police response time */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Police", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float PoliceResponseTimeModifier;

	// ~Police
	//////////////////////////////////////////////////

	//////////////////////////////////////////////////
	// Repair

	/** Modifies repair speed */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Repair", meta = (UIMin = "-0.9"))
	float RepairSpeedModifier;

	// ~Repair
	//////////////////////////////////////////////////

	//////////////////////////////////////////////////
	// Swimming

	/** Modifies swim speed */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Swimming")
	float SwimSpeedModifier;

	/** Chance to avoid Jason's Sense ability while swimming */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Swimming", meta = (UIMin = "0.0", UIMax = "1.0"))
	float SwimmingAvoidSenseChance;

	/** Noise modifier for swimming */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Swimming", meta = (UIMin = "0.0", UIMax = "1.0"))
	float SwimmingNoiseModifier;

	/** Modifies stamina drain while simming */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Swimming")
	float SwimmingStaminaDrainModifier;

	// ~Swimming
	//////////////////////////////////////////////////

	//////////////////////////////////////////////////
	// Vehicle

	/** Modifies car movement speed */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float BoatSpeedModifier;

	/** Modifies Vehicle movement speed */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle", meta = (UIMin = "-99", UIMax = "100"))
	float BoatStartTimeModifier;

	/** Modifies car movement speed */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float CarSpeedModifier;

	/** Modifies Vehicle movement speed */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle", meta = (UIMin = "-99", UIMax = "100"))
	float CarStartTimeModifier;

	/** Modifies Vehicle movement speed when escaping alone */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float SoloEscapeVehicleSpeedModifier;

	// ~Vehicle
	//////////////////////////////////////////////////

	//////////////////////////////////////////////////
	// VoIP

	/** Modifies 3D VoIP attenuation distance */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VoIP", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float VoIPDistanceModifier;

	// ~VoIP
	//////////////////////////////////////////////////

	//////////////////////////////////////////////////
	// XP
	
	/** XP modifier */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XP", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float XPModifier;

	// ~XP
	//////////////////////////////////////////////////

	//////////////////////////////////////////////////
	// Weapons

	/** Modifies damage from weapons when playing as Tommy Jarvis */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapons", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float TommyJarvisWeaponDamageModifier;

	/** Modifies damage from weapons */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapons", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float WeaponDamageModifier;

	/** Modifies damage taken from weapons */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapons", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float WeaponDamageTakenModifier;

	/** Modifies Weapon durability */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapons", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float WeaponDurabilityModifier;

	/** Modifies weapon stun chance */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapons", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float WeaponStunChanceModifier;

	/** Modifies weapon stun duration */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapons", meta = (UIMin = "-0.9", UIMax = "1.0"))
	float WeaponStunDurationModifier;

	/** Weapon required to apply weapon modifiers */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapons")
	TSoftClassPtr<ASCWeapon> RequiredWeapon;

	/** Required character class to hit in order to apply weapon modifiers */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapons")
	TSoftClassPtr<ASCCharacter> RequiredCharacterClassToHit;

	// ~Weapons
	//////////////////////////////////////////////////

	/**
	 * Creates a perk instance based off data from the backend
	 * @param PerkData Backend data about the perk to be created
	 * @return New perk instance with updated perk property values
	 */
	static USCPerkData* CreatePerkFromBackendData(const FSCPerkBackendData& PerkData);
};
