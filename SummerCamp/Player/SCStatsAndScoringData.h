// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Engine/DataAsset.h"
#include "SCStatsAndScoringData.generated.h"

class ASCCharacter;
class ASCCounselorCharacter;
class ASCItem;
class ASCKillerCharacter;
class ASCPlayerState;
class USCPerkData;

/**
* @class USCStatBadge
*/
UCLASS(MinimalAPI, const, Blueprintable, BlueprintType)
class USCStatBadge
: public UDataAsset
{
	GENERATED_UCLASS_BODY()
	
	// Icon for this badge
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> BadgeIcon;

	// Two star icon for this badge
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> TwoStarBadgeIcon;

	// Three star icon for this badge
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> ThreeStarBadgeIcon;

	// The name of this badge
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText DisplayName;

	// The string identifier used on the backend for this badge
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FString BackendId;

	// The description of this badge
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText Description;

	// How many times this badge needs to be earned to get 1 star
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 OneStarUnlock;

	// How many times this badge needs to be earned to get 2 stars
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 TwoStarUnlock;

	// How many times this badge needs to be earned to get 3 stars
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 ThreeStarUnlock;
};

/**
* @struct FSCBadgeAwardInfo
*/
USTRUCT(BlueprintType)
struct FSCBadgeAwardInfo
{
	GENERATED_USTRUCT_BODY()
	
	FSCBadgeAwardInfo() {}
	FSCBadgeAwardInfo(TSubclassOf<USCStatBadge> Badge, const int32 Level)
	: BadgeEarned(Badge)
	, StarLevel(Level)
	{

	}

	// Badge
	UPROPERTY(BlueprintReadOnly)
	TSubclassOf<USCStatBadge> BadgeEarned;

	// Star level
	UPROPERTY(BlueprintReadOnly)
	int32 StarLevel;
};

/**
 * @enum EScoreEventCategory
 */
UENUM(BlueprintType)
enum class EScoreEventCategory : uint8
{
	Offense			UMETA(DisplayName = "Offense"),
	Defense			UMETA(DisplayName = "Defense"),
	Betrayal		UMETA(DisplayName = "Betrayal"),
	Fixer			UMETA(DisplayName = "Fixer"),
	Escaped			UMETA(DisplayName = "Escaped"),
	TeamEscaped		UMETA(DisplayName = "Team Escaped"),
	Reinforcements	UMETA(DisplayName = "Reinforcements"),
	CompletedMatch	UMETA(DisplayName = "Completed Match"),
	TimeBonus		UMETA(DisplayName = "Time Bonus"),
	Demasking		UMETA(DisplayName = "Demasking"),
	Kill			UMETA(DisplayName = "Kill"),
	NoSurvivors		UMETA(DisplayName = "No Survivors"),
	Resourceful		UMETA(DisplayName = "Resourceful"),
	Versatile		UMETA(DisplayName = "Versatile"),
	Seeker			UMETA(DisplayName = "Seeker"),
	DemolitionMan	UMETA(DisplayName = "Demolition Man"),
	BoatKill		UMETA(DisplayName = "Boat Kill"),
	Butcher			UMETA(DisplayName = "Butcher"),
	Trapper			UMETA(DisplayName = "Trapper"),
	PartyCrasher	UMETA(DisplayName = "Party Crasher"),
	DefeatedJason	UMETA(DisplayName = "Defeated Jason!"),
	Escort			UMETA(DisplayName = "Escort"),
	Sacrifice		UMETA(DisplayName = "Sacrifice"),
	INVALID			UMETA(Hidden)
};

