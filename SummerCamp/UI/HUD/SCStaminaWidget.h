// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCUserWidget.h"
#include "SCStaminaWidget.generated.h"

/**
 * 
 */
UCLASS()
class SUMMERCAMP_API USCStaminaWidget
: public USCUserWidget
{
	GENERATED_UCLASS_BODY()
	
public:
	UFUNCTION(BlueprintPure, Category = "Counselor|Stamina")
	float GetStamina() const;

	UFUNCTION(BlueprintPure, Category = "Counselor|Stamina")
	bool CanUseStamina() const;

	UFUNCTION(BlueprintPure, Category = "Counselor|Stamina")
	float GetCurrentFear() const;
};
