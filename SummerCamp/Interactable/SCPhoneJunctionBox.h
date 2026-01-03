// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "SCAnimDriverComponent.h"
#include "SCInteractComponent.h"
#include "SCPhoneJunctionBox.generated.h"

class ASCCounselorAIController;
class ASCPolicePhone;
class ASCRepairPart;
class USCRepairComponent;
class USCScoringEvent;

enum class EILLInteractMethod : uint8;

/**
 * @class ASCPhoneJunctionBox
 */
UCLASS()
class SUMMERCAMP_API ASCPhoneJunctionBox
: public AActor
{
	GENERATED_BODY()

public:
	ASCPhoneJunctionBox(const FObjectInitializer& ObjectInitializer);

	// BEGIN AActor Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	// END AActor Interface

	UFUNCTION(BlueprintPure, Category = "Phone")
	bool IsPhoneBoxRepaired() const { return !bIsBroken; }

	UFUNCTION(BlueprintPure, Category="Phone")
	bool HasFuse() const {	return bHasFuse; }

	/** @return true if the specified part is needed */
	bool IsRepairPartNeeded(TSubclassOf<ASCRepairPart> Part, USCRepairComponent*& AssociatedRepairComponent) const;

	/** @return the interact component for this phone box */
	USCInteractComponent* GetInteractComponent() const { return InteractComponent; }

	/** @return the phone this phone box is linked to */
	ASCPolicePhone* GetConnectedPhone() const { return ConnectedPhone; }

	UFUNCTION(BlueprintPure, Category = "Phone")
	bool IsPoliceCalled() const;

	UFUNCTION()
	void DestroyBoxNotify(FAnimNotifyData NotifyData);

	UFUNCTION()
	void DestroyBoxStarted(ASCCharacter* Interactor);

	UFUNCTION()
	void DestroyBox(ASCCharacter* Interactor);

	/** Called when the killer gets interupted while trying to break the junction box */
	UFUNCTION()
	void DestroyBoxAborted(ASCCharacter* Interactor);

	/** Blueprint event to handle VFX */
	UFUNCTION(BlueprintImplementableEvent, Category = "Phone")
	void OnPhoneBoxDestroyed();

	/** Blueprint event to handle VFX */
	UFUNCTION(BlueprintImplementableEvent, Category = "Phone")
	void OnPhoneBoxRepaired();

	UFUNCTION(BlueprintNativeEvent, Category = "Default")
	int32 CanInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation);

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UParticleSystem* DestroyedEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	USceneComponent* DestroyedEffectLocation;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default")
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default")
	class USCInteractComponent* InteractComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	class USCSpecialMoveComponent* DestroyBox_SpecialMoveHandler;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCAnimDriverComponent* DestructionDriver;

	UPROPERTY(EditDefaultsOnly)
	UMaterial* BrokenLightMaterial;

	UPROPERTY(EditDefaultsOnly)
	UMaterial* RepairedLightMaterial;

	UPROPERTY(EditDefaultsOnly)
	int32 LightMaterialIndex;

	/** The phone paired with this juction box */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	ASCPolicePhone* ConnectedPhone;

	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod);

	/** Handle the state changes for our hold so we can lock the interactable from use by other characters */
	UFUNCTION()
	void HoldInteractStateChange(AActor* Interactor, EILLHoldInteractionState NewState);

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Default")
	bool bHasFuse;

	UPROPERTY(ReplicatedUsing = OnRep_IsBroken, BlueprintReadOnly, Category = "Default")
	bool bIsBroken;

	UFUNCTION()
	void OnRep_IsBroken();

	UPROPERTY()
	UParticleSystemComponent* DamagedEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	TSubclassOf<USCScoringEvent> RepairScoreEvent;
};
