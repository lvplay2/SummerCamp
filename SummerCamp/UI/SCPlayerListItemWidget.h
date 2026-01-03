// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLPlayerListItemWidget.h"
#include "SCPlayerListItemWidget.generated.h"

/**
 * @class USCPlayerListItemWidget
 */
UCLASS()
class SUMMERCAMP_API USCPlayerListItemWidget
: public UILLPlayerListItemWidget
{
	GENERATED_UCLASS_BODY()
		
protected:
	/** Should the Picked Character Panel be visible? */
	UFUNCTION(BlueprintPure, Category="Visibility")
	ESlateVisibility GetPickedCharacterPanelVisibility() const;

	/** Should the Ready Box Panel be visible? */
	UFUNCTION(BlueprintPure, Category="Visibility")
	ESlateVisibility GetReadBoxPanelVisibility() const;

	/** Should the Host arrow be visible? */
	UFUNCTION(BlueprintPure, Category="Visibility")
	ESlateVisibility GetHostArrowImageVisibility() const;

	/** Should the Character Select Panel be visible? */
	UFUNCTION(BlueprintPure, Category="Visibility")
	ESlateVisibility GetCharacterSelectPanelVisibility() const;

	/** Should the Player Status Panel be visible? */
	UFUNCTION(BlueprintPure, Category="Visibility")
	ESlateVisibility GetPlayerStatusPanelVisibility() const;

	/** Should the Spectator Panel be visible? */
	UFUNCTION(BlueprintPure, Category="Visibility")
	ESlateVisibility GetSpectatingPanelVisibility() const;

	/** Should the Alive/Dead Panel be visible? */
	UFUNCTION(BlueprintPure, Category="Visibility")
	ESlateVisibility GetAliveDeadPanelVisiblity() const;

	/** Should the Killer Picker Button be visible? */
	UFUNCTION(BlueprintPure, Category="Visibility")
	ESlateVisibility GetKillerPickerButtonVisibility() const;

	/** @return Visibility state for a walkie-talkie speaking remote player. */
	UFUNCTION(BlueprintPure, Category="Visibility")
	ESlateVisibility GetRemoteInRangeOrWalkieSpeakerVisibility() const;

	/** @return Visibility state for a player with a walkie-talkie when the local player has one too. */
	UFUNCTION(BlueprintPure, Category="Visibility")
	ESlateVisibility GetMutualWalkieVisibility() const;

	/** Returns player status as FText */
	UFUNCTION(BlueprintPure, Category="DisplayText")
	FText GetStatusDisplayText() const;

	UPROPERTY(BlueprintReadWrite, Category="Gameplay")
	bool bSpectating;
};
