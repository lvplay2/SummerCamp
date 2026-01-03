// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLInventoryComponent.h"

#include "ILLCharacter.h"
#include "ILLInventoryItem.h"

DEFINE_LOG_CATEGORY(LogILLInventory);

#define INVENTORY_LOG_PREFIX (GetOwnerRole() == ROLE_Authority ? TEXT("Authority") : TEXT("Remote"))

UILLInventoryComponent::UILLInventoryComponent()
: Super()
, bAutoEquip(true)
{
	// Automatically activate
	bAutoActivate = true;

	// Replicate!
	bNetAddressable = true;
	bReplicates = true;
}

void UILLInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UILLInventoryComponent, ItemList);
	DOREPLIFETIME(UILLInventoryComponent, EquippedItem);
}

bool UILLInventoryComponent::IsLocallyControlled() const
{
	return (GetCharacterOwner() ? GetCharacterOwner()->IsLocallyControlled() : false);
}

void UILLInventoryComponent::DestroyItemList()
{
	check(GetOwnerRole() == ROLE_Authority);

	for (int32 Index = ItemList.Num()-1; Index >= 0; --Index)
	{
		AILLInventoryItem* Item = ItemList[Index];
		if (Item && !Item->IsPendingKill())
		{
			Item->Destroy();
		}
	}
	ItemList.Empty();
}

AILLInventoryItem* UILLInventoryComponent::FindItem(TSubclassOf<AILLInventoryItem> ItemClass)
{
	for (AILLInventoryItem* Item : ItemList)
	{
		if (Item->IsA(ItemClass))
		{
			return Item;
		}
	}

	return nullptr;
}

AILLInventoryItem* UILLInventoryComponent::SpawnItem(TSubclassOf<AILLInventoryItem> ItemClass)
{
	check(GetOwnerRole() == ROLE_Authority);

	AILLInventoryItem* NewItem = GetWorld()->SpawnActorDeferred<AILLInventoryItem>(ItemClass, FTransform::Identity, GetOwner(), nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (NewItem)
	{
		// Finish spawning the item
		PreFinishSpawnItem(NewItem);
		NewItem->FinishSpawning(FTransform::Identity, true);
		PostFinishSpawnItem(NewItem);
	}

	return NewItem;
}

void UILLInventoryComponent::SpawnItems(TArray<TSubclassOf<AILLInventoryItem>> ItemClassList)
{
	for (auto ItemClass : ItemClassList)
	{
		SpawnItem(ItemClass);
	}
}

void UILLInventoryComponent::AddItem(AILLInventoryItem* Item)
{
	check(GetOwnerRole() == ROLE_Authority);

	if (Item)
	{
		// Add the item to the ItemList and call AddedToInventory
		ItemList.Add(Item);
		Item->AddedToInventory(this);
	}
}

void UILLInventoryComponent::RemoveItem(AILLInventoryItem* Item)
{
	check(GetOwnerRole() == ROLE_Authority);

	if (Item && Item->GetOuterInventory() == this)
	{
		ItemList.Remove(Item);
		Item->RemovedFromInventory(this);
	}
}

AILLInventoryItem* UILLInventoryComponent::FindEquippableItem(int32 ItemIndex, const int32 MoveDirection)
{
	for (int32 Iterations = 0; Iterations < ItemList.Num()-1; ++Iterations)
	{
		// Wrap ItemIndex around if it is out of bounds
		if (MoveDirection > 0)
		{
			if (ItemIndex >= ItemList.Num())
				ItemIndex = 0;
		}
		else if (ItemIndex < 0)
		{
			ItemIndex = ItemList.Num()-1;
		}

		// Take this item if it is equippable
		if (ItemList[ItemIndex] && ItemList[ItemIndex]->IsEquippable())
		{
			return ItemList[ItemIndex];
		}
		else if (ItemList[ItemIndex] == EquippedItem)
		{
			// This is our currently EquippedItem, so we have done a full lap!
			break;
		}

		// Advance to the next item
		ItemIndex += MoveDirection;
	}

	return nullptr;
}

AILLInventoryItem* UILLInventoryComponent::FindNextEquippableItem(const bool bNextItem)
{
	if (ItemList.Num() <= 1)
	{
		return nullptr;
	}

	const int32 MoveDirection = bNextItem ? 1 : -1;
	return FindEquippableItem(ItemList.IndexOfByKey(EquippedItem)+MoveDirection, MoveDirection);
}

void UILLInventoryComponent::OnRep_ItemList()
{
	// Force another call to AddedToInventory for remote clients
	for (auto Item : ItemList)
	{
		if (Item)
		{
			Item->AddedToInventory(this);
		}
	}
}

void UILLInventoryComponent::PreFinishSpawnItem(AILLInventoryItem* NewItem)
{
	ItemList.Add(NewItem);
	NewItem->AddedToInventory(this);
}

void UILLInventoryComponent::PostFinishSpawnItem(AILLInventoryItem* NewItem)
{
	// When we have no equipped item and finish spawning a new one, automatically equip it if possible
	if (bAutoEquip && EquippedItem == nullptr && NewItem->IsEquippable())
	{
		EquipItem(NewItem);
	}
}

void UILLInventoryComponent::EquipItem(AILLInventoryItem* Item)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		check(IsLocallyControlled());

		// Request Item to be equipped on the server
		ServerRequestEquip(Item);
		return;
	}

	check(Item != nullptr);
	check(Item->GetOuterInventory() == this);

	if (EquippedItem)
	{
		// Unequip our current item first!
		ClientsUnequip(Item);
	}
	else
	{
		// We can equip this item now
		InternalEquipItem(Item);
	}
}

