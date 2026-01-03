// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCBaseDamageType.h"
#include "SCNamedDamageType.generated.h"

class USCScoringEvent;
class USCStatBadge;

/**
* @class USCContextKillDamageType 
*/
UCLASS(Blueprintable, BlueprintType)
class SUMMERCAMP_API USCNamedDamageType
	: public USCBaseDamageType
{
	GENERATED_BODY()

public:
	USCNamedDamageType(const FObjectInitializer& ObjectInitializer);

	/** Name for this context kill */
	UPROPERTY(EditDefaultsOnly)
	FName KillName;

	/** Display text shown on scoreboard (ie. Murdered, barbecued, etc..) */
	UPROPERTY(EditDefaultsOnly)
	FText ScoreboardDescription;

	/** Achievements linked to this kill */
	UPROPERTY(EditDefaultsOnly)
	TArray<FName> LinkedAchievements;

	/** Score event linked to this kill */
	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	TSubclassOf<USCScoringEvent> ScoreEvent;

	/** Badge linked to this kill */
	UPROPERTY(EditDefaultsOnly, Category = "Scoring|Badges")
	TSubclassOf<USCStatBadge> KillBadge;
};