namespace NSCScoreEvents
{
	inline const FText ToString(EScoreEventCategory inCategory)
	{
		switch(inCategory)
		{
		case EScoreEventCategory::Offense:
			return FText::FromName(TEXT("Offense"));

		case EScoreEventCategory::Defense:
			return FText::FromName(TEXT("Defense"));

		case EScoreEventCategory::Betrayal:
			return FText::FromName(TEXT("Betrayal"));

		case EScoreEventCategory::Fixer:
			return FText::FromName(TEXT("Fixer"));

		case EScoreEventCategory::Escaped:
			return FText::FromName(TEXT("Escaped"));

		case EScoreEventCategory::TeamEscaped:
			return FText::FromName(TEXT("Team Escaped"));

		case EScoreEventCategory::Reinforcements:
			return FText::FromName(TEXT("Reinforcements"));

		case EScoreEventCategory::CompletedMatch:
			return FText::FromName(TEXT("Completed Match"));

		case EScoreEventCategory::TimeBonus:
			return FText::FromName(TEXT("Time Bonus"));

		case EScoreEventCategory::Demasking:
			return FText::FromName(TEXT("Demasking"));

		case EScoreEventCategory::Kill:
			return FText::FromName(TEXT("Kill"));

		case EScoreEventCategory::NoSurvivors:
			return FText::FromName(TEXT("No Survivors"));

		case EScoreEventCategory::Resourceful:
			return FText::FromName(TEXT("Resourceful"));

		case EScoreEventCategory::Versatile:
			return FText::FromName(TEXT("Versatile"));

		case EScoreEventCategory::Seeker:
			return FText::FromName(TEXT("Seeker"));

		case EScoreEventCategory::DemolitionMan:
			return FText::FromName(TEXT("Demolition Man"));

		case EScoreEventCategory::BoatKill:
			return FText::FromName(TEXT("Boat Kill"));

		case EScoreEventCategory::Butcher:
			return FText::FromName(TEXT("Butcher"));

		case EScoreEventCategory::Trapper:
			return FText::FromName(TEXT("Trapper"));

		case EScoreEventCategory::PartyCrasher:
			return FText::FromName(TEXT("Party Crasher"));

		case EScoreEventCategory::DefeatedJason:
			return FText::FromName(TEXT("Defeated Jason!"));

		case EScoreEventCategory::Escort:
			return FText::FromName(TEXT("Escort"));

		case EScoreEventCategory::Sacrifice:
			return FText::FromName(TEXT("Sacrifice"));

		default:
			return FText::FromName(TEXT("INVALID"));

		}
	}
}

/**
* @struct FSCCategorizedScoreEvent
*/
USTRUCT(BlueprintType)
struct FSCCategorizedScoreEvent
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Score Events")
	EScoreEventCategory Category;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Score Events")
	int32 TotalScore;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Score Events")
	bool bAsHunter;

	FSCCategorizedScoreEvent()
	: Category(EScoreEventCategory::INVALID)
	, TotalScore(0)
	, bAsHunter(false)
	{}

	FSCCategorizedScoreEvent(const EScoreEventCategory InCategory, const int32 InTotalScore, const bool AsHunter)
	: Category(InCategory)
	, TotalScore(InTotalScore)
	, bAsHunter(AsHunter)
	{}

	FORCEINLINE bool operator ==(const EScoreEventCategory OtherCategory) const { return (Category == OtherCategory); }
};

/**
 * @class USCScoringEvent
 */
