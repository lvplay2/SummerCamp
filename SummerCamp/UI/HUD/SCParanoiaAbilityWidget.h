// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "UI/SCUserWidget.h"
#include "SCParanoiaAbilityWidget.generated.h"

/**
* @class USCParanoiaAbilityWidget
*/
UCLASS()
class SUMMERCAMP_API USCParanoiaAbilityWidget
	: public USCUserWidget
{
	GENERATED_BODY()

public:
	USCParanoiaAbilityWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Percent")
	float GetUseAbilityPercentage() const;
};