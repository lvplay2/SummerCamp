// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLMinimapIconComponent.h"
#include "SCMinimapIconComponent.generated.h"

/**
 * @class USCMinimapIconComponent
 */
UCLASS(ClassGroup="SCMinimap", BlueprintType, Blueprintable, HideCategories=(LOD, Rendering), meta=(BlueprintSpawnableComponent, ShortTooltip="Component that allows this actor to display on the minimap"))
class USCMinimapIconComponent
: public UILLMinimapIconComponent
{
	GENERATED_BODY()

public:
	USCMinimapIconComponent(const FObjectInitializer& ObjectInitializer);

	/** Icon to display for this component on the killer minimap (counselors will use the default) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ILLMinimapIconComponent")
	TSubclassOf<UILLMinimapWidget> KillerIcon;
};
