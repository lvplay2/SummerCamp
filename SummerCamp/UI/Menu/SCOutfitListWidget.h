// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once
#include "../SCUserWidget.h"
#include "SCOutfitListWidget.generated.h"

class USCClothingData;
class USCOutfitButton;

/** Confirmation dialog delegate. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSCOnItemSelectedDelegate, TSubclassOf<USCClothingData>, SelectedOutfitClass, bool, bFromClick);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSCSelectedLockedOutfit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSCOnOutfitHighlighted, FString, HighlightedText, bool, Locked);

/**
* @class USCOutfitListWidget
*/
UCLASS()
class SUMMERCAMP_API USCOutfitListWidget
	: public USCUserWidget
{
	GENERATED_UCLASS_BODY()
public:

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "List")
	void InitializeList(TSoftClassPtr<ASCCounselorCharacter> CounselorClass, TSubclassOf<USCClothingData> OutfitClass, USCCounselorWardrobeWidget* WardrobeParent);

	UFUNCTION()
	void OnButtonSelected(USCOutfitButton* SelectedOutfit);

	UFUNCTION()
	void OnButtonHighlighted(USCOutfitButton* HighlightedButton);

	UPROPERTY(BlueprintAssignable)
	FSCOnItemSelectedDelegate OnItemSelected;

	UPROPERTY(BlueprintAssignable)
	FSCSelectedLockedOutfit OnLockedSelected;

	UPROPERTY(BlueprintAssignable)
	FSCOnOutfitHighlighted OnOutfitHighlighted;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void SelectCurrentHighlightedButton();

	void SelectByOutfitClass(TSubclassOf<USCClothingData> OutfitClass, bool bAndNotify = true);

protected:

	UFUNCTION()
	void LockedSelected();

	UPROPERTY(EditDefaultsOnly, Category = "List")
	TSubclassOf<USCOutfitButton> ButtonClass;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	UScrollBox* ScrollBox;

	UPROPERTY(Transient, BlueprintReadOnly)
	USCOutfitButton* CurrentSelectedButton;

	UPROPERTY(Transient, BlueprintReadOnly)
	USCOutfitButton* CurrentHighlightedButton;

	UPROPERTY()
	USCCounselorWardrobeWidget* ParentWidget;

};