// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once
#include "../SCUserWidget.h"
#include "SCSwatchButton.generated.h"

/** Confirmation dialog delegate. */
DECLARE_DYNAMIC_DELEGATE_OneParam(FSCOnClickedSwatchDelegate, class USCSwatchButton*, SelectedButton);
DECLARE_DYNAMIC_DELEGATE(FSCOnClickedLockedSwatch);
DECLARE_DYNAMIC_DELEGATE_OneParam(FSCOnHighlightedSwatchDelegate, class USCSwatchButton*, HighlightedButton);

/**
* @class USCSwatchButton
*/
UCLASS()
class SUMMERCAMP_API USCSwatchButton
	: public USCUserWidget
{
	GENERATED_UCLASS_BODY()
public:

	virtual void RemoveFromParent() override;

	UFUNCTION(BlueprintCallable, Category = "Init")
	void InitializeButton(TSubclassOf<USCClothingSlotMaterialOption> _SwatchClass, bool Unlocked = true, bool ShowStore = false);

	UFUNCTION(BlueprintPure, Category = "UI")
	UTexture2D* GetButtonImage() const;

	UPROPERTY(Transient)
	FSCOnClickedSwatchDelegate OnClickedCallback;

	UPROPERTY(Transient)
	FSCOnClickedLockedSwatch OnClickedLockedCallback;

	UPROPERTY(Transient)
	FSCOnHighlightedSwatchDelegate OnHighlightedCallback;

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
	TSubclassOf<USCClothingSlotMaterialOption> GetSwatchClass() const { return SwatchClass; }

	UFUNCTION(BlueprintPure, Category = "UI")
	FString GetHighlightText() const;

	UFUNCTION()
	bool IsLocked() const { return bLocked; }

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Swatch")
	TSubclassOf<USCClothingSlotMaterialOption> SwatchClass;

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
};