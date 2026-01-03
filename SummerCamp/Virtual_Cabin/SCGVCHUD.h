// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCHUD.h"
#include "Virtual_Cabin/SCGVCInventoryWidget.h"
#include "Virtual_Cabin/SCGVCDebugMenuWidget.h"
#include "SCGVCHUD.generated.h"

/**
 * @class ASCGVCHUD
 */
UCLASS()
class SUMMERCAMP_API ASCGVCHUD
: public ASCHUD
{
	GENERATED_UCLASS_BODY()

	// Begin AActor interface
	virtual void BeginPlay() override;
	// End AActor interface

public:
	/** Opens PauseMenuClass. */
	void ShowPauseMenu();
	
	void HidePauseMenu();

	void HideInventory();

	void ShowInventory();

	void ShowDebugMenu();

	void HideDebugMenu();

	UFUNCTION(BlueprintCallable, Category = "VC HUD")
	bool IsDescriptionVisible();

	UFUNCTION(BlueprintCallable, Category = "VC HUD")
	void HandleInventoryUp();

	UFUNCTION(BlueprintCallable, Category = "VC HUD")
	void HandleInventoryDown();

	UFUNCTION(BlueprintCallable, Category = "VC HUD")
	void HandleInventoryLeft();

	UFUNCTION(BlueprintCallable, Category = "VC HUD")
	void HandleInventoryRight();

	UFUNCTION(BlueprintCallable, Category = "VC HUD")
	void HandleInventoryAction();

	UFUNCTION(BlueprintCallable, Category = "VC HUD")
	void HandleDebugMenuDown();

	UFUNCTION(BlueprintCallable, Category = "VC HUD")
	void HandleDebugMenuUp();

	UFUNCTION(BlueprintCallable, Category = "VC HUD")
	void HandleDebugMenuAction();

	UFUNCTION(BlueprintCallable, Category = "VC HUD")
	void ShowButtonInfo(bool showActionButton, bool showRotationButton, bool showInfoButton, bool showBackButton);

	UFUNCTION(BlueprintCallable, Category = "VC HUD")
	void HideButtoninfo();

	UFUNCTION(BlueprintCallable, Category = "VC HUD")
	void ShowInventoryButtonInfo();

	UFUNCTION(BlueprintCallable, Category = "VC HUD")
	void HideInventoryButtoninfo();

	UFUNCTION(BlueprintCallable, Category = "VC HUD")
	void ShowDescriptionText(const FString& newtitle, const FString& newtext);

	UFUNCTION(BlueprintCallable, Category = "VC HUD")
	void HideDescriptionText(bool instant = false);

	UFUNCTION(BlueprintCallable, Category = "VC HUD")
	void PlayUISaveAnimation();

	void SetUseAndBackInfoButtonsText(FText usetext, FText backtext);

	void ResetUseAndBackInfoButtonsText();

	UPROPERTY()
	USCGVCInventoryWidget* m_InvWidgetRef;

protected:
	bool m_IsDebugMenuOpen;
	bool m_IsInventoryOpen;
	bool m_IsDescriptionVisible;

	// button info panel
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Button Info Panel")
	FText m_DefaultUseButtonInfoText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Button Info Panel")
	FText m_DefaultBackButtonInfoText;

	UPROPERTY()
	USCGVCDebugMenuWidget* m_DebugMenuWidgetRef;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<USCGVCInventoryWidget> m_InvWidgetBP;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<USCGVCDebugMenuWidget> m_DebugMenuBP;

	// Class of our in-game pause menu
	UPROPERTY()
	TSubclassOf<USCMenuWidget> PauseMenuClass;
};