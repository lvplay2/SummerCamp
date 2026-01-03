// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Volume.h"
#include "ILLMiniMapVolume.generated.h"

/**
 * 
 */
UCLASS(HideCategories = "Collision")
class ILLGAMEFRAMEWORK_API AILLMiniMapVolume : public AVolume
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap Data")
	TSoftObjectPtr<UTexture2D> MapTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap Data")
	float DefaultMinimapZoom = 1.f;
	
};
