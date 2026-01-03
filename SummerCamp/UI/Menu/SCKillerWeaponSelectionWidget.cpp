// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCKillerWeaponSelectionWidget.h"
#include "SCKillerWeaponListItem.h"
#include "SCKillerCharacter.h"
#include "SCWeapon.h"

USCKillerWeaponSelectionWidget::USCKillerWeaponSelectionWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void USCKillerWeaponSelectionWidget::InitializeList(TSoftClassPtr<ASCKillerCharacter> KillerClass)
{
	if (KillerClass.IsNull())
		return;

	if (HorizontalBox)
		HorizontalBox->ClearChildren();

	AILLPlayerController* PC = GetOwningILLPlayer();

	KillerClass.LoadSynchronous(); // TODO: Make Async
	if (const ASCKillerCharacter* const DefaultKiller = Cast<ASCKillerCharacter>(KillerClass.Get()->GetDefaultObject()))
	{
		const TArray<TSoftClassPtr<ASCWeapon>>& WeaponList = DefaultKiller->GetAllowedWeaponClasses();
		for (TSoftClassPtr<ASCWeapon> WeaponClass : WeaponList)
		{
			WeaponClass.LoadSynchronous(); // TODO: Make Async
			if (const ASCWeapon* const DefaultWeapon = Cast<ASCWeapon>(WeaponClass.Get()->GetDefaultObject()))
			{
				if (!PC->DoesUserOwnEntitlement(DefaultWeapon->DLCEntitlement))
					continue;
			}

			if (USCKillerWeaponListItem* Button = NewObject<USCKillerWeaponListItem>(this, WeaponButtonClass))
			{
				Button->WeaponClass = WeaponClass.Get();
				Button->SetPlayerContext(GetOwningPlayer());
				Button->Initialize();
				Button->bIsFocusable = true;

				Button->OnWeaponSelectedCallback.BindDynamic(this, &USCKillerWeaponSelectionWidget::OnWeaponSelected);

				if (HorizontalBox)
					HorizontalBox->AddChild(Button);
			}
		}
		// highlight the first button in the list
		if (HorizontalBox->GetChildrenCount() > 0)
		{
			if (USCKillerWeaponListItem* Button = Cast<USCKillerWeaponListItem>(HorizontalBox->GetChildAt(0)))
				Button->NavigateToNamedWidget(FName(*Button->GetName()));
		}
	}
}

void USCKillerWeaponSelectionWidget::OnWeaponSelected(TSoftClassPtr<ASCWeapon> SelectedWeapon)
{
	OnWeaponSelectedCallback.Broadcast(SelectedWeapon);
}
