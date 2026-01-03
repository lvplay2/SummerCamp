// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "GameFramework/Actor.h"
#include "ILLCharacter.h"
#include "ILLInventoryItem.generated.h"

class AILLInventoryItem;
class UILLInventoryComponent;

/** Internal equip callback delegate. First argument is the item that just equipped, and the second argument is true when this was made from a replication update. */
DECLARE_DELEGATE_OneParam(FILLInventoryItemEquipDelegate, AILLInventoryItem*);

/** Internal unequip callback delegate. First argument is the item that just unequipped, and the second argument is the next item to equip. */
DECLARE_DELEGATE_TwoParams(FILLInventoryItemUnequipDelegate, AILLInventoryItem*, AILLInventoryItem*);

/**
 * @enum EILLInventoryEquipState
 */
UENUM()
enum class EILLInventoryEquipState : uint8
{
	Unequipped,
	Equipping,
	Equipped,
	Unequipping,
};

/**
 * @class AILLInventoryItem
 */
UCLASS(Abstract, Config=Game, HideCategories=(Actor, ActorTick, Input, Rendering, Replication))
class ILLGAMEFRAMEWORK_API AILLInventoryItem
: public AActor
{
	GENERATED_UCLASS_BODY()

public:
	// Begin AActor interface
	virtual void Destroyed() override;
	virtual void OnRep_Owner() override;
	// End AActor interface

	// Begin UObject interface
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// End UObject interface

	//////////////////////////////////////////////////
	// Management
public:
	/** Notification when we have been added to an inventory. */
	virtual void AddedToInventory(UILLInventoryComponent* Inventory);

	/** Notification when we have been removed from an inventory. */
	virtual void RemovedFromInventory(UILLInventoryComponent* Inventory);

	/** @return ILLCharacter owner. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ILLInventoryItem")
	AILLCharacter* GetCharacterOwner() const;

	/** @return Inventory component this item is within. */
	FORCEINLINE UILLInventoryComponent* GetOuterInventory() const { return OuterInventory; }

	/** @return true if this component is locally controlled. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ILLInventoryItem")
	bool IsLocallyControlled() const;

private:
	/** Inventory this item is within */
	UPROPERTY()
	UILLInventoryComponent* OuterInventory;

	//////////////////////////////////////////////////
	// Equipping and unequipping
public:
	/** Allows sub-classes to specify if this item can be unequipped now, or if it needs to be scheduled for after. */
	virtual bool CanUnequipNow() const;

	/** Allows sub-classes to check if there was a scheduled unequip and handle it. */
	bool CheckScheduledUnequip();

	/** @return true if this item is equippable. */
	FORCEINLINE bool IsEquippable() const { return bEquippable; }

	/** @return true if this item is equipped. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ILLInventoryItem")
	bool IsEquipped() const { return (EquipState == EILLInventoryEquipState::Equipped); }

	/** @return true if this item is being equipped. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ILLInventoryItem")
	bool IsEquipping() const { return (EquipState == EILLInventoryEquipState::Equipping); }

	/** @return true if this item is equipped. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ILLInventoryItem")
	bool IsUnequipped() const { return (EquipState == EILLInventoryEquipState::Unequipped); }

	/** @return true if this item is begin unequipped. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ILLInventoryItem")
	bool IsUnequipping() const { return (EquipState == EILLInventoryEquipState::Unequipping); }

	/** Starts equipping this item, calling to CompletionCallback when done. */
	void StartEquipping(FILLInventoryItemEquipDelegate CompletionCallback, const bool bFromReplication);

	/**
	 * Starts unequipping this item, calling to CompletionCallback on completion with NextItem.
	 * When CanUnequipNow returns false, then 
	 */
	bool StartUnequipping(FILLInventoryItemUnequipDelegate CompletionCallback, AILLInventoryItem* NextItem);

	/** Can this item be equipped? */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipping")
	uint32 bEquippable : 1;

	/** Montage to play when equipping this item */
	UPROPERTY(EditDefaultsOnly, Category = "Equipping", meta=(EditCondition="bEquippable"))
	FILLCharacterMontage EquipMontage;

	/** Equip duration, will only be used if EquipMontage is None */
	UPROPERTY(EditDefaultsOnly, Category = "Equipping", meta=(EditCondition="bEquippable", DisplayName="Equip Duration"))
	float EquipDurationFallback;

	/** Sound to play on equip start */
	UPROPERTY(EditDefaultsOnly, Category = "Equipping", meta=(EditCondition="bEquippable"))
	USoundCue* EquipStartSound;

	/** Sound to play on equip finish */
	UPROPERTY(EditDefaultsOnly, Category = "Equipping", meta=(EditCondition="bEquippable"))
	USoundCue* EquipFinishedSound;

	/** Montage to play when unequipping this item */
	UPROPERTY(EditDefaultsOnly, Category = "Equipping", meta=(EditCondition="bEquippable"))
	FILLCharacterMontage UnequipMontage;

	/** Unequip duration, will only be used if UnequipMontage is None */
	UPROPERTY(EditDefaultsOnly, Category = "Equipping", meta=(EditCondition="bEquippable", DisplayName="Unequip Duration"))
	float UnequipDurationFallback;

	/** Sound to play on unequip start */
	UPROPERTY(EditDefaultsOnly, Category = "Equipping", meta=(EditCondition="bEquippable"))
	USoundCue* UnequipStartSound;

	/** Sound to play on unequip finish */
	UPROPERTY(EditDefaultsOnly, Category = "Equipping", meta=(EditCondition="bEquippable"))
	USoundCue* UnequipFinishedSound;

	/** 
	 * Returns the expected time in second that an equip will take
	 * @param PlaybackRate - Rate at which the equip animation will be played
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipping", meta=(AdvancedDisplay="PlaybackRate"))
	float GetEquipDuration(float PlaybackRate = 1.f) const;

	/** 
	 * Returns the expected time in second that an unequip will take
	 * @param PlaybackRate - Rate at which the unequip animation will be played
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipping", meta=(AdvancedDisplay="PlaybackRate"))
	float GetUnequipDuration(float PlaybackRate = 1.f) const;

protected:
	/** Simulated hook for equip start. */
	virtual void SimulatedStartEquipping();

	/** Simulated hook for equip finish. */
	virtual void SimulatedFinishEquipping();

	/** Simulated hook for unequip start. */
	virtual void SimulatedStartUnequipping();

	/** Simulated hook for unequip finish. */
	virtual void SimulatedFinishUnequipping();

private:
	/** Timer callback for TimerHandle_EquipUnequip. */
	void FinishEquipping(FILLInventoryItemEquipDelegate CompletionCallback, const bool bFromReplication);

	/** Timer callback for TimerHandle_EquipUnequip. */
	void FinishUnequipping(FILLInventoryItemUnequipDelegate CompletionCallback, AILLInventoryItem* NextItem);

	/** Equip state */
	EILLInventoryEquipState EquipState;

	/** Timer handle for equipping and unequipping */
	FTimerHandle TimerHandle_EquipUnequip;

	/** Scheduled unequip delegate */
	FILLInventoryItemUnequipDelegate ScheduledDelegate;

	/** Equip duration, uses montage length when available or EquipDurationFallback if no montage is specified */
	float EquipDuration;

	/** Unequip duration, uses montage length when available or UnequipDurationFallback if no montage is specified */
	float UnequipDuration;

	/** Scheduled unequip item */
	UPROPERTY()
	AILLInventoryItem* ScheduledItem;

	/** Was an unequip scheduled? */
	bool bScheduledUnequip;

	//////////////////////////////////////////////////
	// Attach
public:
	/**
	 * Attaches the item to the character using the specific sockets
	 * @param AttachToBody If true uses BodyAttachSocket, if false uses HandAttachSocket
	 * @param OverrideSocket If set, will ignore the socket specified in the class
	 */
	UFUNCTION(BlueprintCallable, Category = "ILLInventoryItem", meta=(AdvancedDisplay="OverrideSocket"))
	virtual void AttachToCharacter(bool AttachToBody, FName OverrideSocket = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "ILLInventoryItem")
	virtual void DetachFromCharacter();

	/** If true AttachToCharacter will automatically be called when equipping or uneqiupping this item, set to false if this needs to be animation driven */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ILLInventoryItem")
	uint32 bAutoAttach : 1;

	/** Name of the socket to attach to when attaching to the body */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ILLInventoryItem")
	FName BodyAttachSocket;

	/** Name of the socket to attach to when attaching to the hand */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ILLInventoryItem")
	FName HandAttachSocket;

	//////////////////////////////////////////////////
	// Mesh
public:
	/** @return Skeletal mesh component. */
	template <class T = UMeshComponent>
	T* GetMesh(const bool bFirstPerson = false) const { return Cast<T>(bFirstPerson ? ItemMesh1P : ItemMesh3P); }

	/** Updates the visibility if our item mesh(es). */
	virtual void UpdateMeshVisibility(const bool bFirstPerson);

	/** ItemMesh component name */
	static FName ItemMesh3PComponentName;

	/** ItemMesh1P component name */
	static FName ItemMesh1PComponentName;

protected:
	/** Root Component */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USceneComponent* Root;

	/** Item mesh: 3rd person view */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Mesh")
	UMeshComponent* ItemMesh3P;

	/** Item mesh: 1st person view */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Mesh")
	UMeshComponent* ItemMesh1P;

	//////////////////////////////////////////////////
	// Sounds
public:
	/** Plays a sound managed by this item. When the item is destroyed, all playing sounds will automatically be stopped. */
	UFUNCTION(BlueprintCallable, Category = "ILLInventoryItem")
	void PlayItemSound(USoundCue* Sound, const bool bAutoStopOnDestroy = true);

private:
	/** List of item sounds spawned by PlayItemSound */
	UPROPERTY()
	TArray<UAudioComponent*> ItemAudioComponents;

	//////////////////////////////////////////////////
	// Fidgets
protected:
	/** List of fidgets to play when this item is equipped */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<FILLSkeletonIndependentMontage> Fidgets;

public:
	/** 
	 * Gets the subset from Fidgets that can play on the specific character model
	 * @param Character The actor we want to play the montage on (checks against the skeleton)
	 * @param bExactMatchOnly Fidgets will only be valid if the class is an exact match (doesn't also check skeleton)
	 * @return List of matching montages
	 */
	const TArray<FILLCharacterMontage>* GetFidgets(const AILLCharacter* Character, const bool bExactMatchOnly = false) const;
};
