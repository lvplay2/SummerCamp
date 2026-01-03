// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLInventoryItem.h"

#include "SCItem.generated.h"

enum class EILLInteractMethod : uint8;

class ASCCharacter;
class ASCCounselorAIController;

class USCMinimapIconComponent;
class USCScoringEvent;

/**
 * @class ASCItem
 */
UCLASS()
class SUMMERCAMP_API ASCItem
: public AILLInventoryItem
{
	GENERATED_UCLASS_BODY()

	// Begin AActor interface
	virtual void PostInitializeComponents() override;
	virtual void Destroyed() override;
	virtual void BeginPlay() override;
	virtual void SetOwner(AActor* NewOwner) override;
	virtual void OnRep_Owner() override;
	// End AActor interface

	// Begin AILLInventoryItem interface
	virtual void AttachToCharacter(bool bAttachToBody, FName OverrideSocket = NAME_None) override;
	virtual void DetachFromCharacter() override;
	// End AILLInventoryItem interface

protected:
	/** Called from OnRep_Owner and SetOwner */
	virtual void OwningCharacterChanged(ASCCharacter* OldOwner);

public:
	// Pointer to the shore spawner we're currently using so we can mark it as no longer in use when we get picked back up (SERVER only)
	UPROPERTY()
	class ASCShoreItemSpawner* CurrentShoreSpawner;

	/**
	 * Tests if a character can interact with this item
	 * @param Character attempting to interact with this item
	 * @param Location of interactor
	 * @param Rotation of the interactor
	 * @return true if Character can interact with this item, otherwise false\
	 */
	UFUNCTION()
	virtual int32 CanInteractWith(class AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation);

	/**
	 * Adds item to appropriate inventory
	 * @param Actor interacting with this item
	 */
	UFUNCTION()
	virtual void OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod);

	/**
	 * Uses the item (override in child classes), returns true if one use item (should destroy after use)
	 * @return True if item should be destroyed after use
	 */
	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual bool Use(bool bFromInput);

	/** Tells us if we're in a cabinet or not */
	UFUNCTION(BlueprintPure, Category = "Item")
	FORCEINLINE bool IsInCabinet() const { return CurrentCabinet != nullptr;}

	/**
	 * Called when an item is dropped or placed, performs ray traces to try and keep the item accessible and resting on the ground
	 * @param DroppingCharacter - If passed in, will automatically be ignored for all traces and be used for occlusion checking, pass NULL if you want to skip the occlusion check
	 * @param AtLocation - Desired location to drop the item, if ZeroVector is passed in a location will be picked based on the Item ID
	 */
	virtual void OnDropped(ASCCharacter* DroppingCharacter, FVector AtLocation = FVector::ZeroVector);

	virtual void EnableInteraction(bool bInEnable);

	FORCEINLINE const FText& GetFriendlyName() const { return FriendlyName; }

	virtual bool ShouldAttach(bool bFromInput);

	UFUNCTION(BlueprintCallable, BlueprintPure,  Category = "Item")
	virtual bool CanUse(bool bFromInput);

	/** Gets the max number of this type of item we can have in our inventory (0 is no limit) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item")
	FORCEINLINE int32 GetInventoryMax() const { return InventoryMax; }

	// Large or small item
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	bool bIsLarge;

	// This item is neither large or small and should be handled differently.
	// These items are usually kept on the player for the duration of the match.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	bool bIsSpecial;

	// Cabinet currently holding this item
	UPROPERTY(Transient, ReplicatedUsing=OnRep_CurrentCabinet)
	class ASCCabinet* CurrentCabinet;

	// If true, this is being picked up or dropped
	UPROPERTY(Transient, Replicated)
	bool bIsPicking;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear")
	TSubclassOf<class USCFearEventData> PickedUpFearEvent;

	// The Item we want to swap this one to on use
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TSubclassOf<ASCItem> SwapToOnUse;

	/** Rep call for when CurrentCabinet is set to ensure the CurrentCabinet is looking back at us too */
	UFUNCTION()
	void OnRep_CurrentCabinet();

	// if our origin is not in the center of our object we need to set this based on whatever the actual objects center is.
	UPROPERTY(EditDefaultsOnly)
	FVector OriginOffset;

