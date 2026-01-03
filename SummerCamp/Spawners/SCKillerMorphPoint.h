// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLPlayerStart.h"
#include "SCKillerMorphPoint.generated.h"

/**
 * @class ASCKillerMorphPoint
 */
UCLASS()
class SUMMERCAMP_API ASCKillerMorphPoint
: public AILLPlayerStart
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY()
	UBoxComponent* OverlapVolume;
};
