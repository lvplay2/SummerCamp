// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once
#include "SCMenuWidget.h"
#include "SCKillerWeaponSelectionWidget.h"
#include "SCKillerWeaponSelectionMenu.generated.h"

class ASCActorPreview;

/**
* @class USCKillerWeaponSelectionMenu
*/
UCLASS()
class SUMMERCAMP_API USCKillerWeaponSelectionMenu
	: public USCMenuWidget
{
	GENERATED_UCLASS_BODY()
public:
	class USCBackendInventory* GetInventoryManager();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void InitalizeMenu(TSoftClassPtr<ASCKillerCharacter> KillerClass);

	UFUNCTION(BlueprintCallable, Category = "Profile")
	void SaveWeaponProfile(TSoftClassPtr<ASCWeapon> WeaponClass);

protected:
	UPROPERTY(BlueprintReadWrite, Category = "UI")
	USCKillerWeaponSelectionWidget* WeaponList;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TSoftClassPtr<ASCKillerCharacter> SelectedKillerClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	FText KillerName;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	FText KillerTrope;
};