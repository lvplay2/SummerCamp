// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once
#include "../SCUserWidget.h"
#include "SCClothingData.h"
#include "SCSwatchListWidget.generated.h"

/** Confirmation dialog delegate. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSCOnSwatchSelectedDelegate, TSubclassOf<class USCClothingSlotMaterialOption>, SelectedSwatch);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSCOnLockedSwatchSelected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSCOnSwatchHighlighted, FString, HighlightedText, bool, Locked);

/**
* @class USCSwatchListWidget
*/
UCLASS()
class SUMMERCAMP_API USCSwatchListWidget
	: public USCUserWidget
{
	GENERATED_UCLASS_BODY()
public:

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "List")
	void InitializeList(const FSCClothingSlot& ClothingSlot, FSCClothingMaterialPair SelectedSwatch);

	UFUNCTION()
	void OnButtonSelected(class USCSwatchButton* SelectedSwatch);

	UFUNCTION()
	void OnButtonHighlighted(class USCSwatchButton* HighlightedButton);

	UPROPERTY(BlueprintAssignable)
	FSCOnSwatchSelectedDelegate OnItemSelected;

	UPROPERTY(BlueprintAssignable)
	FSCOnLockedSwatchSelected OnLockedSelected;

	UPROPERTY(BlueprintAssignable)
	FSCOnSwatchHighlighted OnSwatchHighlighted;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void SelectCurrentHighlightedButton();

	UFUNCTION(BlueprintPure, Category = "UI")
	UTexture2D* GetSlotIcon() const;

protected:
	UFUNCTION()
	void LockedSelected();

	UPROPERTY(EditDefaultsOnly, Category = "List")
	TSubclassOf<class USCSwatchButton> ButtonClass;

	UPROPERTY(EditDefaultsOnly, Category = "List")
	TSubclassOf<class USCUserWidget> EmptyButtonClass;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UWrapBox* WrapBox;

	UPROPERTY(Transient)
	class USCSwatchButton* CurrentSelectedButton;

	UPROPERTY(Transient)
	class USCSwatchButton* CurrentHighlightedButton;

	UPROPERTY()
	FSCClothingSlot CurrentClothingSlot;

};