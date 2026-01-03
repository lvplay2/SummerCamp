// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Blueprint/UserWidget.h"
#include "Virtual_Cabin/SCGVCItem.h"
#include "SCUserWidget.h"
#include "SCGVCInventoryWidget.generated.h"

/**
 * 
 */
UCLASS()
class SUMMERCAMP_API USCGVCInventoryWidget
: public USCUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "VC UI")
	void OnShow();

	UFUNCTION(BlueprintImplementableEvent, Category = "VC UI")
	void OnHide();

	UFUNCTION(BlueprintImplementableEvent, Category = "VC UI")
	void OnShowButtonPanel(bool showActionButton, bool showRotationButton, bool showInfoButton, bool showBackButton);

	UFUNCTION(BlueprintImplementableEvent, Category = "VC UI")
	void OnHideButtonPanel();

	UFUNCTION(BlueprintImplementableEvent, Category = "VC UI")
	void OnShowInventoryButtonPanel();

	UFUNCTION(BlueprintImplementableEvent, Category = "VC UI")
	void OnHideInventoryButtonPanel();

	UFUNCTION(BlueprintImplementableEvent, Category = "VC UI")
	void OnShowDescriptionText(const FString& newtitle, const FString& newtext);

	UFUNCTION(BlueprintImplementableEvent, Category = "VC UI")
	void OnHideDescriptionText(bool instant);

	UFUNCTION(BlueprintImplementableEvent, Category = "VC UI")
	void OnShowItemGreenScreen();

	UFUNCTION(BlueprintImplementableEvent, Category = "VC UI")
	void OnHideItemGreenScreen();

	UFUNCTION(BlueprintImplementableEvent, Category = "VC UI")
	void OnPressUpButton();

	UFUNCTION(BlueprintImplementableEvent, Category = "VC UI")
	void OnPressDownButton();

	UFUNCTION(BlueprintImplementableEvent, Category = "VC UI")
	void OnPressLeftButton();

	UFUNCTION(BlueprintImplementableEvent, Category = "VC UI")
	void OnPressRightButton();

	UFUNCTION(BlueprintImplementableEvent, Category = "VC UI")
	void OnPressActionButton();

	UFUNCTION(BlueprintImplementableEvent, Category = "VC UI")
	void OnPlaySaveAnimation();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<ASCGVCItem*> m_ItemsArray;

	UFUNCTION(BlueprintCallable, Category = "VC UI")
	UHorizontalBox* CreateNewHorizontalBox();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC UI|Button Info Panel")
	FText m_UseButtonInfoText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC UI|Button Info Panel")
	FText m_BackButtonInfoText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC UI|Button Info Panel")
	FText m_RotationButtonInfoText;
};
