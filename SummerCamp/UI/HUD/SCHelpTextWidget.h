// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCUserWidget.h"
#include "SCHelpTextWidget.generated.h"

/**
*
*/
UCLASS()
class SUMMERCAMP_API USCHelpTextWidget
	: public USCUserWidget
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "HelpText")
	bool ShouldShowHelpText() const;

	/** The level at which the help text stops showing up. */
	UPROPERTY(EditDefaultsOnly, Category = "HelpText")
	int32 MaxLevelForHelpText;
};