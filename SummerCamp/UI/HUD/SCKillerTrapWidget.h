// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "UI/SCUserWidget.h"
#include "SCKillerTrapWidget.generated.h"

/**
* @class USCKillerTrapWidget
*/
UCLASS()
class SUMMERCAMP_API USCKillerTrapWidget
	: public USCUserWidget
{
	GENERATED_BODY()

public:
	USCKillerTrapWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintPure, Category = "trap")
	int32 GetTrapCount() const;

	UFUNCTION(BlueprintPure, Category = "Knife")
	FLinearColor GetActiveColor() const;

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