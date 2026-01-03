// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "SCAnimDriverComponent.h"
#include "SCBridgeCore.generated.h"

class ASCRepairPart;

class USCScoringEvent;
class USCRepairComponent;
class USCAnimDriverComponent;

enum class EILLInteractMethod : uint8;

/**
 * @class ASCBarricade
 */
UCLASS()
class SUMMERCAMP_API ASCBridgeCore
: public AActor
{
	GENERATED_UCLASS_BODY()

public:
	// BEGIN AActor interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	// END AActor interface

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USceneComponent* Root;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UStaticMeshComponent* BrokenMesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UStaticMeshComponent* RepairedMesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCRepairComponent* RepairComponent;

	UFUNCTION(BlueprintPure)
	bool GetIsRepaired() const;

	/** @return true if the specified part is needed */
	bool IsRepairPartNeeded(TSubclassOf<ASCRepairPart> Part, USCRepairComponent*& AssociatedRepairComponent) const;

protected:
	/** Interaction method, called from LinkedDoor only */
	UFUNCTION()
	void OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod);

	/** Update our status based on our hold state */
	UFUNCTION()
	void OnHoldStateChanged(AActor* Interactor, EILLHoldInteractionState InteractState);

	/** Were using an anim notify for the core repair to toggle the meshes and hide the equipped item */
	UFUNCTION()
	void OnCoreAnimTrigger(FAnimNotifyData NotifyData);

	/** The notify state needs to be replicated to all clients so that the swap looks correct on all screens */
	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_UpdateNotifyState(AActor* Interactor, bool Canceled);

	/** Our anim driver listener */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCAnimDriverComponent* UseCoreRepairDriver;
};
