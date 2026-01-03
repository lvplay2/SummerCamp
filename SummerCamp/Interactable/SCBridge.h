// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "SCBridge.generated.h"

class ASCBridgeButton;

/**
 * @class ASCBridge
 */
UCLASS()
class SUMMERCAMP_API ASCBridge
: public AActor
{
	GENERATED_UCLASS_BODY()

public:
	// BEGIN AActor interface
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	// END AActor interface

	// Volume used to tell if Jason is on the bridge at the time of activation
	UPROPERTY(VisibleDefaultsOnly, Category = "Bridge Escape")
	UBoxComponent* JasonDetector;

	// Our list of buttons so we can reset the triggers if the activation fails
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bridge Escape")
	TArray<ASCBridgeButton*> BridgeButtons;

	/** Bound to the buttons so that when they are activated the bridge is alerted */
	UFUNCTION()
	void AttemptBridgeEscape();

	/** Used to play the animation for the doors and activate the escape volume */
	UFUNCTION(BlueprintImplementableEvent, Category = "Bridge Escape")
	void BP_ActivateBridgeEscape();

	/** Multicast the actual activation to all clients */
	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_ActivateBridgeEscape();

	UPROPERTY(BlueprintReadOnly, Category = "Bridge Escape")
	bool bDetached;
};
