// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Object.h"
#include "SCCounselorActiveAbility.generated.h"

class ASCCharacter;
class ASCCounselorCharacter;

/**
 * @enum EAbilityType
 */
UENUM()
enum class EAbilityType : uint8
{
	// Instantly heals
	Healing,
	// Repairing takes less time
	Repair,
	// Shows Jason on the map
	Spotting,
	// Instantly reduces fear
	Fear,
	// Stops stamina loss
	Sprint,
	// Pamela's Sweater
	PamelasSweater
};

/**
 * @class USCCounselorActiveAbility
 */
UCLASS(Blueprintable)
class SUMMERCAMP_API USCCounselorActiveAbility 
: public UObject
{
	GENERATED_UCLASS_BODY()

public:
	void Activate(ASCCounselorCharacter* OwningCounselor);

	UFUNCTION(BlueprintCallable, Category = "Ability")
	FORCEINLINE bool HasBeenUsed() const { return bUsed; }

	// The type of ability this is
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	EAbilityType AbilityType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	FText AbilityName;

	// Discription of what this ability does
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	FText AbilityDiscription;
	
	// Icon for this ability
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	UTexture2D* UIIcon;

	// Effect for this ability
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	UParticleSystem* AbilityVFX;

	// Does this ability affect people around
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	bool bAOEAbility;

	// Area of effect range
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	float AbilityRange;

	// The value of the ability
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	float AbilityModifierValue;

	// Is this ability an instant ability or does it last for a duration
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	bool bInstant;

	// The duration this ability lasts before it wears off
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	float AbilityDuration;

	// The VO the counselor hears (says) when activating the ability
	UPROPERTY(EditDefaultsOnly, Category = "Counselor VO")
	USoundBase* CounselorVO;

	// The VO Jason hears when activating the ability
	UPROPERTY(EditDefaultsOnly, Category = "Jason VO")
	USoundBase* JasonVO;

	/** Returns true if this ability hasn't been used and the player is in a state to use it */
	UFUNCTION(BlueprintPure, Category = "Ability")
	bool CanUseAbility() const;

	/** Sets our owner */
	FORCEINLINE void SetAbilityOwner(ASCCounselorCharacter* NewOwner) { AbilityOwner = NewOwner; }

	/** Sets bUsed, for use with the infinite sweater cheat */
	void SetUsed(const bool bHasBeenUsed) { bUsed = bHasBeenUsed;}

protected:
	/** Instantly gives the specified counselor the ability benefit */
	void InstantActivate(ASCCharacter* Character);

	// Has this ability been used
	bool bUsed;

	// The counselor that activated this ability
	UPROPERTY()
	ASCCounselorCharacter* AbilityOwner;
};
