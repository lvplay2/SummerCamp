// Copyright (C) 2015-2018 IllFonic, LLC.

#include "SummerCamp.h"
#include "SCInventoryComponent.h"

#include "ILLInventoryItem.h"
#include "SCCounselorCharacter.h"

USCInventoryComponent::USCInventoryComponent(const FObjectInitializer& ObjectInitializer)
: Super()
{
}

void USCInventoryComponent::AddItem(AILLInventoryItem* Item)
{
	Super::AddItem(Item);

	// Explicit call to OnRep for listen servers
	OnRep_ItemList();
}

void USCInventoryComponent::RemoveItem(AILLInventoryItem* Item)
{
	Super::RemoveItem(Item);

	// Explicit call to OnRep for listen servers
	OnRep_ItemList();
}

void USCInventoryComponent::ForceRefresh()
{
	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(GetCharacterOwner()))
	{
		if (Counselor->IsLocallyControlled())
		{
			Counselor->UpdateInventoryWidget();
		}
	}
}

void USCInventoryComponent::OnRep_ItemList()
{
	Super::OnRep_ItemList();
	ForceRefresh();
}

AILLInventoryItem* USCInventoryComponent::RemoveAtIndex(const int32 ItemIndex)
{
	check(GetOwnerRole() == ROLE_Authority);

	if (ItemIndex < 0 || ItemIndex >= ItemList.Num())
		return nullptr;

	AILLInventoryItem* Item = ItemList[ItemIndex];

	ItemList.RemoveAt(ItemIndex);

	if (IsValid(Item) && Item->GetOuterInventory() == this)
		Item->RemovedFromInventory(this);

	OnRep_ItemList();

	return Item;
}
