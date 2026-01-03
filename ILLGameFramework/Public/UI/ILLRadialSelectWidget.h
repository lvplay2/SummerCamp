// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLUserWidget.h"
#include "ILLRadialSelectWidget.generated.h"

/**
 * @class UILLRadialSelectWidget
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLRadialSelectWidget
: public UILLUserWidget
{
	GENERATED_UCLASS_BODY()

public:
	// Begin UUserWidget interface
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	// End UUserWidget interface

	UFUNCTION(BlueprintCallable, Category = "Input")
	void OnConfirmPressed();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void OnCancelPressed();

protected:
	/** Number of sections to divide the circle into (divisible by 2, please). */
	UPROPERTY(EditDefaultsOnly, Category = "Defaults")
	int32 NumSections;

	/** Name of the action mapping for confirm command handling */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	FName ConfirmInputName;

	/** Name of the action mapping for cancel command handling */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	FName CancelInputName;

	/** Name of X-axis mapping for left stick navigating the widget */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	FName NavigationLeftXName;

	/** Name of Y-axis mapping for left stick navigating the widget */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	FName NavigationLeftYName;

	/** Name of X-axis mapping for right stick navigating the widget */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	FName NavigationRightXName;

	/** Name of Y-axis mapping for right stick navigating the widget */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	FName NavigationRightYName;

	/** Offsets the start of the first index of the radial wheel (for making cardinal directions open spaces instead of lines) */
	UPROPERTY(EditDefaultsOnly, Category = "Defaults")
	bool bOffsetRotation;

	/** Whether or not the highlight should be visible */
	UPROPERTY(BlueprintReadOnly, Category = "Highlight")
	bool bShowHighlight;

	/** Blueprint function to update the highlight image rotation */
	UFUNCTION(BlueprintImplementableEvent, Category = "Highlight")
	void OnUpdateHighlightRotation(float Rotation);

	/** Blueprint function to handle left stick returning to a neutral position */
	UFUNCTION(BlueprintImplementableEvent, Category = "Highlight")
	void OnStickIsNeutral();

	/** Blueprint function to handle confirm input */
	UFUNCTION(BlueprintImplementableEvent, Category = "Defaults")
	void OnSelect(int32 Selection);

	/** Blueprint function to handle canceling the menu */
	UFUNCTION(BlueprintImplementableEvent, Category = "Defaults")
	void OnCancel();

public:
	UFUNCTION(BlueprintCallable, Category = "Input")
	void OnNavigateX(float AxisValue);

	UFUNCTION(BlueprintCallable, Category = "Input")
	void OnNavigateY(float AxisValue);

	UFUNCTION(BlueprintPure, Category = "Highlight")
	bool IsSelectionHighlighted() const { return CurrentHighlightSection != INDEX_NONE; }

protected:
	// Current section of the radial select that is highlighted
	UPROPERTY(BlueprintReadOnly, Category = "Highlight")
	int32 CurrentHighlightSection;

	// Rotation controlled by right or left stick?
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	bool bUseRightStick;

	// The Diameter of the Widget
	UPROPERTY(EditDefaultsOnly, Category = "Defaults")
	float WidgetDiameter;

private:
	void CalculateAndUpdateHighlightRotation(FVector2D InVector);

	FVector2D StickPos;
};
