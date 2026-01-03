// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCDialogWidget.h"
#include "SCConfirmationDialogWidget.generated.h"

/** Confirmation dialog delegate. */
DECLARE_DYNAMIC_DELEGATE_OneParam(FSCOnConfirmedDialogDelegate, bool, bSelection);

/**
 * @class USCConfirmationDialogWidget
 * @brief Confirmation dialog popup menu.
 */
UCLASS(Abstract)
class SUMMERCAMP_API USCConfirmationDialogWidget
: public USCDialogWidget
{
	GENERATED_UCLASS_BODY()

public:
	// Text to display on the confirm button
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Transient)
	FText ConfirmButtonText;

	// Text to display on the cancel button
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Transient)
	FText CancelButtonText;

	// Confirmation message, if any
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Transient)
	FText ConfirmationText;

	// Callback delegate
	UPROPERTY(Transient)
	FSCOnConfirmedDialogDelegate Callback;

protected:
	/** Blueprint callback for No. */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category="Menu|Dialogs", meta=(BlueprintProtected))
	void OnCancel();

	/** Blueprint callback for Yes. */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category="Menu|Dialogs", meta=(BlueprintProtected))
	void OnConfirm();
};
