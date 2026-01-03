// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCItem.h"

#include "SCPocketKnife.generated.h"

/**
 * @class ASCPocketKnife
 */
UCLASS()
class SUMMERCAMP_API ASCPocketKnife
: public ASCItem
{
	GENERATED_UCLASS_BODY()

public:
	// BEGIN ASCItem interface
	virtual bool ShouldAttach(bool bFromInput) override;
	virtual bool Use(bool bFromInput) override;
	virtual bool CanUse(bool bFromInput) override;
	// END ASCItem interface
};
