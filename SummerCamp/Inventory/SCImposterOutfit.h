// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCSpecialItem.h"
#include "SCImposterOutfit.generated.h"

/**
* @class ASCSpecialItem
*/
UCLASS()
class SUMMERCAMP_API ASCImposterOutfit
	: public ASCSpecialItem
{
	GENERATED_UCLASS_BODY()

	virtual void BeginPlay() override;
	virtual void OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod) override;

	UFUNCTION()
	void OnShowIcon();

protected:
	UPROPERTY(EditDefaultsOnly)
	float ShowMaskTime;


	UPROPERTY(Transient, Replicated)
	bool bWasUsed;

	UPROPERTY()
	FTimerHandle ShowMinimapIconTimerHandle;
};
