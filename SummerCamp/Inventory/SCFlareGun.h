// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCItem.h"

#include "SCFlareGun.generated.h"

/**
 * @class ASCFlareGun
 */
UCLASS()
class SUMMERCAMP_API ASCFlareGun
: public ASCItem
{
	GENERATED_UCLASS_BODY()

public:

	// BEGIN AILLInventoryItem interface
	virtual void AttachToCharacter(bool AttachToBody, FName OverrideSocket = NAME_None) override;
	// END AILLInventoryItem interface

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<class ASCWeapon> FlareGunWeaponClass;
};
