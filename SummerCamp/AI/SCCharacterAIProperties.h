// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Engine/DataAsset.h"
#include "SCGameMode.h"
#include "SCCharacterAIProperties.generated.h"

class ASCItem;
class USCPerkData;

enum class ESCCounselorCloseDoorPreference : uint8;
enum class ESCCounselorLockDoorPreference : uint8;

/**
* @struct FCounslorGameDifficultySettings
*/
USTRUCT()
struct FCounslorGameDifficultySettings
{
	GENERATED_USTRUCT_BODY()
	
	FCounslorGameDifficultySettings() {}

	// Should this counselor always start with one of the possible large items?
	UPROPERTY(EditDefaultsOnly, Category="Inventory: Large Item")
	bool bAlwaysStartWithLargeItem;

	// List of possible large items this counselor can spawn with
	UPROPERTY(EditDefaultsOnly, Category="Inventory: Large Item")
	TArray<TSubclassOf<ASCItem>> PossibleLargeItems;

	// Should this counselor use the specified small items or random possible small items
	UPROPERTY(EditDefaultsOnly, Category="Inventory: Small Items")
	bool bUseSpecifiedSmallItems;

	// List of small items this counselor should spawn with
	UPROPERTY(EditDefaultsOnly, Category="Inventory: Small Items")
	TSubclassOf<ASCItem> SmallItems[3];

	// List of small items this counselor can spawn with
	UPROPERTY(EditDefaultsOnly, Category="Inventory: Small Items")
	TArray<TSubclassOf<ASCItem>> PossibleSmallItems;

	// Should this counselor use the specified perks or random possible perks
	UPROPERTY(EditDefaultsOnly, Category="Perks")
	bool bUseSpecifiedPerks;

	// List of perks this counselor should spawn with
	UPROPERTY(EditDefaultsOnly, Category="Perks")
	TSubclassOf<USCPerkData> Perks[3];

	// List of perks this counselor can spawn with
	UPROPERTY(EditDefaultsOnly, Category="Perks")
	TArray<TSubclassOf<USCPerkData>> PossiblePerks;
};

/**
 * @class USCCharacterAIProperties
 */
UCLASS(Abstract, Const)
class SUMMERCAMP_API USCCharacterAIProperties 
: public UDataAsset
{
	GENERATED_UCLASS_BODY()

	//////////////////////////////////////////////////
	// Sensing

	// Max distance at which a makenoise(1.0) loudness sound can be heard, regardless of occlusion
	UPROPERTY(EditDefaultsOnly, Category="Sensing: Hearing")
	float HearingThreshold;

	// Max distance at which a makenoise(1.0) loudness sound can be heard if unoccluded (LOSHearingThreshold should be > HearingThreshold)
	UPROPERTY(EditDefaultsOnly, Category="Sensing: Hearing")
	float LOSHearingThreshold;

	// Maximum sight distance
	UPROPERTY(EditDefaultsOnly, Category="Sensing: Sight")
	float SightRadius;

	// How far to the side AI can see, in degrees
	UPROPERTY(EditDefaultsOnly, Category="Sensing: Sight")
	float PeripheralVisionAngle;

	//////////////////////////////////////////////////
	// Stun/Break out

	// Minimum time in seconds this character should wait to simulate input to "struggle/wiggle" for the break out mini game
	UPROPERTY(EditDefaultsOnly, Category="Stun: Wiggle")
	float MinWiggleInterval;
	
	// Maximum time in seconds this character should wait to simulate input to "struggle/wiggle" for the break out mini game
	UPROPERTY(EditDefaultsOnly, Category="Stun: Wiggle")
	float MaxWiggleInterval;
};

