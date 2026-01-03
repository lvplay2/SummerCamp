// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCSpecialItem.h"
#include "SCTommyTape.generated.h"

class USCTommyTapeDataAsset;

/**
* @class ASCTommyTape
*/
UCLASS()
class SUMMERCAMP_API ASCTommyTape
	: public ASCSpecialItem
{
	GENERATED_UCLASS_BODY()

public:
	// Begin AILLInventoryItem interface
	virtual void AddedToInventory(UILLInventoryComponent* Inventory) override;
	// End AILLInventoryItem interface
	void SetFriendlyName(const FText InFriendlyName) { FriendlyName = InFriendlyName; }

	UPROPERTY()
		TSubclassOf<USCTommyTapeDataAsset> TapeData;
};
