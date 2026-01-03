// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "SCAnimDriverComponent.h"
#include "SCContextComponent.h"
#include "SCFuseBox.generated.h"

enum class EILLInteractMethod : uint8;
enum class EILLHoldInteractionState : uint8;

class AILLCharacter;
class ASCCharacter;
class ASCCounselorAIController;
class ASCPowerGridVolume;
class USCInteractComponent;
class USCSpecialMoveComponent;
class USCScoringEvent;

/**
 * @class ASCFuseBox
 */
UCLASS()
class SUMMERCAMP_API ASCFuseBox
: public AActor
{
	GENERATED_BODY()
	
public:
	ASCFuseBox(const FObjectInitializer& ObjectInitializer);

	// BEGIN AActor Interface
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	// END AActor Interface

	// Visible mesh we'll be breaking and fixing like mad men (or ladies)
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UStaticMeshComponent* Mesh;

	// Handles button presses!
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCInteractComponent* InteractComponent;

	// Special move for breaking the box
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSpecialMoveComponent* DestroyBox_SpecialMoveHandler;

	// Power grid volume to connect to this fuse box
	UPROPERTY(EditAnywhere, Category = "Default")
	ASCPowerGridVolume* PowerGrid;

	/** Handles interaction highlighting based on if the object is broken or not */
	UFUNCTION()
	int32 CanInteractWith(AILLCharacter* Interactor, const FVector& ViewLocation, const FRotator& ViewRotation);

	/** Handles interaction requestes from users */
	UFUNCTION()
	void OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod);

	/** Handle hold interaction changes */
	UFUNCTION()
	void OnHoldStateChanged(AActor* Interactor, EILLHoldInteractionState NewState);

	///////////////////////////////
	// Repair
public:
	/** Blueprint callback when the box is destroyed */
	UFUNCTION(BlueprintImplementableEvent, Category = "PowerGrid")
	void OnActivate();

	///////////////////////////////
	// Destroy
public:
	// Used to line up the animation with the destruction of the box
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCAnimDriverComponent* DestructionDriver;

	/** Deactivate lights in power grid on all clients */
	UFUNCTION()
	void DeactivateGrid(FAnimNotifyData NotifyData);

	/** Blueprint callback when the box is destroyed */
	UFUNCTION(BlueprintImplementableEvent, Category = "PowerGrid")
	void OnDeactivate();

	/** Assigns the owner for the destruction driver */
	UFUNCTION()
	void DestructionStarted(ASCCharacter* Interactor);

	/** Called when the destruction special move completes */
	UFUNCTION()
	void NativeDestructionCompleted(ASCCharacter* Interactor);

	/** Called when the destruction point can't be reached */
	UFUNCTION()
	void NativeDestructionAborted(ASCCharacter* Interactor);

	///////////////////////////////
	// Materials and other extra variables
protected:
	// Light to use when the box is destroyed (set by NativeDestructionCompleted)
	UPROPERTY(EditDefaultsOnly, Category = "PowerGrid")
	UMaterial* BrokenLightMaterial;

	// Light to use when the box is repaired (set by NativeRepairCompleted)
	UPROPERTY(EditDefaultsOnly, Category = "PowerGrid")
	UMaterial* RepairedLightMaterial;

	// Index on Mesh to use when applying the light material
	UPROPERTY(EditDefaultsOnly, Category = "PowerGrid")
	int32 LightMaterialIndex;

	// If true, we broke. Should only be replicated in BeginPlay and then handled by an RPC every other time
	UPROPERTY(ReplicatedUsing=OnRep_Destroyed, Transient, BlueprintReadOnly)
	bool bDestroyed;

	UFUNCTION()
	void OnRep_Destroyed();

public:
	/**
	* Turns the power grid on or off along with effects and mesh lights
	* @param bDestroy - Should the lights be off (true) or on (false)?
	*/
	UFUNCTION(BlueprintCallable, Category = "Default")
	void SetDestroyed(bool bInDestroyed);

protected:
	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_SetDestroyed(bool bInDestroyed);

	// If true, we'll start destroyed
	UPROPERTY(EditAnywhere, Category = "PowerGrid")
	bool bStartDestroyed;

	// You fixed it! My hero!
	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	TSubclassOf<USCScoringEvent> RepairScoreEvent;

	// You broke it! Have points I guess!
	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	TSubclassOf<USCScoringEvent> DestroyScoreEvent;
};
