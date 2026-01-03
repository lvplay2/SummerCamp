// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Inventory/SCItem.h"
#include "SCAnimDriverComponent.h"

#include "SCThrowingKnifePickup.generated.h"

/**
 * @class ASCThrowingKnifePickup
 */
UCLASS()
class SUMMERCAMP_API ASCThrowingKnifePickup
: public ASCItem
{
	GENERATED_BODY()

public:
	ASCThrowingKnifePickup(const FObjectInitializer& ObjectInitializer);

	// Begin AActor Interface
#if WITH_EDITOR
	virtual void CheckForErrors() override;
#endif
	virtual void PostInitializeComponents() override;
	// End AActor Interface

	// Begin SCItem Interface
	virtual int32 CanInteractWith(class AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation) override;
	virtual void OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod) override;
	// End SCItem Interface

	// Special move to align jason with the pickup
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	class USCSpecialMoveComponent* SpecialMoveAlignment;

	UFUNCTION()
	void PerformPickup(FAnimNotifyData NotifyData);

	// Number of knives to give the player when picked up
	UPROPERTY(EditAnywhere, Category = "Knives")
	float StackSize;

	/** Anim driver to be notified when the item has been 'Picked Up'... If you know what I'm sayin' */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	USCAnimDriverComponent* KnifePickupDriver;

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_DisablePickup();

protected:
	UFUNCTION()
	void OnPickupStart(ASCCharacter* Interactor);

	UFUNCTION()
	void OnPickupCompleted(ASCCharacter* Interactor);

	UFUNCTION()
	void OnPickupAborted(ASCCharacter* Interactor);
};
