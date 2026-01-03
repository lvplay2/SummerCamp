// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCDialogWidget.h"
#include "SCErrorDialogWidget.generated.h"

/** Error dialog delegate. */
DECLARE_DYNAMIC_DELEGATE(FSCOnAcceptedErrorDialogDelegate);

/**
 * @class USCErrorDialogWidget
 * @brief Error dialog popup menu.
 */
UCLASS(Abstract)
class SUMMERCAMP_API USCErrorDialogWidget
: public USCDialogWidget
{
	GENERATED_UCLASS_BODY()

public:
	// Text to display on the yes button
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Transient)
	FText AcceptButtonText;

	// Error message, if any
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Transient)
	FText ErrorText;

	// Callback delegate
	UPROPERTY(Transient)
	FSCOnAcceptedErrorDialogDelegate Callback;

protected:
	/** Blueprint callback for Accept. */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category="Menu|Dialogs", meta=(BlueprintProtected))
	void OnAccept();
};
