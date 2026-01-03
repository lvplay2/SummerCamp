// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Volumes/ILLMiniMapVolume.h"
#include "SCMiniMapVolume.generated.h"

/**
 * 
 */
UCLASS()
class SUMMERCAMP_API ASCMiniMapVolume : public AILLMiniMapVolume
{
	GENERATED_BODY()
	
public:
	ASCMiniMapVolume(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap Data")
	TSoftObjectPtr<UTexture2D> KillerMapTexture;
	
	
};
