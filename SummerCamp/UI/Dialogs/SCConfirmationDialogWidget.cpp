// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCConfirmationDialogWidget.h"

#include "ILLMenuStackHUDComponent.h"

#define LOCTEXT_NAMESPACE "ILLFonic.F13.SCConfirmationDialogWidget"

USCConfirmationDialogWidget::USCConfirmationDialogWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bTopMostMenu = false;

	HeaderText = LOCTEXT("ConfirmationDialogHeader", "Are you sure?");
	ConfirmButtonText = LOCTEXT("ConfirmButtonText", "Confirm");
	CancelButtonText = LOCTEXT("CancelButtonText", "Cancel");
}

void USCConfirmationDialogWidget::OnCancel()
{
	Callback.ExecuteIfBound(false);
}

void USCConfirmationDialogWidget::OnConfirm()
{
	Callback.ExecuteIfBound(true);
}

#undef LOCTEXT_NAMESPACE
