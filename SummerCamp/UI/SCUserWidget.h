// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLUserWidget.h"
#include "SCUserWidget.generated.h"

class ASCHUD;

/**
 * @class USCUserWidget
 */
UCLASS(Abstract)
class SUMMERCAMP_API USCUserWidget
: public UILLUserWidget
{
	GENERATED_UCLASS_BODY()

	/** @return SCHUD instance for our OwningPlayer. */
	UFUNCTION(BlueprintPure, Category = "HUD")
	ASCHUD* GetOwningPlayerSCHUD() const;

	/** Wraps to ILLMenuStackHUDComponent::GetCurrentMenu. */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	UILLUserWidget* GetCurrentMenu() const;
};
