// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "UI/SCUserWidget.h"
#include "SCGVCDebugMenuWidget.generated.h"

/**
 * 
 */
UCLASS()
class SUMMERCAMP_API USCGVCDebugMenuWidget
: public USCUserWidget
{
	GENERATED_BODY()

public:
	USCGVCDebugMenuWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintImplementableEvent, Category = "VC UI")
	void OnShow();

	UFUNCTION(BlueprintImplementableEvent, Category = "VC UI")
	void OnHide();

	UFUNCTION(BlueprintImplementableEvent, Category = "VC UI")
	void OnPressUpButton();

	UFUNCTION(BlueprintImplementableEvent, Category = "VC UI")
	void OnPressDownButton();

	UFUNCTION(BlueprintImplementableEvent, Category = "VC UI")
	void OnPressActionButton();
};
