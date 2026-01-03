// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Inventory/SCSpecialItem.h"
#include "SCWalkieTalkie.generated.h"

/**
 * @class ASCWalkieTalkie 
 */
UCLASS()
class SUMMERCAMP_API ASCWalkieTalkie 
	: public ASCSpecialItem
{
	GENERATED_UCLASS_BODY()

public:
	/** The minimum distance before 2D voice */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Range")
	float MinVoIPDistance;

	// BEGIN AILLInventoryItem interface
	virtual void AttachToCharacter(bool AttachToBody, FName OverrideSocket = NAME_None) override;
	// END AILLInventoryItem interface
};	
