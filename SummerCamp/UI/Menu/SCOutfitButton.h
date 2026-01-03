// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once
#include "../SCUserWidget.h"
#include "SCOutfitButton.generated.h"

/** Confirmation dialog delegate. */
DECLARE_DYNAMIC_DELEGATE_OneParam(FSCOnClickedOutfitDelegate, class USCOutfitButton*, SelectedButton);
DECLARE_DYNAMIC_DELEGATE(FSCOnClickedLockedOutfit);
DECLARE_DYNAMIC_DELEGATE_OneParam(FSCOnHighlightedOutfitDelegate, class USCOutfitButton*, HighlightedButton);

/**
* @class USCOutfitButton
*/
UCLASS()
class SUMMERCAMP_API USCOutfitButton
	: public USCUserWidget
{
	GENERATED_UCLASS_BODY()
public:

	virtual void RemoveFromParent() override;

	UFUNCTION(BlueprintCallable, Category = "Init")
	void InitializeButton(TSubclassOf<USCClothingData> _OutfitClass, USCCounselorWardrobeWidget* OwningMenu, bool Unlocked = true, bool ShowStore = false);

	UFUNCTION(BlueprintPure, Category = "UI")
	UTexture2D* GetButtonImage() const;

	UPROPERTY(Transient)
	FSCOnClickedOutfitDelegate OnClickedCallback;

	UPROPERTY(Transient)
	FSCOnClickedLockedOutfit OnClickedLockedCallback;

	UPROPERTY(Transient)
	FSCOnHighlightedOutfitDelegate OnHighlightedCallback;

	UFUNCTION(BlueprintPure, Category = "UI")
	ESlateVisibility GetSelectedVisibility() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	ESlateVisibility GetLockedVisibility() const;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetSelected(bool bSelected) { bIsSelected = bSelected; }

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void HighlightButton();

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void UnHighlightButton();

	UFUNCTION()
	TSubclassOf<USCClothingData> GetOutfitClass() const { return OutfitClass; }

	UFUNCTION(BlueprintPure, Category = "UI")
	FString GetHighlightText() const;

	UFUNCTION()
	bool IsLocked() const { return bLocked; }

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Outfit")
	TSubclassOf<USCClothingData> OutfitClass;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void OnClicked();

	UPROPERTY(BlueprintReadOnly, Transient, Category = "UI")
	bool bIsSelected;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void OnHighlighted();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void OnUnHighlighted();

	UPROPERTY(BlueprintReadOnly, Transient, Category = "UI")
	bool bLocked;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "UI")
	bool bShowStore;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "UI")
	bool bHighlighted;

	UPROPERTY(Transient)
	int32 UnlockLevel;

	UPROPERTY(Transient)
	FString DescriptionText;

	USCCounselorWardrobeWidget* ParentMenu;
};