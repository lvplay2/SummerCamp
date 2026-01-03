// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCYesNoCancelDialogWidget.h"

#include "ILLMenuStackHUDComponent.h"

#define LOCTEXT_NAMESPACE "ILLFonic.F13.SCYesNoCancelDialogWidget"

USCYesNoCancelDialogWidget::USCYesNoCancelDialogWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bTopMostMenu = false;

	YesButtonText = LOCTEXT("YesButtonText", "Yes");
	NoButtonText = LOCTEXT("NoButtonText", "No");
	CancelButtonText = LOCTEXT("CancelButtonText", "Cancel");
}

void USCYesNoCancelDialogWidget::Broadcast(ESCYesNoDialogResult Result)
{
	Callback.ExecuteIfBound(Result);
}

#undef LOCTEXT_NAMESPACE
