// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "UI/ILLWidgetComponent.h"
#include "SCStatusIconComponent.generated.h"

/**
* @class USCStatusIconComponent
*/
UCLASS()
class SUMMERCAMP_API USCStatusIconComponent
: public UILLWidgetComponent
{
	GENERATED_BODY()

public:
	USCStatusIconComponent(const FObjectInitializer& ObjectInitializer);

	virtual void InitializeComponent() override;

	void ShowIcon();
	void HideIcon();

	UFUNCTION(BlueprintPure, Category="StatusIconComponent")
	FORCEINLINE bool IsHidden() const { return bIsHidden; }

protected:
	UPROPERTY()
	class USCStatusIconWidget* StatusIconWidget;

	// Set by ShowIcon/HideIcon, defaults to true
	bool bIsHidden;
};
