// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCMenuWidget.h"
#include "SCDialogWidget.generated.h"

/**
 * @class USCDialogWidget
 * @brief Popup style menu.
 */
UCLASS(Abstract)
class SUMMERCAMP_API USCDialogWidget
: public USCMenuWidget
{
	GENERATED_UCLASS_BODY()

	// Header message
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Transient)
	FText HeaderText;

	/** Called after creating one of these dialogs to update the header. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category="User Interface", meta=(BlueprintProtected))
	void OnPostSpawn();
};
