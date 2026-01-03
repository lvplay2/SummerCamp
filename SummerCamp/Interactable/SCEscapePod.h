// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "SCAnimDriverComponent.h"
#include "SCInteractComponent.h"
#include "SCEscapePod.generated.h"

class ASCCounselorAIController;
class ASCRepairPart;
class USCRepairComponent;
class USCScoringEvent;

enum class EILLInteractMethod : uint8;
class USCSpecialMoveComponent;

UENUM(BlueprintType)
enum class EEscapeState : uint8
{
	EEscapeState_Disabled,
	EEscapeState_Repaired,
	EEscapeState_Active,
	EEscapeState_Launched,

	EEscapeState_MAX
};

/**
* @class ASCEscapePod
*/
UCLASS()
class SUMMERCAMP_API ASCEscapePod
	: public AActor
{
	GENERATED_BODY()

public:
	ASCEscapePod(const FObjectInitializer& ObjectInitializer);

	// BEGIN AActor Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	// END AActor Interface

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default")
	USkeletalMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default")
	UStaticMeshComponent* RepairPanel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default")
	class USCInteractComponent* InteractComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default")
	class USCRepairComponent* RepairComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default")
	UStaticMeshComponent* PodScreen;

	// The current state of the escape pod.
	UPROPERTY(ReplicatedUsing=OnRep_PodState, BlueprintReadOnly, Category = "Default")
	EEscapeState PodState;

	/** @return true if the specified part is needed */
	bool IsRepairPartNeeded(TSubclassOf<ASCRepairPart> Part, USCRepairComponent*& AssociatedRepairComponent) const;

	UFUNCTION()
	void OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod);

	UFUNCTION()
	void HoldInteractStateChange(AActor* Interactor, EILLHoldInteractionState NewState);

	UFUNCTION()
	void Native_ActivatePod();

	// Blueprint event to play the door open animation
	UFUNCTION(BlueprintImplementableEvent, Category = "Defualt")
	void ActivatePod();

	UFUNCTION(BlueprintCallable, Category = "Default")
	void BreakPod();

	// The material to create our dynamic instance from
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	UMaterialInstanceConstant* BaseMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Default")
	UMaterialInstanceConstant* BrokenMaterial;

	// Our material instance to update the activation time display.
	UMaterialInstanceDynamic* InstanceMaterial;

	/** @return The time until this pod is activated. -1 if the pod is not repaired. */
	float GetRemainingActivationTime() const;

protected:

	UFUNCTION()
	void OnRep_PodState();

	// how long after its repaired before its usable
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	float UsableTime;

	UPROPERTY(VisibleDefaultsOnly, Category = "Default")
	UParticleSystemComponent* BrokenSparksEmitter;

	UPROPERTY(VisibleDefaultsOnly, Category = "Default")
	USCSpecialMoveComponent* EscapeSpecialMove;

	// Timer handle for tracking time between repair and activation
	UPROPERTY()
	FTimerHandle ActivationTimerHandle;

	// Material to show on the screen when out of order
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	UMaterialInstanceConstant* OutOfOrderMaterial;

	// Material to show on the screen when not repaired
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	UMaterialInstanceConstant* ChipRequiredMaterial;

	// Material to show on the screen when launched
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	UMaterialInstanceConstant* LaunchedMaterial;

	// Material to show on the screen when repaired
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	UMaterialInstanceConstant* ChipInstalledMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	TSubclassOf<USCScoringEvent> RepairScoreEvent;
};