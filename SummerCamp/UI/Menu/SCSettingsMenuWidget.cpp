// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCSettingsMenuWidget.h"

#include "ILLPlayerSettingsSaveGame.h"

#include "SCLocalPlayer.h"
#include "SCWidgetBlueprintLibrary.h"

#define LOCTEXT_NAMESPACE "ILLFonic.F13.SCSettingsMenuWidget"

USCSettingsMenuWidget::USCSettingsMenuWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	ConfirmApplyText = LOCTEXT("ConfirmationMessage_ApplySettings", "Apply modified settings?");
	ConfirmBackText = LOCTEXT("ConfirmationMessage_DiscardSettings", "Discard modified settings?");
	ConfirmDefaultsText = LOCTEXT("ConfirmationMessage_RevertToDefaults", "Revert to default settings?");
}

void USCSettingsMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Setup delegates
	ConfirmApplyDelegate.BindDynamic(this, &USCSettingsMenuWidget::OnConfirmApply);
	ConfirmBackDelegate.BindDynamic(this, &USCSettingsMenuWidget::OnConfirmBack);
	ConfirmDefaultsDelegate.BindDynamic(this, &USCSettingsMenuWidget::OnConfirmDefaults);

	// Create our EditableSaveGame
	if (SaveGameClass)
	{
		USCLocalPlayer* LocalPlayer = CastChecked<USCLocalPlayer>(GetOwningLocalPlayer());
		DefaultSaveGame = LocalPlayer->CreateSettingsSave(SaveGameClass);
		CurrentSaveGame = LocalPlayer->GetLoadedSettingsSave(SaveGameClass);
		if (ensure(CurrentSaveGame))
			EditableSaveGame = CurrentSaveGame->SpawnDuplicate();

		SettingsLoaded();
	}
}

void USCSettingsMenuWidget::OnSettingsApply()
{
	if (HasModifiedSettings())
	{
		// Spawn a confirmation dialog and wait for confirmation
		USCWidgetBlueprintLibrary::ShowConfirmationDialog(GetOwningPlayerILLHUD(), ConfirmApplyDelegate, ConfirmApplyText, FText());
	}
	else
	{
		// Automatically close if there were no changes
		PlayTransitionOut();
	}
}

void USCSettingsMenuWidget::OnSettingsBack()
{
	if (HasModifiedSettings())
	{
		// Spawn a confirmation dialog and wait for confirmation
		USCWidgetBlueprintLibrary::ShowConfirmationDialog(GetOwningPlayerILLHUD(), ConfirmBackDelegate, ConfirmBackText, FText());
	}
	else
	{
		// Settings are clean, just pop
		PlayTransitionOut();
	}
}

void USCSettingsMenuWidget::OnSettingsDefaults()
{
	if (EditableSaveGame && EditableSaveGame->HasChangedFrom(DefaultSaveGame))
	{
		// Always confirm a set to defaults
		USCWidgetBlueprintLibrary::ShowConfirmationDialog(GetOwningPlayerILLHUD(), ConfirmDefaultsDelegate, ConfirmDefaultsText, FText());
	}
	else
	{
		// Do nothing when there are no differences from the default
	}
}

bool USCSettingsMenuWidget::HasModifiedSettings() const
{
	return (EditableSaveGame && CurrentSaveGame && EditableSaveGame->HasChangedFrom(CurrentSaveGame));
}

void USCSettingsMenuWidget::UpdateDynamicSettings()
{
	// Apply settings without saving them, which should also stop resolution changes from applying
	if (EditableSaveGame)
	{
		EditableSaveGame->ApplyPlayerSettings(GetOwningILLPlayer(), false);
	}
}

void USCSettingsMenuWidget::OnConfirmApply(bool bSelection)
{
	if (bSelection)
	{
		// Apply settings
		if (SaveGameClass && EditableSaveGame)
		{
			EditableSaveGame->ApplyPlayerSettings(GetOwningILLPlayer());

			// Update the CurrentSaveGame to the newly applied one, then spawn a new editable copy
			CurrentSaveGame = EditableSaveGame;
			EditableSaveGame = CurrentSaveGame->SpawnDuplicate();
		}

		// Notify Blueprint
		ApplySettings();

		// Automatically close
		PlayTransitionOut();
	}
}

void USCSettingsMenuWidget::OnConfirmBack(bool bSelection)
{
	if (bSelection)
	{
		if (CurrentSaveGame)
		{
			// Apply our CurrentSaveGame and write them out
			// This is to ensure the save in UpdateDynamicSettings is reset
			CurrentSaveGame->ApplyPlayerSettings(GetOwningILLPlayer());
		}

		// Notify Blueprint
		ResetSettings();

		// Automatically close
		PlayTransitionOut();
	}
}

void USCSettingsMenuWidget::OnConfirmDefaults(bool bSelection)
{
	if (bSelection)
	{
		if (SaveGameClass)
		{
			// Apply the settings from the default save
			DefaultSaveGame->ApplyPlayerSettings(GetOwningILLPlayer());

			// Now take it as current
			CurrentSaveGame = DefaultSaveGame;
			EditableSaveGame = CurrentSaveGame->SpawnDuplicate();
		}

		// Notify Blueprint
		DefaultSettings();
	}
}

#undef LOCTEXT_NAMESPACE
