// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Engine/DataAsset.h"
#include "SCObjectiveData.generated.h"

/**
 * @class USCObjectiveData
 */
UCLASS(Abstract, MinimalAPI, const, Blueprintable, BlueprintType)
class USCObjectiveData
: public UDataAsset
{
	GENERATED_BODY()

public:
	USCObjectiveData(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) { }

	//////////////////////////////////////////////////
	// Objective Data
public:
	/** Getter for CPReward */
	FORCEINLINE int32 GetCPReward() const { return CPReward; }

	/** Getter for bIsHidden */
	FORCEINLINE bool GetIsHidden() const { return bIsHidden; }
	
	/** Unique identifier for this objective */
	FORCEINLINE const FString& GetObjectiveID() const { return ObjectiveID; }

	/* Unique title for this objective */
	FORCEINLINE const FText& GetObjectiveTitle() const { return Title; }

protected:
	// Title for the objective
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective Data")
	FText Title;

	// Description of the objective
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective Data")
	FText Description;

	// Backend identifier for objectives, SHOULD NOT BE CHANGED. Must be unique inside the challenge (can be duplicated by other challenges)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective Data")
	FString ObjectiveID;

	// CP Reward for when you finish this objective (ONLY REWARDED ONCE)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective Data")
	int32 CPReward; // May move this to the backend only

	// If true, don't show this objective until it's completed
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective Data")
	bool bIsHidden;
};
