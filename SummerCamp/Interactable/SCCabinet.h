// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"

#include "ILLInteractableComponent.h"

#include "SCAnimDriverComponent.h"

#include "SCCabinet.generated.h"

class ASCCharacter;
class ASCCounselorAIController;
class ASCItem;
class USCActorSpawnerComponent;
class USCInteractComponent;
class USCSpecialMoveComponent;

enum class EILLInteractMethod : uint8;

static const FName CabinetOpen_NAME(TEXT("Open"));
static const FName CabinetIdle_NAME(TEXT("Idle"));
static const FName CabinetClose_NAME(TEXT("Close"));
static const FName CabinetTake_NAME(TEXT("Take"));
static const FName CabinetTakeClose_NAME(TEXT("TakeClose"));
static const FName CabinetTargetBone_NAME(TEXT("target_jnt"));

/**
 * @class ASCCabinet
 */
UCLASS()
class SUMMERCAMP_API ASCCabinet
: public AActor
{
	GENERATED_BODY()

public:
	ASCCabinet(const FObjectInitializer& ObjectInitializer);

	// BEGIN AActor interface
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// END AActor interface

	virtual void DebugDraw(ASCCharacter* Character);

	virtual void UpdateDrawerTickEnabled();

	virtual bool SpawnItem(TSubclassOf<ASCItem> ClassOverride = nullptr);

	virtual void AddItem(ASCItem* Item, bool bFromSpawn = false, ASCCharacter* Interactor = nullptr);
	virtual void RemoveItem(ASCItem* Item);

	UFUNCTION()
	void OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod);

	UFUNCTION()
	int32 CanInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation);

	/** Called when the character finishes or cancels holding the pickup interact */
	UFUNCTION()
	void OnHoldStateChanged(AActor* Interactor, EILLHoldInteractionState NewState);

	// Scene Component to use for positioning the item. Needs to be set in defaults
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="Cabinet|Item")
	USceneComponent* ItemAttachLocation;

	UPROPERTY(ReplicatedUsing = OnRep_ContainedItem, BlueprintReadOnly, Category="Cabinet|Item")
	ASCItem* ContainedItem;

	UFUNCTION()
	void OpenCloseCabinetDriverBegin(FAnimNotifyData NotifyData);

	UFUNCTION()
	void OpenCloseCabinetDriverEnd(FAnimNotifyData NotifyData);

	// Component to spawn items
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="Default")
	USCActorSpawnerComponent* ItemSpawner;

	UFUNCTION()
	virtual bool IsCabinetFull() const;

	/** Get the location of the interact component of the drawer */
	FVector GetDrawerLocation() const;

	/** @return true if this cabinet is open (virtual so SCDoubleCabinet can override). */
	virtual bool IsCabinetFullyOpened() const { return bIsCabinetFullyOpen; }

	/** @return true if the first drawer is physically opened. */
	FORCEINLINE bool IsDrawerPhysicallyOpened() const { return bIsOpen; }

	/** @return true if the first drawer is fully opened for interaction. */
	FORCEINLINE bool IsDrawerFullyOpened() const { return bIsCabinetFullyOpen; }

	/** @return Interact component */
	USCInteractComponent* GetInteractComponent() const { return InteractComponent; }

protected:
	UFUNCTION()
	void OnRep_ContainedItem(ASCItem* OldItem);

	/** Use special move is done (happens on all clients) */
	UFUNCTION()
	void OpenCloseCabinetCompleted(ASCCharacter* Interactor);

	/** Called when the use special move starts (happens on all clients) */
	UFUNCTION()
	void UseCabinetStarted(ASCCharacter* Interactor);

	/** We couldn't get to this cabinet */
	UFUNCTION()
	void UseCabinetAborted(ASCCharacter* Interactor);

	// Base Mesh for the cabinet
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="Default")
	UStaticMeshComponent* BaseMesh;

	// Mesh that "animates"
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="Default")
	UStaticMeshComponent* AnimatedMesh;

	// Scene component for the Item to follow (when flagged)
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="Default")
	USceneComponent* FollowLocation;

	// Component to handle press interaction
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="Default")
	USCInteractComponent* InteractComponent;

	// Moves the drawer/door
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="Default")
	USCAnimDriverComponent* OpenCloseCabinetDriver;

	// Use the drawer, handles everything (open/close/take)
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="Default")
	USCSpecialMoveComponent* UseCabinet_SpecialMoveHandler;

	// Track whether the cabinet is open or closed
	UPROPERTY(Transient, Replicated, BlueprintReadOnly, Category="Cabinet|Interaction")
	bool bIsOpen;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cabinet|Interaction")
	bool bItemShouldFollowAnimatedMesh;

	UPROPERTY(BlueprintReadOnly, Category="Cabinet|Interaction")
	ASCCharacter* CurrentInteractor;

	void Close(bool bTakeItem);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_Close(bool bTakeItem);

protected:
	// Time (in seconds) into the player animation to grab the inital bone transform from
	UPROPERTY(EditDefaultsOnly, Category="Cabinet|Animation")
	float InitialTargetBoneTime;

	/** Allow logic to toggle always relevant on/off for cabinets */
	virtual void SetAlwaysRelevant(const bool bRelevant);

private:
	UPROPERTY(VisibleDefaultsOnly, Category="Cabinet|Animation")
	FTransform CloseTransform;

	// Bone transform at the start of the open animation
	FTransform InitialTargetBoneTransform;

	// Used to track when the open animation finishes
	bool bIsCabinetFullyOpen;

	FTimerHandle TimerHandle_CabinetOpen;

	void FullyOpenedCabinet();
};
