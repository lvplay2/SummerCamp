// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLInventoryComponent.h"
#include "SCInventoryComponent.generated.h"

/**
 * @class USCInventoryComponent
 */
UCLASS(Blueprintable, ClassGroup="SCItem", Config=Game, meta=(BlueprintSpawnableComponent))
class SUMMERCAMP_API USCInventoryComponent
: public UILLInventoryComponent
{
	GENERATED_UCLASS_BODY()

	// BEGIN UILLInventoryComponent interface
	virtual void AddItem(AILLInventoryItem* Item) override;
	virtual void RemoveItem(AILLInventoryItem* Item) override;
	void ForceRefresh();

protected:
	virtual void OnRep_ItemList() override;
	// END UILLInventoryComponent interface

public:
	/**
	 * Removes item at index, regardless of if it is valid or not
	 * @param ItemIndex - Index to remove the item at (should be 0 <= ItemIndex < ItemList.Num())
	 * @return The item removed, may be nullptr
	 */
	AILLInventoryItem* RemoveAtIndex(const int32 ItemIndex);
};