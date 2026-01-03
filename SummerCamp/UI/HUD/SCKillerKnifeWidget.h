// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "UI/SCUserWidget.h"
#include "SCKillerKnifeWidget.generated.h"

/**
* @class USCKillerKnifeWidget
*/
UCLASS()
class SUMMERCAMP_API USCKillerKnifeWidget
	: public USCUserWidget
{
	GENERATED_BODY()

public:
	USCKillerKnifeWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintPure, Category = "Knife")
	int32 GetKnifeCount() const;

	UFUNCTION(BlueprintPure, Category = "Knife")
	FLinearColor GetActiveColor() const;

	UFUNCTION(BlueprintPure, Category = "Knife")
	ESlateVisibility GetUseActionMapVisibility() const;

	UFUNCTION(BlueprintPure, Category = "Knife")
	ESlateVisibility GetCancelActionMapVisibility() const;

	UFUNCTION(BlueprintPure, Category = "Knife")
	FLinearColor GetActionMapColor() const;

	UPROPERTY(EditDefaultsOnly, Category = "Knife")
	FLinearColor ActiveColor;

	UPROPERTY(EditDefaultsOnly, Category = "Knife")
	FLinearColor RechargeColor;

	UPROPERTY(EditDefaultsOnly, Category = "Knife")
	FLinearColor EmptyColor;

	UPROPERTY(EditDefaultsOnly, Category = "Knife")
	FLinearColor ActiveActionMapColor;

	UPROPERTY(EditDefaultsOnly, Category = "Knife")
	FLinearColor InactiveActionMapColor;

};