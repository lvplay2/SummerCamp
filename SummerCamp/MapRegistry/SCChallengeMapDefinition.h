// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCMapDefinition.h"
#include "SCChallengeMapDefinition.generated.h"

class USCChallengeData;

/**
 * @class USCMapDefinition
 */
UCLASS(Abstract, MinimalAPI, const, Blueprintable, BlueprintType)
class USCChallengeMapDefinition 
: public USCMapDefinition
{
	GENERATED_BODY()

public:
	USCChallengeMapDefinition(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) { }

	//////////////////////////////////////////////////
	// Challenge Data
public:
	/** Getter for MapChallenges */
	FORCEINLINE const TArray<TSubclassOf<USCChallengeData>>& GetMapChallenges() const { return MapChallenges; }

protected:
	// List of Challenge available for this single player map
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Challenge Data")
	TArray<TSubclassOf<USCChallengeData>> MapChallenges;
};
