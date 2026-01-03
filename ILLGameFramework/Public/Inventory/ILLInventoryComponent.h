// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Components/ActorComponent.h"
#include "ILLCharacter.h"
#include "ILLInventoryComponent.generated.h"

class AILLInventoryItem;

ILLGAMEFRAMEWORK_API DECLARE_LOG_CATEGORY_EXTERN(LogILLInventory, Log, All);

/** General use broadcast inventory item delegate. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnILLInventoryItemChangeDelegate, AILLInventoryItem*);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnILLInventoryItemChange, AILLInventoryItem*, Item);

/**
 * @class UILLInventoryComponent
 */
UCLASS(Blueprintable, ClassGroup="ILLItem", Config=Game, meta=(BlueprintSpawnableComponent))
class ILLGAMEFRAMEWORK_API UILLInventoryComponent
: public UActorComponent
{
	GENERATED_BODY()

public:
	UILLInventoryComponent();

	/** @return true if this component is locally controlled. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ILLInventoryComponent")
	bool IsLocallyControlled() const;

	/** @return ILLCharacter owner. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ILLInventoryComponent")
	AILLCharacter* GetCharacterOwner() const { return Cast<AILLCharacter>(GetOwner()); }

	//////////////////////////////////////////////////
	// Item management

	/** Destroys all items in the ItemList and clears it. */
	void DestroyItemList();

	/** @return List of items in this inventory. */
	FORCEINLINE const TArray<AILLInventoryItem*>& GetItemList() const { return ItemList; }

	/** @return First item in ItemList that IsA(ItemClass). */
	UFUNCTION(BlueprintCallable, Category = "ILLInventoryComponent")
	AILLInventoryItem* FindItem(TSubclassOf<AILLInventoryItem> ItemClass);

	/** Spawns ItemClass and adds it to ItemList. */
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "ILLInventoryComponent")
	AILLInventoryItem* SpawnItem(TSubclassOf<AILLInventoryItem> ItemClass);

	/** Calls SpawnItem on each ItemClassList entry. */
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "ILLInventoryComponent")
	void SpawnItems(TArray<TSubclassOf<AILLInventoryItem>> ItemClassList);

	/** Adds Item directly to the List of items. */
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "ILLInventoryComponent")
	virtual void AddItem(AILLInventoryItem* Item);

	/** Removes Item directly from the list of items. */
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "ILLInventoryComponent")
	virtual void RemoveItem(AILLInventoryItem* Item);

protected:
	/** @return Equippable item at ItemIndex or along MoveDirection. */
	AILLInventoryItem* FindEquippableItem(int32 ItemIndex, const int32 MoveDirection);

	/** @return When bNextItem is true then the next equippable item, otherwise the previous. Will return nullptr when there are one or fewer items in the ItemList. */
	AILLInventoryItem* FindNextEquippableItem(const bool bNextItem);

	/** Replication handler for ItemList. */
	UFUNCTION()
	virtual void OnRep_ItemList();

	/** List of items in this inventory */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_ItemList, Transient)
	TArray<AILLInventoryItem*> ItemList;

protected:
	/** Called from SpawnItem before a deferred actor spawn finishes. */
	virtual void PreFinishSpawnItem(AILLInventoryItem* NewItem);

	/** Called from SpawnItem after it has finished spawning. */
	virtual void PostFinishSpawnItem(AILLInventoryItem* NewItem);
	
	//////////////////////////////////////////////////
	// Equipping and unequipping
public:

	/** If true, newly spawned items will auto-equip when nothing else is equipped */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ILLInventoryComponent")
	uint32 bAutoEquip : 1;

	/** Unequips the current item and equips Item. Item must be in this inventory. */
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "ILLInventoryComponent")
	void EquipItem(AILLInventoryItem* Item);

	/** Unequips the current item. */
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "ILLInventoryComponent")
	void UnequipCurrentItem();

	/** When bNextItem is true then Equip the next equippable inventory item, otherwise the previous. */
	void EquipNextItem(const bool bNextItem);

	/** Equips the first item found at LowestIndex, if possible. Increments LowestIndex until an equippable item is found or the entire ItemList is walked. */
	void EquipItemAt(const int32 LowestIndex);

	/** @return Currently equipped item. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ILLInventoryComponent")
	AILLInventoryItem* GetEquippedItem() const { return EquippedItem; }
	
	/** Quickly switch back to the last equipped item. */
	UFUNCTION(BlueprintCallable, Category="ILLInventoryComponent")
	void QuickSwitch();

	/** Blueprintable delegate for item equip start */
	UPROPERTY(BlueprintAssignable)
	FOnILLInventoryItemChange OnItemEquipStart;

	/** Blueprintable delegate for item equip completion */
	UPROPERTY(BlueprintAssignable)
	FOnILLInventoryItemChange OnItemEquipFinished;

	/** Blueprintable delegate for item unequip start */
	UPROPERTY(BlueprintAssignable)
	FOnILLInventoryItemChange OnItemUnequipStart;

	/** Blueprintable delegate for item unequip completion */
	UPROPERTY(BlueprintAssignable)
	FOnILLInventoryItemChange OnItemUnequipFinished;

	/** Delegate for item equip start */
	FOnILLInventoryItemChangeDelegate OnItemEquipStartDelegate;

	/** Delegate for item equip completion */
	FOnILLInventoryItemChangeDelegate OnItemEquipFinishedDelegate;

	/** Delegate for item unequip start */
	FOnILLInventoryItemChangeDelegate OnItemUnequipStartDelegate;

	/** Delegate for item unequip completion */
	FOnILLInventoryItemChangeDelegate OnItemUnequipFinishedDelegate;

private:
	/** Called from whoever locally controls this component to call EquipItem. */
	UFUNCTION(Reliable, Server, WithValidation)
	void ServerRequestEquip(AILLInventoryItem* Item);

	/** Notification that the current item is unequipping. */
	UFUNCTION(NetMulticast, Reliable)
	void ClientsUnequip(AILLInventoryItem* NextItem);

	/** Internal simulated path for item equipping. */
	void InternalEquipItem(AILLInventoryItem* Item);

	/** Internal simulated path for item unequipping. */
	void InternalUnequipItem(AILLInventoryItem* NextItem);

	/** Callback for when item unequipping completes. */
	void OnUnequipCompleted(AILLInventoryItem* UnequippedItem, AILLInventoryItem* NextItem);

	/** Callback for when item equipping completes. */
	void OnEquipCompleted(AILLInventoryItem* Item);

	/** Replication handler for EquippedItem. */
	UFUNCTION()
	void OnRep_EquippedItem(AILLInventoryItem* LastValue);

	/** Currently equipped item */
	UPROPERTY(ReplicatedUsing=OnRep_EquippedItem)
	AILLInventoryItem* EquippedItem;

	// Last item we equipped
	UPROPERTY()
	AILLInventoryItem* LastEquippedItem;

	//////////////////////////////////////////////////
	// Mesh
public:
	/** Tells all inventory items to update their visibility state(s). */
	virtual void UpdateMeshVisibility(const bool bFirstPerson);
};
