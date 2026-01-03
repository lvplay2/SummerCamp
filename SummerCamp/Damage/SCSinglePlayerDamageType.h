// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCBaseDamageType.h"
#include "SCSinglePlayerDamageType.generated.h"

class USCScoringEvent;
class USCStatBadge;

/**
* @class USCContextKillDamageType 
*/
UCLASS(Blueprintable, BlueprintType)
class SUMMERCAMP_API USCSinglePlayerDamageType
	: public USCBaseDamageType
{
	GENERATED_BODY()

public:
	USCSinglePlayerDamageType(const FObjectInitializer& ObjectInitializer);

	/** Score event linked to this kill */
	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	TSubclassOf<USCScoringEvent> ScoreEvent;
};
