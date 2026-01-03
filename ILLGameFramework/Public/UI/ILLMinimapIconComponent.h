// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Components/SceneComponent.h"
#include "ILLMinimapIconComponent.generated.h"

class UILLMinimapWidget;

/**
 * @class UILLMinimapIconComponent
 */
UCLASS(ClassGroup="ILLMinimap", BlueprintType, Blueprintable, HideCategories=(LOD, Rendering), meta=(BlueprintSpawnableComponent, ShortTooltip="Component that allows this actor to display on the minimap"))
class ILLGAMEFRAMEWORK_API UILLMinimapIconComponent
: public USceneComponent
{
	GENERATED_BODY()

public:
	UILLMinimapIconComponent(const FObjectInitializer& ObjectInitializer);

	// Begin UActorComponent Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// End UActorComponent Interface

	/** Icon to display for this component on the player minimap */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ILLMinimapIconComponent")
	TSubclassOf<UILLMinimapWidget> DefaultIcon;

	/** If true, display the icon on the minimap automatically when BeginPlay is called */
	UPROPERTY(EditAnywhere, Category = "ILLMinimapIconComponent")
	uint32 bStartVisible : 1;
};