/**
* @class USCCounselorAIProperties
*/
UCLASS(Abstract, Blueprintable)
class SUMMERCAMP_API USCCounselorAIProperties
: public USCCharacterAIProperties
{
	GENERATED_UCLASS_BODY()
	
	//////////////////////////////////////////////////
	// Difficulty Settings

	// Counselor settings based on game mode difficulty
	UPROPERTY(EditDefaultsOnly, Category = "Difficulty Settings (Offline Bots)")
	FCounslorGameDifficultySettings DifficultySettings[(int32)ESCGameModeDifficulty::MAX];

	//////////////////////////////////////////////////
	// Doors

	// Should this counselor close doors
	UPROPERTY(EditDefaultsOnly, Category="Doors")
	ESCCounselorCloseDoorPreference ShouldCloseDoors;

	// Should this counselor lock or barricade doors
	UPROPERTY(EditDefaultsOnly, Category="Doors")
	ESCCounselorLockDoorPreference ShouldLockDoors;

	// Should this counselor's preferences revert back to ExteriorOnly for ShouldCloseDoors & ShouldLockDoors when switching back into Offline Bots logic
	UPROPERTY(EditDefaultsOnly, Category="Doors")
	bool bUseDefaultPreferencesForOfflineLogic;

	// Should this counselor close doors when exiting a building
	UPROPERTY(EditDefaultsOnly, Category="Doors")
	bool bCloseDoorWhenExitingBuilding;

	// How long (in seconds) after this counselor locks/unlocks a door before they can lock/unlock a door again
	UPROPERTY(EditDefaultsOnly, Category="Doors")
	float TimeBetweenLockingInteractions;

	//////////////////////////////////////////////////
	// Hiding Spots

	// Minimum time in seconds a counselor should stay in a hiding spot
	UPROPERTY(EditDefaultsOnly, Category="HidingSpot")
	float MinTimeToHide;

	// Time in seconds a counselor should stay in a hiding spot after Jason was last heard
	UPROPERTY(EditDefaultsOnly, Category="HidingSpot")
	float HideJasonHeardMinTime;

	// Should this AI perform a LOS check with Jason before entering an hiding spot
	UPROPERTY(EditDefaultsOnly, Category="HidingSpot")
	bool bCheckJasonLOSBeforeHiding;

	// Minimum distance from Jason this counselor should be before entering a hiding spot
	UPROPERTY(EditDefaultsOnly, Category="HidingSpot")
	float MinDistanceFromJasonToHide;

	//////////////////////////////////////////////////
	// Items

	// Large item for this character to start with in single player challenges
	UPROPERTY(EditDefaultsOnly, Category="Inventory: Single Player")
	TSubclassOf<ASCItem> LargeItem;

	// Small items for this character to start with in single player challenges
	UPROPERTY(EditDefaultsOnly, Category="Inventory: Single Player")
	TSubclassOf<ASCItem> SmallItems[3];

	// Minimum Strength stat required to pick a weapon over a part when Jason is a threat
	UPROPERTY(EditDefaultsOnly, Category="Inventory: Preferences", meta=(UIMin=1, UIMax=10))
	uint8 WeaponsOverPartsMinStrength;

	// Minimum health before this counselor should use a Med Spray
	UPROPERTY(EditDefaultsOnly, Category="Inventory: Health", meta=(UIMin=10, UIMax=90))
	int32 MinHealthToUseMedSpray;

	// Minimum distance from Jason before this counselor can use a Med Spray
	UPROPERTY(EditDefaultsOnly, Category="Inventory: Health")
	float MinDistanceFromJasonToUseMedSpray;

	// Maximum distance from Jason before this counselor considers using firecrackers to distract.
	UPROPERTY(EditDefaultsOnly, Category="Inventory: Distraction")
	float MaxDistanceFromJasonToUseFirecrackers;

	//////////////////////////////////////////////////
	// Jason

	// How close Jason has to be to be considered an immediate threat, for juking
	UPROPERTY(EditDefaultsOnly, Category="Jason")
	float JasonJukeDistance;

	// How far away Jason can be before we consider him a threat, should be less than HearingThreshold so turning to face Jason works
	UPROPERTY(EditDefaultsOnly, Category="Jason")
	float JasonThreatDistance;

	// When beyond JasonThreatDistance, still consider him a threat when he was chasing us within this many seconds
	UPROPERTY(EditDefaultsOnly, Category="Jason")
	float JasonThreatChasingRecently;

	// When beyond JasonThreatDistance, still consider him a threat when he was seen within this many seconds
	UPROPERTY(EditDefaultsOnly, Category="Jason")
	float JasonThreatSeenRecently;

	// How long since we have last seen Jason for considering him a threat
	UPROPERTY(EditDefaultsOnly, Category="Jason")
	float JasonThreatSeenTimeout;

	//////////////////////////////////////////////////
	// FightBack

	// Minimum amount of time to wait after stunning Jason before considering another fight back
	UPROPERTY(EditDefaultsOnly, Category="FightBack")
	float FightBackStunDelayMinimum;

	// Maximum amount of time to wait after stunning Jason before considering another fight back
	UPROPERTY(EditDefaultsOnly, Category="FightBack")
	float FightBackStunDelayMaximum;

	// Minimum distance away from Jason to consider shooting back with a weapon
	UPROPERTY(EditDefaultsOnly, Category="FightBack: Armed")
	float ArmedFightBackMinimumDistance;

	// Maximum distance away from Jason to consider shooting back with a weapon
	UPROPERTY(EditDefaultsOnly, Category="FightBack: Armed")
	float ArmedFightBackMaximumDistance;

	// Minimum amount of time to wait after performing a melee fight back before considering another
	UPROPERTY(EditDefaultsOnly, Category="FightBack: Melee")
	float MeleeFightBackCooldownMinimum;

	// Maximum amount of time to wait after performing a melee fight back before considering another
	UPROPERTY(EditDefaultsOnly, Category="FightBack: Melee")
	float MeleeFightBackCooldownMaximum;

	// Angle requirement to consider a melee fight back when closer than MeleeFightBackMinimumDistance
	UPROPERTY(EditDefaultsOnly, Category="FightBack: Melee")
	float MeleeFightBackCloseFacingAngle;

	// How close Jason has to be to be considered for a melee fight back
	UPROPERTY(EditDefaultsOnly, Category="FightBack: Melee")
	float MeleeFightBackMinimumDistance;

	// Furthest Jason can be to be considered for a melee fight back
	UPROPERTY(EditDefaultsOnly, Category="FightBack: Melee")
	float MeleeFightBackMaximumDistance;

	// Furthest Jason can be to be considered for a melee fight back while wounded
	UPROPERTY(EditDefaultsOnly, Category="FightBack: Melee")
	float MeleeFightBackMaximumDistanceWounded;

	//////////////////////////////////////////////////
	// Flashlight

	// Minimum fear level before this character automatically turns on their flashlight inside a cabin
	UPROPERTY(EditDefaultsOnly, Category="Running")
	float MinFearForFlashlightIndoors;

	// Minimum fear level before this character automatically turns on their flashlight outside
	UPROPERTY(EditDefaultsOnly, Category="Running")
	float MinFearForFlashlightOutdoors;

	//////////////////////////////////////////////////
	// Run/Sprint

	// Minimum time to ease off of run/sprint state changes after making a change
	UPROPERTY(EditDefaultsOnly, Category="Running")
	float MinRunSprintStateEaseOff;

	// Maximum time to ease off of run/sprint state changes after making a change
	UPROPERTY(EditDefaultsOnly, Category="Running")
	float MaxRunSprintStateEaseOff;

	// Minimum amount of stamina needed for this character to run
	UPROPERTY(EditDefaultsOnly, Category="Running")
	float MinStaminaToRun;

	// Minimum fear level before this character starts to run
	UPROPERTY(EditDefaultsOnly, Category="Running")
	float MinFearToRun;

	// Minimum amount of stamina needed for this character to run
	UPROPERTY(EditDefaultsOnly, Category="Sprinting")
	float MinStaminaToSprint;

	// Minimum fear level before this character sprints
	UPROPERTY(EditDefaultsOnly, Category="Sprinting")
	float MinFearToSprint;

	//////////////////////////////////////////////////
	// Sensing

	// How far we should share audio stimulus information with nearby counselors
	UPROPERTY(EditDefaultsOnly, Category="Sensing: Sharing")
	float ShareJasonNoiseStimulusDistance;

	// How far we should share visual stimulus information with nearby counselors
	UPROPERTY(EditDefaultsOnly, Category="Sensing: Sharing")
	float ShareJasonVisualStimulusDistance;

	//////////////////////////////////////////////////
	// Windows

	// Minimum fear required to perform a window-dive
	UPROPERTY(EditDefaultsOnly, Category="Windows")
	float MinFearForWindowDive;

	// Minimum health required to perform a window-dive
	UPROPERTY(EditDefaultsOnly, Category="Windows")
	float MinHealthForWindowDive;
};

/**
 * @class USCJasonAIProperties
 */
UCLASS(Abstract, Blueprintable)
class SUMMERCAMP_API USCJasonAIProperties
: public USCCharacterAIProperties
{
	GENERATED_UCLASS_BODY()


	//////////////////////////////////////////////////
	// Combat

	// Max distance a counselor can be from this killer for grabbing
	UPROPERTY(EditDefaultsOnly, Category="Combat|Grab")
	float MaxGrabDistance;

	// Min distance a counselor can be before attempting to throw a knife
	UPROPERTY(EditDefaultsOnly, Category="Combat|ThrowingKnife")
	float MinThrowingKnifeDistance;

	// Max distance a counselor can before for a throwing knife
	UPROPERTY(EditDefaultsOnly, Category="Combat|ThrowingKnife")
	float MaxThrowingKnifeDistance;
};
