// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "SCBridgeButton.generated.h"

class USCInteractComponent;

enum class EILLInteractMethod : uint8;

DECLARE_DYNAMIC_DELEGATE(FBridgeButtonActivationDelegate);

/**
 * @class ASCBridgeButton
 */
UCLASS()
class SUMMERCAMP_API ASCBridgeButton
: public AActor
{
	// The bridge is a friend so that it can access the activation delegate and reset the triggers without giving everyone access to them
	friend class ASCBridge;

	GENERATED_UCLASS_BODY()

public:
	// BEGIN AActor interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	// END AActor interface

	/** Interaction method, called from LinkedDoor only */
	UFUNCTION()
	void OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod);

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USceneComponent* Root;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCInteractComponent* InteractComponent;

	// Base material to create our dynamic material from
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UMaterialInstanceConstant* DetatchConsoleMaterial;

	// Material used to modify the color of the button
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UMaterialInstanceDynamic* DynamicMatDetachConsole;

	// What other buttons have to be paired before we're determined to be fully activated?
	UPROPERTY(EditAnywhere, Category = "Default")
	TArray<ASCBridgeButton*> PairedButtons;

	// How long before the trigger is reset?
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default")
	float DeactivationTimer;

	bool GetIsTriggered() const { return bTriggered; }


protected:
	// Has the button been activated?
	UPROPERTY(ReplicatedUsing="OnRep_ButtonTriggered")
	bool bTriggered;

	// Bound from the main bridge object to attempt to activate the bridge escape
	UPROPERTY()
	FBridgeButtonActivationDelegate AttemptBridgeActivation;

	UPROPERTY()
	FTimerHandle BridgeButtonTimer;

	// Handle property updated when activated
	UFUNCTION()
	void OnRep_ButtonTriggered();

	/** Reset that trigger! */
	UFUNCTION()
	void OnResetTrigger();

	/** Destroy the timer so that the button remains #triggered */
	UFUNCTION()
	void KillButtonTimer();
};
