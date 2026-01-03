// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCKillerWeaponListItem.h"

#include "SCWeapon.h"

USCKillerWeaponListItem::USCKillerWeaponListItem(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void USCKillerWeaponListItem::OnItemPressed()
{
	OnWeaponSelectedCallback.ExecuteIfBound(WeaponClass);
}

UTexture2D* USCKillerWeaponListItem::GetWeaponThumbnail() const
{
	if (!WeaponClass.IsNull())
	{
		WeaponClass.LoadSynchronous(); // TODO: Make Async
		if (const ASCWeapon* const Weapon = Cast<ASCWeapon>(WeaponClass.Get()->GetDefaultObject()))
			return Weapon->GetThumbnailImage();
	}

	return nullptr;
}

const FText& USCKillerWeaponListItem::GetFriendlyName() const
{
	if (!WeaponClass.IsNull())
	{
		WeaponClass.LoadSynchronous(); // TODO: Make Async
		if (const ASCWeapon* const Weapon = Cast<ASCWeapon>(WeaponClass.Get()->GetDefaultObject()))
			return Weapon->GetFriendlyName();
	}

	return FText::GetEmpty();
}