UCLASS(MinimalAPI, const, Blueprintable, BlueprintType)
class USCScoringEvent 
: public UDataAsset
{
	GENERATED_UCLASS_BODY()

	/** Get the display message for this event */
	FString GetDisplayMessage(const float Modifier = 1.f, const TArray<USCPerkData*>& PerkModifiers = TArray<USCPerkData*>()) const;

	/** Get the the experience for this event using a modifier */
	int32 GetModifiedExperience(const float Modifier = 1.f, const TArray<USCPerkData*>& PerkModifiers = TArray<USCPerkData*>()) const;

	// The category this event is linked to
	UPROPERTY(EditDefaultsOnly)
	EScoreEventCategory EventCategory;

	// The description that is displayed to the user when they trigger this type of event
	UPROPERTY(EditDefaultsOnly)
	FText EventDescription;

	// How much exp the player gets for this event
	UPROPERTY(EditDefaultsOnly)
	int32 Experience;

	// Can this event only be scored once in a match
	UPROPERTY(EditDefaultsOnly)
	bool bOnlyScoreOnce;

	// Is this event scored for all counselors
	UPROPERTY(EditDefaultsOnly)
	bool bAllCounselors;

	// Counselor badge linked to this score event
	UPROPERTY(EditDefaultsOnly, Category = "Badges")
	TSubclassOf<USCStatBadge> CounselorBadge;

	// Jason badge linked to this score event
	UPROPERTY(EditDefaultsOnly, Category = "Badges")
	TSubclassOf<USCStatBadge> JasonBadge;
};

UCLASS(MinimalAPI, const, Blueprintable, BlueprintType)
class USCParanoiaScore
: public UDataAsset
{
	GENERATED_UCLASS_BODY()

	/** Get the display message for this event */
	FString GetDisplayMessage(const float Modifier = 1.f, const TArray<USCPerkData*>& PerkModifiers = TArray<USCPerkData*>()) const;

	/** Get the point value for this event using a modifer */
	int32 GetModifiedScore(const float Modiier = 1.f, const TArray<USCPerkData*>& PerkModifiers = TArray<USCPerkData*>()) const;

	// The category this event is linked to
	UPROPERTY(EditDefaultsOnly)
	EScoreEventCategory EventCategory;

	// The description for this score event
	UPROPERTY(EditDefaultsOnly)
	FText ScoreDescription;

	// How many points is this event worth
	UPROPERTY(EditDefaultsOnly)
	int32 Points;

	// Can this event only be scored once in a match?
	UPROPERTY(EditDefaultsOnly)
	bool bOnlyScoreOnce;

};

/**
* @enum EItemStatFlags
*/
enum class ESCItemStatFlags : uint8
{
	PickedUp		= 1 << 0,
	Used			= 1 << 1,
	HitCounselor	= 1 << 2,
	HitJason		= 1 << 3,
	Kill			= 1 << 4,
	Stun			= 1 << 5,
};

/**
* @struct FSCItemStats
*/
USTRUCT(BlueprintType)
struct FSCItemStats
{
	GENERATED_USTRUCT_BODY()
	
	/** The type of item this is */
	UPROPERTY(BlueprintReadOnly)
	TSubclassOf<ASCItem> ItemClass;

	/** Number of times this player has picked up this item */
	UPROPERTY(BlueprintReadOnly)
	int32 TimesPickedUp;

	/** Number of times this player has used this item */
	UPROPERTY(BlueprintReadOnly)
	int32 TimesUsed;

	//////////////////////////////////////////////////
	// Weapon Stats

	/** Number of times this player hit a counselor with this weapon */
	UPROPERTY(BlueprintReadOnly)
	int32 CounselorsHit;

	/** Number of times this player hit Jason with this weapon */
	UPROPERTY(BlueprintReadOnly)
	int32 JasonHits;

	/** Number of kills with this weapon */
	UPROPERTY(BlueprintReadOnly)
	int32 Kills;

	/** Number of stuns with this weapon */
	UPROPERTY(BlueprintReadOnly)
	int32 Stuns;

	// ~Weapon Stats
	//////////////////////////////////////////////////

	FSCItemStats() {}
	FSCItemStats(TSubclassOf<ASCItem> Item, uint8 StatFlags = 0);

	FSCItemStats& operator +=(const FSCItemStats& OtherItem);

