// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Inventory/SCItem.h"
#include "SCKillerMask.generated.h"

/**
 * @class ASCKillerMask
 */
UCLASS()
class SUMMERCAMP_API ASCKillerMask
: public ASCItem
{
	GENERATED_BODY()

public:
	// Begin ASCItem Interface
	virtual bool CanUse(bool bFromInput) override { return false; }
	// End ASCItem Interface
};
