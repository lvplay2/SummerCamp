// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "StatModifiedValue.generated.h"

class ASCCounselorCharacter;

/**
 * @enum ECounselorStats
 */
UENUM(BlueprintType)
enum class ECounselorStats : uint8
{
	/** Affects the percentage to land killing blow on Jason, chance to knock Jason’s mask off, length of stun on hits, and ability to break free from grabs. */
	Strength		UMETA(DisplayName = "Strength"),
	/** Affects the top speed while sprinting. */
	Speed			UMETA(DisplayName = "Speed"),
	/** Affects speed of interaction with objects, such as vehicle and phone repair speed, setting traps, etc. */
	Intelligence	UMETA(DisplayName = "Intelligence"),
	/** Affects stamina cost for sprinting, and stamina refresh rate. */
	Stamina			UMETA(DisplayName = "Stamina"),
	/** Affects how often you get wounded versus death blows. */
	Luck			UMETA(DisplayName = "Luck"),
	/** Affects the magnitude of modifiers to fear meter. */
	Composure		UMETA(DisplayName = "Composure"),
	/** Affects blood sense radius and duration of blood ping. */
	Stealth			UMETA(Displayname = "Stealth"),
	/** Total number of stats counselors have. */
	MAX				UMETA(Hidden)
};

/**
 * @struct FStatModifiedValue
 */
USTRUCT(BlueprintType)
struct FStatModifiedValue
{
	GENERATED_USTRUCT_BODY()

public:
	FStatModifiedValue();

	/** Default middle of the road value, couneslors will get this with all modifying stats at 6 */
	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	float BaseValue;

	/**
	 * Represented as an upper and lower bounds percentage change
	 * e.g. +/- 5% on a base value of 10 would equate to a range of [9.5 -> 10.5]
	 * ... so you'd enter 5
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	float StatModifiers[(uint8)ECounselorStats::MAX];

	/** Clear the baked value, forcing the next GetModifiedValue to re-calculate it */
	void SetDirty() { BakedValue = FLT_MAX; }

	/**
	 * Calculates the stat-modified value based on couneslor stats, first call bakes the value in
	 * @param Counselor - The character you want to get the stats from (CANNOT use nullptr, in this case just grab BaseValue). If this value changes from the first call to Get() the value will be re-baked
	 * @param Reverse - If true, higher stats will produce a lower number (useful for mods to decrease negative effects). Be consistant! Baked values will NOT be updated from calling with mixed reverse values. Call SetDirty() if you NEED to re-bake
	 * @return Baked down value based on the stats of the passed in counselor
	 */
	float Get(const ASCCounselorCharacter* Counselor, const bool Reverse = false) const;

private:
	/** This is computed the first time GetModifiedValue is called and returned every time after */
	mutable float BakedValue;

	/** Handle baking the stat modified values for different characters! (Useful for stun damage) */
	mutable const ASCCounselorCharacter* BakedCounselor;
};
