// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCMenuWidget.h"
#include "SCConfirmationDialogWidget.h"
#include "SCSettingsMenuWidget.generated.h"

class UILLPlayerSettingsSaveGame;

/**
 * @class USCSettingsMenuWidget
 * @brief Base class for settings menus.
 */
UCLASS(Abstract)
class SUMMERCAMP_API USCSettingsMenuWidget
: public USCMenuWidget
{
	GENERATED_UCLASS_BODY()

	// Begin UUserWidget interface
	virtual void NativeConstruct() override;
	// End UUserWidget interface

	/** Menu helper: Confirms if you want to apply changes (if any) and does so. */
	UFUNCTION(BlueprintCallable, Category="Menu|Settings")
	void OnSettingsApply();

	/** Menu helper: Confirms if you want to lose changes (if any) and does so. */
	UFUNCTION(BlueprintCallable, Category="Menu|Settings")
	void OnSettingsBack();

	/** Menu helper: Confirms if you want to set values to defaults. */
	UFUNCTION(BlueprintCallable, Category="Menu|Settings")
	void OnSettingsDefaults();

protected:
	/** Call to the widget to handle applying the actual settings. */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category="Menu|Settings", meta=(BlueprintProtected))
	bool HasModifiedSettings() const;

	/** Tentatively applies all dynamic (non-resolution) settings on our player save. */
	UFUNCTION(BlueprintCallable, Category="Menu|Settings")
	void UpdateDynamicSettings();

	/** Call to the widget to handle applying the actual settings. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category="Menu|Settings", meta=(BlueprintProtected))
	void ApplySettings();

	/** Call to the widget to handle setting the actual settings to default values. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category="Menu|Settings", meta=(BlueprintProtected))
	void DefaultSettings();

	/** Call to the widget to handle resetting the actual settings. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category="Menu|Settings", meta=(BlueprintProtected))
	void ResetSettings();

	/** Call to the widget when settings are loaded to update the UI. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category="Menu|Settings", meta=(BlueprintProtected))
	void SettingsLoaded();

	/** Callback for the confirmation dialog spawned in OnSettingsApply. */
	UFUNCTION()
	void OnConfirmApply(bool bSelection);

	/** Callback for the confirmation dialog spawned in OnSettingsBack. */
	UFUNCTION()
	void OnConfirmBack(bool bSelection);

	/** Callback for the confirmation dialog spawned in OnSettingsDefaults. */
	UFUNCTION()
	void OnConfirmDefaults(bool bSelection);

	// Save game class to use
	UPROPERTY(EditDefaultsOnly, Category="Menu|Settings")
	TSubclassOf<UILLPlayerSettingsSaveGame> SaveGameClass;

	// Current player settings
	UPROPERTY(BlueprintReadOnly, Transient, Category="Menu|Settings")
	UILLPlayerSettingsSaveGame* CurrentSaveGame;

	// Default values save game instance
	UPROPERTY(BlueprintReadOnly, Transient, Category="Menu|Settings")
	UILLPlayerSettingsSaveGame* DefaultSaveGame;

	// Duplicate of the current player settings for editing and applying later
	UPROPERTY(BlueprintReadOnly, Transient, Category="Menu|Settings")
	UILLPlayerSettingsSaveGame* EditableSaveGame;

	// Text to display when OnSettingsApply hits and there are changes detected
	UPROPERTY(EditDefaultsOnly, Category="Menu|Settings")
	FText ConfirmApplyText;

	// Text to display when OnSettingsBack hits and there are changes detected
	UPROPERTY(EditDefaultsOnly, Category="Menu|Settings")
	FText ConfirmBackText;

	// Text to display when OnSettingsDefaults hits and there are changes detected
	UPROPERTY(EditDefaultsOnly, Category="Menu|Settings")
	FText ConfirmDefaultsText;

	// Should this menu apply to video or non-video settings?
	UPROPERTY(EditDefaultsOnly, Category="Menu|Settings")
	bool bAffectsResolutionSettings;

	// Delegate handle for OnConfirmApply
	UPROPERTY()
	FSCOnConfirmedDialogDelegate ConfirmApplyDelegate;

	// Delegate handle for OnConfirmBack
	UPROPERTY()
	FSCOnConfirmedDialogDelegate ConfirmBackDelegate;

	// Delegate handle for OnConfirmDefaults
	UPROPERTY()
	FSCOnConfirmedDialogDelegate ConfirmDefaultsDelegate;
};
