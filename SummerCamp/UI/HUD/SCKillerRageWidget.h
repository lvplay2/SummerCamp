// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "UI/SCUserWidget.h"
#include "SCKillerRageWidget.generated.h"

/**
* @class USCKillerRageWidget
*/
UCLASS()
class SUMMERCAMP_API USCKillerRageWidget
	: public USCUserWidget
{
	GENERATED_BODY()

public:
	USCKillerRageWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintPure, Category = "Visibility")
	float GetRagePercentage() const;

	UFUNCTION(BlueprintPure, Category = "Fill Color")
	FLinearColor GetFillColor() const;

	UPROPERTY(EditDefaultsOnly, Category = "Fill Color")
	FLinearColor LockedColor;

	UPROPERTY(EditDefaultsOnly, Category = "Fill Color")
	FLinearColor UnlockedColor;
};
