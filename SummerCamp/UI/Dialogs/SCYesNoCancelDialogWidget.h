// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCDialogWidget.h"
#include "SCYesNoCancelDialogWidget.generated.h"

/**
 * @enum ESCYesNoDialogResult
 */
UENUM(BlueprintType)
enum class ESCYesNoDialogResult : uint8
{
	Yes,
	No,
	Cancel
};

/** Confirmation dialog delegate. */
DECLARE_DYNAMIC_DELEGATE_OneParam(FSCOnYesNoCancelDialogDelegate, ESCYesNoDialogResult, Selection);

/**
 * @class USCYesNoCancelDialogWidget
 * @brief Confirmation dialog popup menu.
 */
UCLASS(Abstract)
class SUMMERCAMP_API USCYesNoCancelDialogWidget
: public USCDialogWidget
{
	GENERATED_UCLASS_BODY()

public:
	// Confirmation message, if any
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Transient)
	FText MessageText;

	// Text to display on the yes button
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Transient)
	FText YesButtonText;

	// Text to display on the no button
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Transient)
	FText NoButtonText;

	// Text to display on the cancel button
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Transient)
	FText CancelButtonText;

	// Callback delegate
	UPROPERTY(BlueprintReadOnly, Transient)
	FSCOnYesNoCancelDialogDelegate Callback;

protected:
	/** Blueprint accessible callback. */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category="Menu|Dialogs", meta=(BlueprintProtected))
	void Broadcast(ESCYesNoDialogResult Result);
};
