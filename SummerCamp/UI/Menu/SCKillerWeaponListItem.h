// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCUserWidget.h"
#include "SCKillerWeaponListItem.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FSCOnWeaponSelectedDelegate, TSoftClassPtr<ASCWeapon>, SelectedWeaponClass);

/**
 * @class USCKillerWeaponListItem
 */
UCLASS()
class SUMMERCAMP_API USCKillerWeaponListItem
: public USCUserWidget
{
	GENERATED_UCLASS_BODY()
public:

	UPROPERTY(Transient)
	FSCOnWeaponSelectedDelegate OnWeaponSelectedCallback;

	UPROPERTY()
	TSoftClassPtr<ASCWeapon> WeaponClass;

protected:
	UFUNCTION(BlueprintCallable, Category = "Selection")
	void OnItemPressed();

	UFUNCTION(BlueprintPure, Category = "UI")
	UTexture2D* GetWeaponThumbnail() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	const FText& GetFriendlyName() const;
};
