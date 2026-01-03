// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCUserWidget.h"
#include "SCObjectiveWidget.generated.h"

/**
*
*/
UCLASS()
class SUMMERCAMP_API USCObjectiveWidget
	: public USCUserWidget
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Objective")
	void UpdateObjectives();
};
