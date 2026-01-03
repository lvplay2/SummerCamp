// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "SCInteractComponent.h"
#include "SCPoweredObject.h"
#include "SCCBRadio.generated.h"

class ASCCharacter;
class ASCCounselorAIController;
class USCInteractComponent;
class USCScoringEvent;

enum class EILLInteractMethod : uint8;

/**
 * @class ASCCBRadio
 */
UCLASS()
class SUMMERCAMP_API ASCCBRadio
: public AActor
, public ISCPoweredObject
{
	GENERATED_BODY()

public:
	ASCCBRadio(const FObjectInitializer& ObjectInitializer);

	// Begin AActor Interface
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	// End AActor Interface

	// Begin ISCPoweredObject interface
	virtual void SetPowered(bool bHasPower, UCurveFloat* Curve) override;
	virtual void SetPowerCurve(UCurveFloat* Curve) override { PowerCurve = Curve; }
	// End ISCPoweredObject interface

	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	TSubclassOf<USCScoringEvent> CallHunterScoreEvent;

	// If true, materials will be made dynamic and power will be driven by glow. Just putting this in here for when Shane changes his mind.
	UPROPERTY(EditDefaultsOnly, Category = "CB Radio")
	bool bUseDynamicMaterial;

	UFUNCTION(BlueprintPure, Category = "CB Radio")
	bool IsHunterCalled() const { return bHunterCalled; }

	/** @return the interact component for this CB radio */
	USCInteractComponent* GetInteractComponent() const { return InteractComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default")
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default")
	USCInteractComponent* InteractComponent;

	UFUNCTION()
	int32 CanInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation);

	UFUNCTION()
	void OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod);

	/** Handle the state changes for our hold so we can lock the interactable from use by other characters */
	UFUNCTION()
	void HoldInteractStateChange(AActor* Interactor, EILLHoldInteractionState NewState);

	// Cumulative time for use in the PowerCurve when turning on/off
	float CurrentPowerTime;

	// Curve used to turn lights on/off in a dynamic manner
	UPROPERTY(transient)
	UCurveFloat* PowerCurve;

	// Set when the hunter is called so we don't turn this back on when power comes back online
	UPROPERTY(Replicated, Transient)
	bool bHunterCalled;

	// Character using this thing right now
	UPROPERTY(Replicated)
	ASCCharacter* InteractingCharacter;
};
