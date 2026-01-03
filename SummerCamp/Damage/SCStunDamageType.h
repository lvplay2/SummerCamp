// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCBaseDamageType.h"
#include "StatModifiedValue.h"
#include "SCTypes.h"
#include "SCStunDamageType.generated.h"

/**
 * @struct FSCStunData
 */
USTRUCT(BlueprintType)
struct FSCStunData
{
	GENERATED_USTRUCT_BODY()

	// Stun time modifier
	UPROPERTY(Transient)
	float StunTimeModifier;

	// Reference to the actor that caused the stun
	UPROPERTY(Transient)
	AActor* StunInstigator;

	FSCStunData()
	: StunTimeModifier(1.f)
	, StunInstigator(nullptr)
	{
	}
};

/**
 * @class USCStunDamageType
 */
UCLASS(Blueprintable, BlueprintType)
class SUMMERCAMP_API USCStunDamageType
: public USCBaseDamageType
{
	GENERATED_UCLASS_BODY()

public:
	// Should the wiggle mini-game be played for this stun
	UPROPERTY(EditDefaultsOnly, Category = "Stun")
	bool bPlayMinigame;

	// Should we have Stun recovery time while playing EndStun?
	UPROPERTY(EditDefaultsOnly, Category = "Stun")
	bool bHasStunRecovery;

	// The total stun time with stat modifiers
	UPROPERTY(EditDefaultsOnly, Category = "Stun")
	FStatModifiedValue TotalStunTime;

	// Percentage of the mini-game to remove per wiggle (independent of stamina)
	UPROPERTY(EditDefaultsOnly, Category = "Stun")
	FStatModifiedValue PercentPerWiggle;

	// The amount of stamina that will be used per wiggle, smaller numbers mean more wigging will be necessary
	UPROPERTY(EditDefaultsOnly, Category = "Stun")
	FStatModifiedValue StaminaPerWiggle;

	// The base rate of mini-game decay if the user is not actively playing the mini-game
	UPROPERTY(EditDefaultsOnly, Category = "Stun")
	FStatModifiedValue DepletionRate;

	// The animation montage to play on counselors
	UPROPERTY(EditDefaultsOnly, Category = "Stun")
	UAnimMontage* CounselorStunAnimMontage;

	// The animation montage to play on the killer
	UPROPERTY(EditDefaultsOnly, Category = "Stun")
	UAnimMontage* JasonStunAnimMontage;

	// The animation montage to play on the killer when they have a two handed weapon (if no animation is provided, will use the default above)
	UPROPERTY(EditDefaultsOnly, Category = "Stun")
	UAnimMontage* JasonStunAnimMontage2H;

	// Directional animation montages to play on the killer
	UPROPERTY(EditDefaultsOnly, Category = "Stun")
	UAnimMontage* JasonDirectionalStunAnimMontages[static_cast<int32>(ESCCharacterHitDirection::MAX)];

	// Should we check to make sure this stun can be played in the character's current location
	UPROPERTY(EditDefaultsOnly, Category = "Stun")
	bool bShouldCheckStunArea;

	UPROPERTY(EditDefaultsOnly, Category = "Stun|UI")
	FText StunText;

	UPROPERTY(EditDefaultsOnly, Category = "Stun|PamelaSweater")
	bool IsPamelasSweater;

	UPROPERTY(EditDefaultsOnly, Category = "Stun")
	bool bCanBeBlocked;
};