	FSCItemStats& operator +=(const uint8 StatFlags)
	{
		if (StatFlags & (uint8)ESCItemStatFlags::PickedUp)
			TimesPickedUp++;
		if (StatFlags & (uint8)ESCItemStatFlags::Used)
			TimesUsed++;
		if (StatFlags & (uint8)ESCItemStatFlags::HitCounselor)
			CounselorsHit++;
		if (StatFlags & (uint8)ESCItemStatFlags::HitJason)
			JasonHits++;
		if (StatFlags & (uint8)ESCItemStatFlags::Kill)
			Kills++;
		if (StatFlags & (uint8)ESCItemStatFlags::Stun)
			Stuns++;

		return *this;
	}
};

/**
 * @struct FSCKillStats
 */
USTRUCT(BlueprintType)
struct FSCKillStats
{
	GENERATED_USTRUCT_BODY()
	
	FSCKillStats()
	: KillName(NAME_None)
	, KillCount(0)
	{

	}

	FSCKillStats(const FName InKillName, const int32 InKillCount)
	: KillName(InKillName)
	, KillCount(InKillCount)
	{

	}

	bool operator==(const FSCKillStats& Other) const { return KillName == Other.KillName; }
	
	/** Name of the kill */
	UPROPERTY(BlueprintReadOnly)
	FName KillName;

	/** The number of times this kill was performed */
	UPROPERTY(BlueprintReadOnly)
	int32 KillCount;
};

/**
* @struct FSCPlayerStats
*/
USTRUCT(BlueprintType)
struct FSCPlayerStats
{
	GENERATED_USTRUCT_BODY()

	//////////////////////////////////////////////////
	// Counselor Stats

	/** Number repairs done to a boat */
	UPROPERTY(BlueprintReadOnly)
	int32 BoatPartsRepaired;

	/** Number repairs done to a car */
	UPROPERTY(BlueprintReadOnly)
	int32 CarPartsRepaired;

	/** Number closed windows this player has jumped through */
	UPROPERTY(BlueprintReadOnly)
	int32 ClosedWindowsJumpedThrough;

	/** Number of matches played as a counselor */
	UPROPERTY(BlueprintReadOnly)
	int32 CounselorMatchesPlayed;

	/** Number of times this player has been killed by Jason */
	UPROPERTY(BlueprintReadOnly)
	int32 DeathsByJason;

	/** Number of times this player has been killed by another counselor */
	UPROPERTY(BlueprintReadOnly)
	int32 DeathsByOtherCounselors;

	/** Number times this player has escaped in a boat as the captain */
	UPROPERTY(BlueprintReadOnly)
	int32 EscapedInBoatAsDriverCount;

	/** Number times this player has escaped in a boat as a passenger */
	UPROPERTY(BlueprintReadOnly)
	int32 EscapedInBoatAsPassengerCount;

	/** Number times this player has escaped in a car as the driver */
	UPROPERTY(BlueprintReadOnly)
	int32 EscapedInCarAsDriverCount;

	/** Number times this player has escaped in a car as a passenger */
	UPROPERTY(BlueprintReadOnly)
	int32 EscapedInCarAsPassengerCount;

	/** Number times this player has escaped in a car as a passenger */
	UPROPERTY(BlueprintReadOnly)
	int32 EscapedWithPoliceCount;

	/** Number repairs done to electrical and phone boxes */
	UPROPERTY(BlueprintReadOnly)
	int32 GridPartsRepaired;

	/** Number of times this player has killed Jason */
	UPROPERTY(BlueprintReadOnly)
	int32 KilledJasonCount;

	/** Number of times this player has killed another counselor (friendly fire kills) */
	UPROPERTY(BlueprintReadOnly)
	int32 KilledAnotherCounselorCount;

	/** Number of times this player has knocked Jason's mask off */
	UPROPERTY(BlueprintReadOnly)
	int32 KnockedJasonMaskOffCount;

	/** Number of times this player has called the police */
	UPROPERTY(BlueprintReadOnly)
	int32 PoliceCallCount;

	/** Number of times this player has stunned Jason using Pamela's sweater */
	UPROPERTY(BlueprintReadOnly)
	int32 SweaterStunCount;

