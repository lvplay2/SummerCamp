// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "UI/SCUserWidget.h"
#include "SCCounselorInventoryWidget.generated.h"

/**
* @class USCCounselorInventoryWidget
*/
UCLASS()
class SUMMERCAMP_API USCCounselorInventoryWidget
	: public USCUserWidget
{
	GENERATED_BODY()

public:
	USCCounselorInventoryWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintPure, Category = "InputVisibility")
	ESlateVisibility GetSlot1UseVisibility() const;

	UFUNCTION(BlueprintPure, Category = "InputVisibility")
	ESlateVisibility GetSlot2UseVisibility() const;

	UFUNCTION(BlueprintPure, Category = "InputVisibility")
	ESlateVisibility GetSlot3UseVisibility() const;

	UFUNCTION(BlueprintPure, Category = "InputVisibility")
	ESlateVisibility GetSlot1BackgroundVisibility() const;

	UFUNCTION(BlueprintPure, Category = "InputVisibility")
	ESlateVisibility GetSlot2BackgroundVisibility() const;

	UFUNCTION(BlueprintPure, Category = "InputVisibility")
	ESlateVisibility GetSlot3BackgroundVisibility() const;

	UFUNCTION(BlueprintPure, Category = "InputVisibility")
	ESlateVisibility GetCycleButtonsVisibility() const;

private:
	ESlateVisibility GetSlotVisibility(int ItemSlot) const;
	ESlateVisibility GetSlotBackgroundVisibility(int ItemSlot) const;
};
