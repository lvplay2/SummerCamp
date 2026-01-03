// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCWidgetBlueprintLibrary.h"

#include "ILLHUD.h"
#include "ILLMenuStackHUDComponent.h"

USCWidgetBlueprintLibrary::USCWidgetBlueprintLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	static ConstructorHelpers::FClassFinder<USCErrorDialogWidget> ErrorDialogObj(TEXT("WidgetBlueprint'/Game/UI/Menu/Dialogs/ErrorPopupWidget.ErrorPopupWidget_C'"));
	ErrorDialogWidgetClass = ErrorDialogObj.Class;

	static ConstructorHelpers::FClassFinder<USCConfirmationDialogWidget> ConfirmationDialogObj(TEXT("WidgetBlueprint'/Game/UI/Menu/Dialogs/ConfirmationPopupWidget.ConfirmationPopupWidget_C'"));
	ConfirmationDialogWidgetClass = ConfirmationDialogObj.Class;

	static ConstructorHelpers::FClassFinder<USCYesNoCancelDialogWidget> YesNoCancelDialogObj(TEXT("WidgetBlueprint'/Game/UI/Menu/Dialogs/YesNoCancelPopupWidget.YesNoCancelPopupWidget_C'"));
	YesNoCancelDialogWidgetClass = YesNoCancelDialogObj.Class;
}

USCErrorDialogWidget* USCWidgetBlueprintLibrary::ShowErrorDialog(AILLHUD* HUD, FSCOnAcceptedErrorDialogDelegate Delegate, FText UserErrorText, FText HeaderText)
{
	if (!HUD)
	{
		UE_LOG(LogSC, Warning, TEXT("ShowErrorDialog called with NULL HUD!"));
		return nullptr;
	}

	const USCWidgetBlueprintLibrary* const DefaultLibrary = StaticClass()->GetDefaultObject<USCWidgetBlueprintLibrary>();
	UILLMenuStackHUDComponent* MenuStack = HUD->GetMenuStackComp();
	if (ensure(MenuStack) && DefaultLibrary->ErrorDialogWidgetClass)
	{
		USCErrorDialogWidget* NewDialog = Cast<USCErrorDialogWidget>(MenuStack->PushMenu(DefaultLibrary->ErrorDialogWidgetClass));
		if (ensure(NewDialog))
		{
			NewDialog->Callback = Delegate;
			if (!HeaderText.IsEmptyOrWhitespace())
				NewDialog->HeaderText = HeaderText;
			NewDialog->ErrorText = UserErrorText;
			NewDialog->OnPostSpawn();
			return NewDialog;
		}
	}

	return nullptr;
}

USCConfirmationDialogWidget* USCWidgetBlueprintLibrary::ShowConfirmationDialog(AILLHUD* HUD, FSCOnConfirmedDialogDelegate Delegate, FText UserConfirmationText, FText HeaderText)
{
	if (!HUD)
	{
		UE_LOG(LogSC, Warning, TEXT("ShowConfirmationDialog called with NULL HUD!"));
		return nullptr;
	}

	const USCWidgetBlueprintLibrary* const DefaultLibrary = StaticClass()->GetDefaultObject<USCWidgetBlueprintLibrary>();
	UILLMenuStackHUDComponent* MenuStack = HUD->GetMenuStackComp();
	if (ensure(MenuStack) && DefaultLibrary->ConfirmationDialogWidgetClass)
	{
		USCConfirmationDialogWidget* NewDialog = Cast<USCConfirmationDialogWidget>(MenuStack->PushMenu(DefaultLibrary->ConfirmationDialogWidgetClass));
		if (ensure(NewDialog))
		{
			NewDialog->Callback = Delegate;
			if (!HeaderText.IsEmptyOrWhitespace())
				NewDialog->HeaderText = HeaderText;
			NewDialog->ConfirmationText = UserConfirmationText;
			NewDialog->OnPostSpawn();
			return NewDialog;
		}
	}

	return nullptr;
}

USCYesNoCancelDialogWidget* USCWidgetBlueprintLibrary::ShowYesNoCancelDialog(AILLHUD* HUD, FSCOnYesNoCancelDialogDelegate Delegate, FText HeaderText, FText UserMessageText, FText YesText, FText NoText, FText CancelText)
{
	if (!HUD)
	{
		UE_LOG(LogSC, Warning, TEXT("ShowYesNoCancelDialog called with NULL HUD!"));
		return nullptr;
	}

	const USCWidgetBlueprintLibrary* const DefaultLibrary = StaticClass()->GetDefaultObject<USCWidgetBlueprintLibrary>();
	UILLMenuStackHUDComponent* MenuStack = HUD->GetMenuStackComp();
	if (ensure(MenuStack) && DefaultLibrary->YesNoCancelDialogWidgetClass)
	{
		USCYesNoCancelDialogWidget* NewDialog = Cast<USCYesNoCancelDialogWidget>(MenuStack->PushMenu(DefaultLibrary->YesNoCancelDialogWidgetClass));
		if (ensure(NewDialog))
		{
			NewDialog->Callback = Delegate;
			NewDialog->HeaderText = HeaderText;
			NewDialog->MessageText = UserMessageText;
			if (!YesText.IsEmptyOrWhitespace())
				NewDialog->YesButtonText = YesText;
			if (!NoText.IsEmptyOrWhitespace())
				NewDialog->NoButtonText = NoText;
			if (!CancelText.IsEmptyOrWhitespace())
				NewDialog->CancelButtonText = CancelText;
			NewDialog->OnPostSpawn();
			return NewDialog;
		}
	}

	return nullptr;
}
