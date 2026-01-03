// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "SCConfirmationDialogWidget.h"
#include "SCErrorDialogWidget.h"
#include "SCYesNoCancelDialogWidget.h"
#include "SCWidgetBlueprintLibrary.generated.h"

/**
 * @class USCWidgetBlueprintLibrary
 */
UCLASS()
class SUMMERCAMP_API USCWidgetBlueprintLibrary
: public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	//////////////////////////////////////////////////
	// Dialog menu spawning
public:
	/**
	 * Displays a error dialog using the first menu stack component from HUD.
	 *
	 * @param HUD HUD with a menu stack to spawn this in.
	 * @param Delegate Callback delegate.
	 * @param UserErrorText Error text.
	 * @param HeaderText (Optional) Header text.
	 * @return Error dialog spawned.
	 */
	UFUNCTION(BlueprintCallable, Category = "Menu|Dialogs")
	static USCErrorDialogWidget* ShowErrorDialog(AILLHUD* HUD, FSCOnAcceptedErrorDialogDelegate Delegate, FText UserErrorText, FText HeaderText);

	/**
	 * Displays a confirmation dialog using the first menu stack component from HUD.
	 *
	 * @param HUD HUD with a menu stack to spawn this in.
	 * @param Delegate Callback delegate.
	 * @param UserConfirmationText Confirmation message text.
	 * @param HeaderText (Optional) Header text.
	 * @return Confirmation dialog spawned.
	 */
	UFUNCTION(BlueprintCallable, Category = "Menu|Dialogs")
	static USCConfirmationDialogWidget* ShowConfirmationDialog(AILLHUD* HUD, FSCOnConfirmedDialogDelegate Delegate, FText UserConfirmationText, FText HeaderText);

	/**
	 * Displays a yes/no/cancel dialog using the first menu stack component from HUD.
	 *
	 * @param HUD HUD with a menu stack to spawn this in.
	 * @param Delegate Callback delegate.
	 * @param HeaderText Header text.
	 * @param UserMessageText Message text.
	 * @param YesText (Optional) Yes button text.
	 * @param NoText (Optional) No button text.
	 * @param CancelText (Optional) Cancel button text.
	 * @return Confirmation dialog spawned.
	 */
	UFUNCTION(BlueprintCallable, Category = "Menu|Dialogs")
	static USCYesNoCancelDialogWidget* ShowYesNoCancelDialog(AILLHUD* HUD, FSCOnYesNoCancelDialogDelegate Delegate, FText HeaderText, FText UserMessageText, FText YesText, FText NoText, FText CancelText);

protected:
	// Widget to spawn in ShowErrorDialog
	UPROPERTY()
	TSubclassOf<USCErrorDialogWidget> ErrorDialogWidgetClass;

	// Widget to spawn in ShowConfirmationDialog
	UPROPERTY()
	TSubclassOf<USCConfirmationDialogWidget> ConfirmationDialogWidgetClass;

	// Widget to spawn in ShowYesNoCancelDialog
	UPROPERTY()
	TSubclassOf<USCYesNoCancelDialogWidget> YesNoCancelDialogWidgetClass;
};
