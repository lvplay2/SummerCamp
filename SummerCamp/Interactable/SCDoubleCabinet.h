// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCCabinet.h"
#include "SCAnimDriverComponent.h"
#include "SCDoubleCabinet.generated.h"

/**
 * @class ASCDoubleCabinet
 */
UCLASS()
class SUMMERCAMP_API ASCDoubleCabinet
: public ASCCabinet
{
	GENERATED_BODY()

public:
	ASCDoubleCabinet(const FObjectInitializer& ObjectInitializer);

	// BEGIN AActor interface
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// END AActor interface

	// BEGIN ASCCabinet interface
	virtual void UpdateDrawerTickEnabled() override;
	virtual bool SpawnItem(TSubclassOf<ASCItem> ClassOverride = nullptr) override;
	virtual void AddItem(ASCItem* Item, bool bFromSpawn = false, ASCCharacter* Interactor = nullptr) override;
	virtual void RemoveItem(ASCItem* Item) override;
	virtual bool IsCabinetFull() const override;
	virtual void SetAlwaysRelevant(const bool bRelevant) override;
	virtual void DebugDraw(ASCCharacter* Character2) override;
	virtual bool IsCabinetFullyOpened() const { return bIsCabinetFullyOpen2 && Super::IsCabinetFullyOpened(); }
	// END ASCCabinet interface

	UFUNCTION()
	void OnInteract2(AActor* Interactor, EILLInteractMethod InteractMethod);

	UFUNCTION()
	int32 CanInteractWith2(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation);

	/** Called when the character finishes or cancels holding the pickup interact */
	UFUNCTION()
	void OnHoldStateChanged2(AActor* Interactor, EILLHoldInteractionState NewState);

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Cabinet|Item2")
	USceneComponent* ItemAttachLocation2;

	UPROPERTY(ReplicatedUsing=OnRep_ContainedItem2, BlueprintReadOnly, Category = "Cabinet|Item2")
	ASCItem* ContainedItem2;

	UFUNCTION()
	void OpenCloseCabinetDriverBegin2(FAnimNotifyData NotifyData);

	UFUNCTION()
	void OpenCloseCabinetDriverEnd2(FAnimNotifyData NotifyData);

	/** Get the location of the interact component of the second drawer */
	FVector GetDrawerLocation2() const;

	/** @return true if the second drawer is physically opened. */
	FORCEINLINE bool IsDrawer2PhysicallyOpened() const { return bIsOpen2; }

	/** @return true if the second drawer is fully opened for interaction. */
	FORCEINLINE bool IsDrawer2FullyOpened() const { return bIsCabinetFullyOpen2; }

	/** @return Interact component2 */
	USCInteractComponent* GetInteractComponent2() const { return InteractComponent2; }

protected:
	UFUNCTION()
	void OnRep_ContainedItem2(ASCItem* OldItem);

	/** Use special move is done (happens on all clients) */
	UFUNCTION()
	void OpenCloseCabinetCompleted2(ASCCharacter* Interactor);

	/** Called when the use special move starts (happens on all clients) */
	UFUNCTION()
	void UseCabinetStarted2(ASCCharacter* Interactor);

	/** We couldn't get to this cabinet */
	UFUNCTION()
	void UseCabinetAborted2(ASCCharacter* Interactor);

	/** Mesh that "animates" */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UStaticMeshComponent* AnimatedMesh2;

	/** Scene component for the Item to follow (when flagged) */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USceneComponent* FollowLocation2;

	/** Component to spawn items */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCActorSpawnerComponent* ItemSpawner2;

	/** Component to handle press interaction */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCInteractComponent* InteractComponent2;

	/** Moves the drawer/door */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCAnimDriverComponent* OpenCloseCabinetDriver2;

	/** Use the drawer, handles everything (open/close/take) */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSpecialMoveComponent* UseCabinet_SpecialMoveHandler2;

	UPROPERTY(Transient, Replicated, BlueprintReadOnly, Category = "Cabinet|Interaction")
	bool bIsOpen2;

	UPROPERTY(BlueprintReadOnly, Category = "Cabinet|Interaction")
	ASCCharacter* CurrentInteractor2;

	void Close2(bool bTakeItem);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_Close2(bool bTakeItem);

private:
	UPROPERTY(VisibleDefaultsOnly, Category = "Cabinet|Animation")
	FTransform CloseTransform2;

	/** Bone transform at the start of the open animation */
	FTransform InitialTargetBoneTransform2;

	/** Used to track when the open animation finishes */
	bool bIsCabinetFullyOpen2;

	FTimerHandle TimerHandle_CabinetOpen2;

	void FullyOpenedCabinet2();
};
