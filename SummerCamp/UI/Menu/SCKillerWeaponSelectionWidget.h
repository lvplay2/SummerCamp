// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCUserWidget.h"
#include "HorizontalBox.h"
#include "SCKillerWeaponSelectionWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSCWeaponSelectedDelegate, TSoftClassPtr<ASCWeapon>, SelectedWeapon);

/**
* @class USCKillerWeaponSelectionWidget
*/
UCLASS()
class SUMMERCAMP_API USCKillerWeaponSelectionWidget
	: public USCUserWidget
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintCallable, Category = "Init")
	void InitializeList(TSoftClassPtr<ASCKillerCharacter> KillerClass);

	UFUNCTION()
	void OnWeaponSelected(TSoftClassPtr<ASCWeapon> SelectedWeapon);

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FSCWeaponSelectedDelegate OnWeaponSelectedCallback;

protected:
	UPROPERTY(BlueprintReadWrite, Category = "UI")
	UHorizontalBox* HorizontalBox;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USCUserWidget> WeaponButtonClass;
};