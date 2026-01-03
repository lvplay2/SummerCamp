// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLDamageType.h"
#include "SCBaseDamageType.generated.h"

/**
 * @class USCBaseDamageType
 */
UCLASS(MinimalAPI, const, Blueprintable, BlueprintType)
class USCBaseDamageType
: public UILLDamageType
{
	GENERATED_UCLASS_BODY()

public:
	// Should the wiggle minigame be played for this stun?
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bShouldPlayHitReaction;

	UPROPERTY(EditDefaultsOnly)
	bool bPlayDeathAnim;
};
