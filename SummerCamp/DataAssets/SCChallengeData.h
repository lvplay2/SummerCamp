// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Engine/DataAsset.h"
#include "SCEmoteData.h"
#include "SCChallengeData.generated.h"

class USCObjectiveData;

/**
 * @class USCChallengeData
 */
UCLASS(Abstract, MinimalAPI, const, Blueprintable, BlueprintType)
class USCChallengeData
: public UDataAsset
{
	GENERATED_BODY()

public:
	USCChallengeData(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) { }

	//////////////////////////////////////////////////
	// Challenge Data
public:
	/** Gets the list of Challenge objectives */
	FORCEINLINE const TArray<TSubclassOf<USCObjectiveData>>& GetChallengeObjectives() const { return ChallengeObjectives; }

	/** Getter for XPRequiredForSkull */
	FORCEINLINE int32 GetXPRequiredForSkull() const { return XPRequiredForSkull; }

	/** Gets the sub-level name we want to load when playing this challenge */
	FORCEINLINE const FName& GetChallengeSublevelName() const { return ChallengeSublevelName; }

	/** Helper to get LobbyThumbnailImages for a USCChallengeData class */
	UFUNCTION(BlueprintPure, Category = "MapDefinition")
	static TArray<UTexture2D*> GetLobbyThumbnailImages(const TSubclassOf<USCChallengeData>& ChallengeClass)
	{
		TArray<UTexture2D*> OutData;
		if (ChallengeClass)
		{
			for (TSoftObjectPtr<UTexture2D> Image : ChallengeClass->GetDefaultObject<USCChallengeData>()->LobbyThumbnailImages)
			{
				OutData.Add(Image.LoadSynchronous());
			}
		}

		return OutData;
	}

	/** Returns the challenge index so we can sort challenges in the lobby */
	FORCEINLINE int32 GetChallengeIndex() const { return ChallengeIndex; }

	/** Unique identifier for this challenge */
	FORCEINLINE const FString& GetChallengeID() const { return ChallengeID; }

	// The emote to unlock if the XP skull is achieved
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Challenge Data")
	TSubclassOf<USCEmoteData> XPEmoteUnlock;

	// The emote to unlock if the kills skull is achieved
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Challenge Data")
	TSubclassOf<USCEmoteData> AllKillEmoteUnlock;

	// The emote to unlock if the stealth skull is achieved
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Challenge Data")
	TSubclassOf<USCEmoteData> StealthEmoteUnlock;

protected:
	// List of all the objectives for this Challenge
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Challenge Data")
	TArray<TSubclassOf<USCObjectiveData>> ChallengeObjectives;

	// Sub-level to load when loading this map in order to play this mission
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Challenge Data")
	FName ChallengeSublevelName;

	// Challenge name to display to the user
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Challenge Data")
	FText ChallengeName;

	// Challenge description to display to the user
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Challenge Data")
	FText ChallengeDescription;

	// Backend identifier for challenges, SHOULD NOT BE CHANGED. If you want to re-order challenges, update the index ONLY
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Challenge Data")
	FString ChallengeID;

	// Order for the challenges to show up in the lobby. NOTE: These may not be unique! Use the ChallengeID when looking for a specific challenge.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Challenge Data")
	int32 ChallengeIndex;

	// Minimum XP requirement to earn the XP skull
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Challenge Data")
	int32 XPRequiredForSkull;

	// Thumbnail images to use for this Challenge in the single player challenge select
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	TArray<TSoftObjectPtr<UTexture2D>> LobbyThumbnailImages;
};
