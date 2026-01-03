// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCOutfitListWidget.h"
#include "SCGameInstance.h"
#include "SCCounselorWardrobe.h"
#include "SCOutfitButton.h"
#include "PanelWidget.h"
#include "ScrollBox.h"
#include "ILLPlayerController.h"
#include "ILLPlayerInput.h"

USCOutfitListWidget::USCOutfitListWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CurrentSelectedButton = nullptr;
	CurrentHighlightedButton = nullptr;
}

void USCOutfitListWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
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

void USCOutfitListWidget::InitializeList(TSoftClassPtr<ASCCounselorCharacter> CounselorClass, TSubclassOf<USCClothingData> OutfitClass, USCCounselorWardrobeWidget* WardrobeParent)
{
	const USCGameInstance* GameInstance = Cast<USCGameInstance>(GetWorld()->GetGameInstance());
	const USCCounselorWardrobe* CounselorWardrobe = GameInstance ? GameInstance->GetCounselorWardrobe() : nullptr;

	const AILLPlayerController* PC = GetOwningILLPlayer();

	if (CounselorWardrobe)
	{
		const TArray<TSubclassOf<USCClothingData>>& ClothingDataList = CounselorWardrobe->GetClothingOptions(CounselorClass);

		for (TSubclassOf<USCClothingData> ClothingDataClass : ClothingDataList)
		{
			const USCClothingData* const DefaultOutfit = ClothingDataClass->GetDefaultObject<USCClothingData>();
			bool bOutfitLocked = true;
			bool bShowStore = false;
			bool bShouldHide = false;

			CounselorWardrobe->IsOutfitLockedFor(DefaultOutfit, PC, bOutfitLocked, bShouldHide);

			if (bShouldHide)
				continue;

			bShowStore = !DefaultOutfit->UnlockEntitlement.IsEmpty();

			//spawn new outfit button
			if (USCOutfitButton* Button = NewObject<USCOutfitButton>(this, ButtonClass))
			{
				Button->SetPlayerContext(GetOwningPlayer());
				Button->Initialize();
				// set its clothing class
				Button->InitializeButton(ClothingDataClass, WardrobeParent, !bOutfitLocked, bShowStore);

				Button->OnClickedCallback.BindDynamic(this, &USCOutfitListWidget::OnButtonSelected);
				Button->OnHighlightedCallback.BindDynamic(this, &USCOutfitListWidget::OnButtonHighlighted);
				Button->OnClickedLockedCallback.BindDynamic(this, &USCOutfitListWidget::LockedSelected);

				// add to the listbox
				if (ScrollBox)
					ScrollBox->AddChild(Button);
			}
		}

		// find and select the button for the outfit passed in
		SelectByOutfitClass(OutfitClass);
	}

	ParentWidget = WardrobeParent;
}

void USCOutfitListWidget::OnButtonSelected(USCOutfitButton* SelectedButton)
{
	CurrentSelectedButton = SelectedButton;

	// make sure selected button stays highlighted
	OnButtonHighlighted(CurrentSelectedButton);

	// make sure all other buttons in list are unselected and no longer highlighted
	for (uint8 i(0); i<ScrollBox->GetChildrenCount(); ++i)
	{
		if (USCOutfitButton* Button = Cast<USCOutfitButton>(ScrollBox->GetChildAt(i)))
		{
			if (Button != SelectedButton)
			{
				Button->SetSelected(false);
				Button->UnHighlightButton();
			}
		}
	}

	// inform listeners that an outfit was selected
	OnItemSelected.Broadcast(SelectedButton->GetOutfitClass(), true);
}

void USCOutfitListWidget::OnButtonHighlighted(USCOutfitButton* HighlightedButton)
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

	OnOutfitHighlighted.Broadcast(HighlightString, IsLocked);
}

void USCOutfitListWidget::SelectCurrentHighlightedButton()
{
	if (CurrentHighlightedButton != nullptr)
	{
		if (!CurrentHighlightedButton->IsLocked())
		{
			CurrentHighlightedButton->SetSelected(true);
			OnButtonSelected(CurrentHighlightedButton);
		}
	}
}

void USCOutfitListWidget::SelectByOutfitClass(TSubclassOf<class USCClothingData> OutfitClass, bool bAndNotify/* = true*/)
{
	bool bFoundOutfit = false;
	// make sure all other buttons in list are unselected and no longer highlighted
	for (uint8 i(0); i < ScrollBox->GetChildrenCount(); ++i)
	{
		if (USCOutfitButton* Button = Cast<USCOutfitButton>(ScrollBox->GetChildAt(i)))
		{
			if (Button->GetOutfitClass() == OutfitClass)
			{
				Button->SetSelected(true);

				CurrentSelectedButton = Button;

				// make sure selected button stays highlighted
				OnButtonHighlighted(CurrentSelectedButton);

				Button->NavigateToNamedWidget(FName(*Button->GetName()));

				bFoundOutfit = true;

			}
			else
			{
				Button->SetSelected(false);
				Button->UnHighlightButton();
			}
		}
	}

	if (bFoundOutfit && bAndNotify)
		OnItemSelected.Broadcast(OutfitClass, false);
}

void USCOutfitListWidget::LockedSelected()
{
	OnLockedSelected.Broadcast();
}