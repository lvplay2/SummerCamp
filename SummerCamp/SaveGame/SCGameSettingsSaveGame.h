// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLGameSettingsSaveGame.h"
#include "SCGameSettingsSaveGame.generated.h"

/**
 * @class USCGameSettingsSaveGame
 */
UCLASS()
class USCGameSettingsSaveGame
: public UILLGameSettingsSaveGame
{
	GENERATED_UCLASS_BODY()

	// Begin UILLPlayerSettingsSaveGame interface
	virtual void ApplyPlayerSettings(AILLPlayerController* PlayerController, const bool bAndSave = true) override;
	virtual bool HasChangedFrom(UILLPlayerSettingsSaveGame* OtherSave) const override;
	// End UILLPlayerSettingsSaveGame interface

	// Automatically rotate the minimap with the player orientation?
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Game")
	bool bRotateMinimapWithPlayer;

	// Show hint popups?
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Game")
	bool bShowHelpText;
};
