// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLMinimap.h"
#include "SCMiniMapBase.generated.h"

/**
 * @class USCMiniMapBase
 */
UCLASS()
class SUMMERCAMP_API USCMiniMapBase
: public UILLMinimap
{
	GENERATED_UCLASS_BODY()

protected:
	// Begin UILLMinimap interface
	virtual void InitializeMapMaterial(UMaterialInstanceDynamic* MID, AILLMiniMapVolume* Volume) override;
	// End UILLMinimap interface

public:
	// If set, bRotateMap will reflect the user preference in game settings
	UPROPERTY(EditDefaultsOnly, Category = "Minimap|Data")
	bool bCanEverRotate;
};
