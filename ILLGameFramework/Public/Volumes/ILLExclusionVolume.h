// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Volume.h"
#include "IllExclusionVolume.generated.h"

/**
 * 
 */
UCLASS()
class ILLGAMEFRAMEWORK_API AIllExclusionVolume : public AVolume
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure, Category = "Extents")
	FBoxSphereBounds GetVolumeExtents();
	
	
};
