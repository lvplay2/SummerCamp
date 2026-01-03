// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCSwatchListWidget.h"
#include "SCGameInstance.h"
#include "SCCounselorWardrobe.h"
#include "SCSwatchButton.h"
#include "PanelWidget.h"
#include "WrapBox.h"
#include "ILLPlayerController.h"
#include "ILLPlayerInput.h"

USCSwatchListWidget::USCSwatchListWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CurrentClothingSlot()
{
	CurrentSelectedButton = nullptr;
	CurrentHighlightedButton = nullptr;
}

void USCSwatchListWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	bool bUsingGamepad = false;
	if (AILLPlayerController* PC = GetOwningILLPlayer())
	{
		if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(PC->PlayerInput))
		{
			bUsingGamepad = Input->IsUsingGamepad();
		}
	}

	if (bUsingGamepad)
	{
		if (CurrentHighlightedButton)
			CurrentHighlightedButton->HighlightButton();
	}
}

void USCSwatchListWidget::InitializeList(const FSCClothingSlot& ClothingSlot, FSCClothingMaterialPair SelectedSwatch)
{
	if (!WrapBox)
		return;

	CurrentClothingSlot = ClothingSlot;

	const AILLPlayerController* PC = GetOwningILLPlayer();

	// wipe any old buttons
	for (int i(0); i < WrapBox->GetChildrenCount(); ++i)
	{
		USCUserWidget* Widget = Cast<USCUserWidget>(WrapBox->GetChildAt(i));
		Widget->SetVisibility(ESlateVisibility::Collapsed);
		Widget->RemoveFromViewport();
		Widget->MarkPendingKill();
	}
	WrapBox->ClearChildren();

	const USCGameInstance* GameInstance = Cast<USCGameInstance>(GetWorld()->GetGameInstance());
	const USCCounselorWardrobe* CounselorWardrobe = GameInstance ? GameInstance->GetCounselorWardrobe() : nullptr;

	if (CounselorWardrobe)
	{
		for (const TSubclassOf<USCClothingSlotMaterialOption>& Swatch : CurrentClothingSlot.MaterialOptions)
		{
			const USCClothingSlotMaterialOption* const DefaultSwatch = Swatch->GetDefaultObject<USCClothingSlotMaterialOption>();
			bool bSwatchLocked = false;
			bool bShowStore = false;
			bool bShouldHide = false;

			CounselorWardrobe->IsMaterialOptionLockedFor(Swatch, PC, bSwatchLocked, bShouldHide);

			// if we dont own this swatch and its not allowed to be shown just skip it
			if (bShouldHide)
				continue;

			// check level and entitlement to see if they can use this
			bShowStore = !DefaultSwatch->UnlockEntitlement.IsEmpty();

			if (USCSwatchButton* Button = NewObject<USCSwatchButton>(this, ButtonClass))
			{
				Button->SetPlayerContext(GetOwningPlayer());
				Button->Initialize();
				Button->InitializeButton(Swatch, !bSwatchLocked, bShowStore);

				Button->OnClickedCallback.BindDynamic(this, &USCSwatchListWidget::OnButtonSelected);
				Button->OnHighlightedCallback.BindDynamic(this, &USCSwatchListWidget::OnButtonHighlighted);
				Button->OnClickedLockedCallback.BindDynamic(this, &USCSwatchListWidget::LockedSelected);

				WrapBox->AddChild(Button);

				// see if this button is the currently set swatch
				if (SelectedSwatch.SelectedSwatch == Swatch)
				{
					Button->SetSelected(true);
					OnButtonHighlighted(Button);
					Button->NavigateToNamedWidget(FName(*Button->GetName()));
				}
			}
		}
	}

	// there is no valid swatch which means its default, set the first button as selected
	if (SelectedSwatch.SelectedSwatch == nullptr)
	{
		if (USCSwatchButton* Button = Cast<USCSwatchButton>(WrapBox->GetChildAt(0)))
		{
			Button->SetSelected(true);
			OnButtonHighlighted(Button);
		}
	}

	// add empty slots
	for (int i = WrapBox->GetChildrenCount(); i < 18; ++i)
	{
		if (USCUserWidget* Widget = NewObject<USCUserWidget>(this, EmptyButtonClass))
		{
			Widget->SetPlayerContext(GetOwningPlayer());
			Widget->Initialize();

			WrapBox->AddChild(Widget);
		}
	}
}

void USCSwatchListWidget::OnButtonSelected(USCSwatchButton* SelectedButton)
{
	CurrentSelectedButton = SelectedButton;

	// clear highlighted button
	OnButtonHighlighted(CurrentSelectedButton);

	// make sure all other buttons in list are unselected and no longer highlighted
	for (uint8 i(0); i<WrapBox->GetChildrenCount(); ++i)
	{
		if (USCSwatchButton* Button = Cast<USCSwatchButton>(WrapBox->GetChildAt(i)))
		{
			if (Button != SelectedButton)
			{
				Button->SetSelected(false);
				Button->UnHighlightButton();
			}
		}
	}

	// inform listeners that an outfit was selected
	OnItemSelected.Broadcast(SelectedButton->GetSwatchClass());
}

UTexture2D* USCSwatchListWidget::GetSlotIcon() const
{
	// shouldnt need to preload this asset because it should already be loaded from the slot selectoin buttons
	return CurrentClothingSlot.SlotIcon.Get();
}

void USCSwatchListWidget::OnButtonHighlighted(USCSwatchButton* HighlightedButton)
{
	if (CurrentHighlightedButton)
		CurrentHighlightedButton->UnHighlightButton();

	CurrentHighlightedButton = HighlightedButton;

	FString HighlightString = TEXT("");
	bool IsLocked = false;
	if (HighlightedButton)
	{
		HighlightString = HighlightedButton->GetHighlightText();
		IsLocked = HighlightedButton->IsLocked();
	}

	OnSwatchHighlighted.Broadcast(HighlightString, IsLocked);
}

void USCSwatchListWidget::SelectCurrentHighlightedButton()
{
	if (CurrentHighlightedButton != nullptr)
	{
		CurrentHighlightedButton->SetSelected(true);
		OnButtonSelected(CurrentHighlightedButton);
	}
}

void USCSwatchListWidget::LockedSelected()
{
	OnLockedSelected.Broadcast();
}