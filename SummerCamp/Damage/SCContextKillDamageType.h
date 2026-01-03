// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCNamedDamageType.h"
#include "SCContextKillDamageType.generated.h"

class USCScoringEvent;
class USCStatBadge;

/**
 * @class USCContextKillDamageType 
 */
UCLASS(Blueprintable, BlueprintType)
class SUMMERCAMP_API USCContextKillDamageType
	: public USCNamedDamageType
{
	GENERATED_BODY()

public:
	USCContextKillDamageType(const FObjectInitializer& ObjectInitializer);
};