protected:
	// Max number of this item that a player can hold (0 for no limit)
	UPROPERTY(EditDefaultsOnly, Category = "Item")
	int32 InventoryMax;

	/////////////////////////////////////////////////////////////////////////////
	// Animation
public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Animation")
	virtual UAnimMontage* GetUseItemMontage(const USkeleton* Skeleton) const { return GetBestAsset(Skeleton, UseMontages); }

	/** Finds the best idle pose for the passed in skeleton, may return None if no match is found */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Animation")
	virtual UAnimSequenceBase* GetIdlePose(const USkeleton* Skeleton) const { return GetBestAsset(Skeleton, IdleAsset); }

	void EnableSimulatePhysics(bool bEnable);

	UFUNCTION(BlueprintPure, Category = "Item|Interaction")
	class USCInteractComponent* GetInteractComponent() const { return InteractComponent; }

	// List of assets for all possible users of this item, use GetIdlePose() to find a matching asset for your character
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Animation")
	TArray<UAnimSequenceBase*> IdleAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Animation")
	TArray<UAnimMontage*> UseMontages;

	// Rotation to use when item is in a cabinet
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	FRotator CabinetRotation;

protected:
	/** Handles an array of assets and finds a best fit for the passed in skeleton */
	UAnimSequenceBase* GetBestAsset(const USkeleton* Skeleton, const TArray<UAnimSequenceBase*>& AssetArray) const;

	UAnimMontage* GetBestAsset(const USkeleton* Skeleton, const TArray<UAnimMontage*>& AssetArray) const;

	// ~Animation
	/////////////////////////////////////////////////////////////////////////////

public:
	// Minimap icon so important items show up on your map when dropped
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default")
	USCMinimapIconComponent* MinimapIconComponent;

	/** Utility function to turn shimmer on/off */
	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual void SetShimmerEnabled(const bool bEnabled);

	/** Let the item know that it is currently being used, just in case the actual USE call comes later */
	UFUNCTION()
	virtual void BeginItemUse();

	/** Let the item know that it is done being used. */
	UFUNCTION()
	virtual void EndItemUse();

	UFUNCTION(BlueprintCallable, Category = "Default")
	bool GetIsInUse() const { return bIsInUse; }

protected:
	// Component that handles interaction
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	class USCInteractComponent* InteractComponent;

	// Friendly name for this item
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Default")
	FText FriendlyName;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	UTexture2D* ImageOn;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	UTexture2D* ImageOff;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	UTexture2D* HUDPickupIcon;

	// Num uses will trach how many times this object could potentially be used by the character holding it.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_NumUses)
	int32 NumUses;

	// Tracks how many times the item has been used thus far so that it is destroyed properly for whoever is holding it.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_NumUses)
	int32 TimesUsed;

	UPROPERTY(BlueprintReadOnly, Replicated)
	bool bIsInUse;

	UFUNCTION()
	void OnRep_NumUses();

	// If true (default) item will shimmer when not held
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	bool bCanShimmer;

	// If true (default) item will start shimmering, if false will only shimmer when dropped
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	bool bStartsShimmering;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_Shadows)
	bool bShouldCastShadows;

	UPROPERTY()
	bool bWasCastingShadows;

	UFUNCTION()
	void OnRep_Shadows();

	// Scoring, I guess
public:
	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	TSubclassOf<USCScoringEvent> PickupScoreEvent;

protected:
	// Simulated by the replicated owner, used to drop an item in the correct direction
	UPROPERTY(Transient)
	ASCCharacter* OwningCharacter;

	// Rotation to use when dropped
	UPROPERTY(EditDefaultsOnly, Category = "Item")
	FRotator DropRotation;

	// Amount to sacle the highlight when an item is dropped
	UPROPERTY(EditDefaultsOnly, Category = "Item")
	float HightlightScaleOnDropped;

	// Actual value to set the highlight to when it's dropped
	float HighlightSizeOnDropped;

	bool bShouldOffsetPivotOnDrop;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Sound")
	TAssetPtr<USoundBase> DroppedSoundCue;

protected:
	UFUNCTION(NetMulticast, Reliable)
	void MULTI_DeferredPlayDroppedSound();

	UFUNCTION()
	void PlayDroppedSound();

private:
	// this is used to ensure items arent placed on top of eachother
	static uint8 RotationIndex;
};
