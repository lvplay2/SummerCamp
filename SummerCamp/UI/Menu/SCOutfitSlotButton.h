// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once
#include "../SCUserWidget.h"
#include "SCClothingData.h"
#include "SCOutfitSlotButton.generated.h"

/** Confirmation dialog delegate. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSCOnSlotSelectedDelegate, FSCClothingSlot, SelectedSlotData);

/**
* @class USCOutfitSlotButton
*/
UCLASS()
class SUMMERCAMP_API USCOutfitSlotButton
	: public USCUserWidget
{
	GENERATED_UCLASS_BODY()
public:

	virtual void RemoveFromParent() override;

	UFUNCTION(BlueprintCallable, Category = "Init")
	void InitSlot(const FSCClothingSlot& SlotData);

	UFUNCTION(BlueprintPure, Category = "UI")
	UTexture2D* GetSlotIcon() const;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FSCOnSlotSelectedDelegate OnSlotSelectedCallback;

protected:
	UFUNCTION(BlueprintCallable, Category = "UI")
	void OnClicked();

private:
	FSCClothingSlot ClothingSlotData;
};
