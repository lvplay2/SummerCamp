// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "UI/SCUserWidget.h"
#include "SCStatusIconWidget.generated.h"

/**
* @class USCStatusIconWidget
*/
UCLASS()
class SUMMERCAMP_API USCStatusIconWidget
: public USCUserWidget
{
	GENERATED_BODY()

public:
	USCStatusIconWidget(const FObjectInitializer& ObjectInitializer);

	/** Blueprint function to perform logic and animations when hiding the status icon. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Icon")
	void HideIcon();

	/** Blueprint function to perform logic and animations when un-hiding the status icon. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Icon")
	void ShowIcon();
};
