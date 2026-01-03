// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLUserWidget.h"
#include "IllMinimapWidget.generated.h"

/**
 * @class UILLMinimapWidget
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLMinimapWidget
: public UILLUserWidget
{
	GENERATED_UCLASS_BODY()
	
public:
	/**
	 * Initializes the necessary references for the widget, If overridden be sure to call Super
	 *
	 * @param Owner - The actor that this widget is displaying on the minimap.
	 * @param MiniMapOwner - the actor who owns the minimap widget, who's hud is it?
	 * @param minimap - the minimap object that this widget was added to.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "MiniMap|Binding")
	void InitializeWidget(UObject* InOwner, AActor* InMiniMapOwner, class UILLMinimap* InMinimap);

	// Blueprintable function to update the display of the widget, called every frame.
	UFUNCTION(BlueprintNativeEvent, Category = "MiniMap|Binding")
	bool UpdateMapWidget(UPARAM(ref) FVector& CurrentPosition, UPARAM(ref) FRotator& CurrentRotation);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap|Binding")
	bool bShowOnMinimapEdge = false;

	UPROPERTY(BlueprintReadOnly, Category = "MiniMap|Binding")
	bool bIsOnMinimap = true;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "MiniMap|Slot")
	int32 ZOrder;

protected:
	UPROPERTY(BlueprintReadWrite, Category = "MiniMap|Binding")
	UObject* Owner;

	UPROPERTY(BlueprintReadWrite, Category = "MiniMap|Binding")
	AActor* MiniMapOwner;

	UPROPERTY(BlueprintReadWrite, Category = "MiniMap|Binding")
	class UILLMinimap* Minimap;

	UPROPERTY(EditAnywhere, Category = "Minimap|Binding")
	bool bScaleWithMap;

	UPROPERTY(EditAnywhere, Category = "Minimap|Binding")
	FVector2D DefaultSize;
};
