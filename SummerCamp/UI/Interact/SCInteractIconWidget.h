// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCUserWidget.h"
#include "SCInteractIconWidget.generated.h"

/**
 * @class USCInteractIconWidget
 */
UCLASS()
class SUMMERCAMP_API USCInteractIconWidget
: public USCUserWidget
{
	GENERATED_UCLASS_BODY()
public:

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Animation")
	void ShowIcons();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Animation")
	void HideIcons();

	UPROPERTY(BlueprintReadWrite, Category = "Parent")
	AActor* OwningActor;

	UPROPERTY(BlueprintReadWrite, Category = "Interaction")
	bool bHasHoldInteraction;
};
