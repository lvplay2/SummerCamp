// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once
#include "../SCUserWidget.h"
#include "SCWeapon.h"
#include "SCKillerWeaponSelectionButton.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSCOnSelectedDelegate);


/**
* @class USCKillerWeaponSelectionButton
*/
UCLASS()
class SUMMERCAMP_API USCKillerWeaponSelectionButton
	: public USCUserWidget
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintPure, Category = "UI")
	UTexture2D* GetButtonImage() const
	{
		if (!WeaponClass.IsNull())
		{
			WeaponClass.LoadSynchronous(); // TODO: Make Async
			if (const ASCWeapon* const Weapon = Cast<ASCWeapon>(WeaponClass.Get()->GetDefaultObject()))
				return Weapon->GetThumbnailImage();
		}
		return nullptr;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSoftClassPtr<ASCWeapon> WeaponClass;

	UPROPERTY(BlueprintAssignable)
	FSCOnSelectedDelegate OnSelected;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	bool bIsDisabled;

protected:
	UFUNCTION(BlueprintCallable, Category = "UI")
	void OnButtonPressed() { OnSelected.Broadcast(); }
};