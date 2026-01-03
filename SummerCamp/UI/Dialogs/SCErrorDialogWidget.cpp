// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCErrorDialogWidget.h"

#include "ILLMenuStackHUDComponent.h"

#define LOCTEXT_NAMESPACE "ILLFonic.F13.SCErrorDialogWidget"

USCErrorDialogWidget::USCErrorDialogWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bTopMostMenu = false;

	HeaderText = LOCTEXT("ErrorDialogHeader", "Error!");
	AcceptButtonText = LOCTEXT("AcceptButtonText", "Accept");
}

void USCErrorDialogWidget::OnAccept()
{
	Callback.ExecuteIfBound();
}

#undef LOCTEXT_NAMESPACE
