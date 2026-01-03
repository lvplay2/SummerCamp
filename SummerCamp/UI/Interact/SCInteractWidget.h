// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCUserWidget.h"
#include "SCInteractWidget.generated.h"

/**
 * @class USCInteractWidget
 */
UCLASS()
class SUMMERCAMP_API USCInteractWidget
: public USCUserWidget
{
	GENERATED_UCLASS_BODY()

public:
	// Begin UUserWidget interface
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	// End UUserWidget interface

	UFUNCTION()
	void UpdatePosition();

	UPROPERTY(BlueprintReadOnly, Category = "Default")
	class USCInteractComponent* LinkedInteractable;

	UFUNCTION(BlueprintPure, Category = "Default")
	ESlateVisibility ShowHoldRing() const;

	UFUNCTION(BlueprintPure, Category = "Default")
	ESlateVisibility ShowHoldRingBackground() const;

	UPROPERTY(BlueprintReadWrite, Category = "Default")
	class UMaterialInstanceDynamic* DynamicHoldMaterial;

	UFUNCTION(BlueprintImplementableEvent, Category = "Default")
	void ShowWidget();

	UFUNCTION(BlueprintImplementableEvent, Category = "Default")
	void HideWidget();

	UFUNCTION(BlueprintImplementableEvent, Category = "Default")
	void OnIsInteractable();

	UFUNCTION(BlueprintImplementableEvent, Category = "Default")
	void OnNotInteractable();

	UFUNCTION(BlueprintImplementableEvent, Category = "Default")
	void OnNotInteractableOrHighlighted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Default")
	void OnCleanUpWidget();

	UFUNCTION(BlueprintPure, Category = "Default")
	bool CanHoldInteract() const;

	UFUNCTION(BlueprintCallable, Category = "Default")
	void CleanUpWidget();
	
	bool bBestInteractable;

	bool bIsHighlighted;

	UPROPERTY(BlueprintReadOnly, Category = "Default")
	TSubclassOf<class USCInteractIconWidget> InteractionIconWidgetClass;

	/** Should this widget be drawn in specified screen space (Default = false) */
	UPROPERTY()
	bool bDrawInExactScreenSpace;

	/** The exact Screen space location (0 - 1 space) to draw this widget if bDrawInExactScreenSpace = true */
	UPROPERTY()
	FVector2D ScreenLocationRatio;
};