void UILLInventoryComponent::UnequipCurrentItem()
{
	if (EquippedItem)
	{
		ClientsUnequip(nullptr);
	}
}

void UILLInventoryComponent::EquipNextItem(const bool bNextItem)
{
	if (AILLInventoryItem* NextItem = FindNextEquippableItem(bNextItem))
	{
		EquipItem(NextItem);
	}
}

void UILLInventoryComponent::EquipItemAt(const int32 LowestIndex)
{
	if (AILLInventoryItem* FoundItem = FindEquippableItem(LowestIndex, 1))
	{
		EquipItem(FoundItem);
	}
}

void UILLInventoryComponent::QuickSwitch()
{
	// Default back to the next equippable item in the inventory when the character has not changed the equipped weapon yet
	if (!LastEquippedItem || LastEquippedItem == EquippedItem)
	{
		LastEquippedItem = FindNextEquippableItem(true);
	}

	// Now equip it!
	if (LastEquippedItem && LastEquippedItem != EquippedItem)
	{
		EquipItem(LastEquippedItem);
	}
}

void UILLInventoryComponent::ServerRequestEquip_Implementation(AILLInventoryItem* Item)
{
	EquipItem(Item);
}

bool UILLInventoryComponent::ServerRequestEquip_Validate(AILLInventoryItem* Item)
{
	return true;
}

void UILLInventoryComponent::ClientsUnequip_Implementation(AILLInventoryItem* NextItem)
{
	InternalUnequipItem(NextItem);
}

void UILLInventoryComponent::InternalEquipItem(AILLInventoryItem* Item)
{
	UE_LOG(LogILLInventory, Log, TEXT("[%s] Equipping %s"), INVENTORY_LOG_PREFIX, *GetNameSafe(Item));

	// Disallow unequippable items
	if (Item && !Item->IsEquippable())
		Item = nullptr;

	// Take this as the EquippedItem
	EquippedItem = Item;

	// Force a call to AddedToInventory for the sake of remote clients
	if (EquippedItem)
	{
		EquippedItem->AddedToInventory(this);
	}

	// Notify our listeners
	OnItemEquipStartDelegate.Broadcast(Item);
	OnItemEquipStart.Broadcast(Item);

	if (Item)
	{
		// Start equip animations and listen for completion
		auto Delegate = FILLInventoryItemEquipDelegate::CreateUObject(this, &UILLInventoryComponent::OnEquipCompleted);
		Item->StartEquipping(Delegate, (GetOwnerRole() != ROLE_Authority));
	}
	else
	{
		// Still broadcast this event!
		OnItemUnequipFinishedDelegate.Broadcast(nullptr);
		OnItemUnequipFinished.Broadcast(nullptr);
	}
}

void UILLInventoryComponent::InternalUnequipItem(AILLInventoryItem* NextItem)
{
	// We may have no EquippedItem on remote clients, or may receive this call twice for a remote reload
	if (EquippedItem && EquippedItem != NextItem)
	{
		UE_LOG(LogILLInventory, Log, TEXT("[%s] Unequipping %s to equip %s"), INVENTORY_LOG_PREFIX, *GetNameSafe(EquippedItem), *GetNameSafe(NextItem));
		check(EquippedItem->IsEquipped() || EquippedItem->IsEquipping() || EquippedItem->IsUnequipping());

		// Notify our listener
		OnItemUnequipStartDelegate.Broadcast(NextItem);
		OnItemUnequipStart.Broadcast(NextItem);

		// We already have an item equipped, so wait for it to unequip first!
		auto Delegate = FILLInventoryItemUnequipDelegate::CreateUObject(this, &UILLInventoryComponent::OnUnequipCompleted);
		EquippedItem->StartUnequipping(Delegate, NextItem);
	}
}

void UILLInventoryComponent::OnUnequipCompleted(AILLInventoryItem* UnequippedItem, AILLInventoryItem* NextItem)
{
	UE_LOG(LogILLInventory, Log, TEXT("[%s] OnUnequipCompleted %s to equip %s"), INVENTORY_LOG_PREFIX, *GetNameSafe(UnequippedItem), *GetNameSafe(NextItem));

	// Only NULL out the EquippedItem on the authority, so clients dont trample it
	if (GetOwnerRole() == ROLE_Authority)
	{
		EquippedItem = nullptr;
	}

	// Notify our listener
	OnItemUnequipFinishedDelegate.Broadcast(UnequippedItem);
	OnItemUnequipFinished.Broadcast(UnequippedItem);

	// Start equipping NextItem immediately
	if (NextItem)
	{
		InternalEquipItem(NextItem);
	}
}

void UILLInventoryComponent::OnEquipCompleted(AILLInventoryItem* Item)
{
	UE_LOG(LogILLInventory, Log, TEXT("[%s] OnEquipCompleted %s"), INVENTORY_LOG_PREFIX, *GetNameSafe(Item));
	LastEquippedItem = Item;

	// Notify our listener
	OnItemEquipFinishedDelegate.Broadcast(Item);
	OnItemEquipFinished.Broadcast(Item);
}

void UILLInventoryComponent::OnRep_EquippedItem(AILLInventoryItem* LastValue)
{
	// Swap to the same state the server was before it unequipped/equipped
	AILLInventoryItem* NextItem = EquippedItem;
	EquippedItem = LastValue;

	// Simulate the unequip/equip
	if (EquippedItem && EquippedItem->IsEquipped())
	{
		InternalUnequipItem(NextItem);
	}
	else
	{
		InternalEquipItem(NextItem);
	}
}

void UILLInventoryComponent::UpdateMeshVisibility(const bool bFirstPerson)
{
	for (auto Item : ItemList)
	{
		if (Item)
		{
			Item->UpdateMeshVisibility(bFirstPerson);
		}
	}
}