	/** Number of times this player has called Tommy Jarvis */
	UPROPERTY(BlueprintReadOnly)
	int32 TommyJarvisCallCount;

	/** Number of times this player has played as Tommy Jarvis */
	UPROPERTY(BlueprintReadOnly)
	int32 TommyJarvisMatchesPlayed;

	/** Counselor classes played */
	UPROPERTY(BlueprintReadOnly)
	TArray<TSoftClassPtr<ASCCounselorCharacter>> CounselorsPlayed;

	// ~Counselor Stats
	//////////////////////////////////////////////////

	/** Number of suicides */
	UPROPERTY(BlueprintReadOnly)
	int32 Suicides;

	//////////////////////////////////////////////////
	// Jason Stats

	/** Number of times a car slam was preformed */
	UPROPERTY(BlueprintReadOnly)
	int32 CarSlamCount;

	/** Number of Counselors this player has killed */
	UPROPERTY(BlueprintReadOnly)
	int32 CounselorsKilled;

	/** Number of doors this player has broken down as Jason */
	UPROPERTY(BlueprintReadOnly)
	int32 DoorsBrokenDown;

	/** Number of times this player died as Jason */
	UPROPERTY(BlueprintReadOnly)
	int32 JasonDeathsByCounselors;

	/** Number of matches played as Jason */
	UPROPERTY(BlueprintReadOnly)
	int32 JasonMatchesPlayed;

	/** Number times this player has hit a counselor with throwing knives */
	UPROPERTY(BlueprintReadOnly)
	int32 ThrowingKnifeHits;

	/** Number of walls this player has broken down as Jason */
	UPROPERTY(BlueprintReadOnly)
	int32 WallsBrokenDown;

	/** Jason classes played */
	UPROPERTY(BlueprintReadOnly)
	TArray<TSoftClassPtr<ASCKillerCharacter>> JasonsPlayed;

	/** Kills performed */
	UPROPERTY(BlueprintReadOnly)
	TArray<FSCKillStats> ContextKillsPerformed;

	/** 
	 * Increases the kill count for the specified kill
	 * @param KillName The name of the kill performed
	 */
	void PerformedKill(const FName KillName);

	/**
	 * Checks to see if this player has performed the specified kill
	 * @param KillName The name of the kill to check
	 * @return True if this player has performed the kill
	 */
	bool HasPerformedKill(const FName KillName);

	/**
	* Get index for the specified kill.
	* @param KillName The kill to find stats for
	* @return The index of the kill stats. -1 if not found
	*/
	int32 GetContextKillIndex(const FName KillName);

	// ~Jason Stats
	//////////////////////////////////////////////////

	//////////////////////////////////////////////////
	// Item Stats

	UPROPERTY(BlueprintReadOnly)
	TArray<FSCItemStats> ItemStats;

	/**
	 * Parse player stats from the backend.
	 * @param JsonString the json string containing the player stats
	 */
	void ParseStats(const FString& JsonString, const ASCPlayerState* OwningPlayer);
	
	/**
	 * Serialize Jason feats into JSON
	 * @param JsonWriter The JSON writer to write the feats in to.
	 */
	void WriteJasonFeats(TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>>& JsonWriter) const;

	/**
	* Serialize Counselor feats into JSON
	* @param JsonWriter The JSON writer to write the feats in to.
	*/
	void WriteCounselorFeats(TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>>& JsonWriter) const;

	/**
	 * Get index for the specified item.
	 * @param ItemClass The item class to find stats for
	 * @return The index of the item stats. -1 if not found
	 */
	int32 GetItemStatsIndex(TSubclassOf<ASCItem> ItemClass) const;

	/**
	* Find stats for the specified item.
	* @param ItemClass The item class to find stats for
	* @return The stats for the specified item.
	*/
	FSCItemStats GetItemStats(TSubclassOf<ASCItem> ItemClass) const;

	// ~Item Stats
	//////////////////////////////////////////////////
};