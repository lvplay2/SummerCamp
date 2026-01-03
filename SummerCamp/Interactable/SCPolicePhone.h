// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "SCInteractComponent.h"
#include "SCPolicePhone.generated.h"

class USCScoringEvent;
class USCStatusIconComponent;

/**
 * @class ASCPolicePhone
 */
UCLASS()
class SUMMERCAMP_API ASCPolicePhone
: public AActor
{
	GENERATED_BODY()

public:
	ASCPolicePhone(const FObjectInitializer& ObjectInitializer);

	// Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaTime) override;
	// End AActor Interface

	/** Activated when the juction box is repaired */
	UFUNCTION(BlueprintAuthorityOnly, Category = "Default")
	void Enable(const bool bEnabled);

private:
	// Update Shimmer in the material
	virtual void UpdateShimmer();

public:
	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	TSubclassOf<USCScoringEvent> CallPoliceScoreEvent;

	bool IsPoliceCalled(const bool bJunctionBoxRepaired) const { return bJunctionBoxRepaired ? InteractComponent->bIsEnabled == bJunctionBoxRepaired ?  false : true : false; }

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "UI")
	USCStatusIconComponent* DisabledIcon;

	UFUNCTION()
	void OnInRange(AActor* Interactor);

	UFUNCTION()
	void OnOutOfRange(AActor* Interactor);

	UFUNCTION()
	void CancelPhoneCall();

	/** @return the interact component for this phone */
	USCInteractComponent* GetInteractComponent() const { return InteractComponent; }

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USkeletalMeshComponent* ItemMesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCInteractComponent* InteractComponent;

	// Store our current interactor so we can unlock the interactable when finished
	UPROPERTY(Replicated, Transient)
	ASCCharacter* CurrentInteractor;

	// Animation to play on the phone mesh when used
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	UAnimationAsset* UsePhoneAnimation;

	// Animation to play on the phone mesh when the interact game is failed
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	UAnimationAsset* UsePhoneFailedAnimation;

	// Animation to play on the phone mesh when the interact game is successful
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	UAnimationAsset* UsePhoneSuccessAnimation;

	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod);

	UFUNCTION()
	int32 CanInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation);

	UFUNCTION()
	void HoldInteractStateChange(AActor* Interactor, EILLHoldInteractionState NewState);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_PlayPhoneAnimation(UAnimationAsset* PlayAnim);

	UPROPERTY(Transient, BlueprintReadOnly)
	bool bIsEnabled;

	// Dynamic material, allowing us to make the phone shimmer when enabled
	UPROPERTY()
	UMaterialInstanceDynamic* PhoneMat;

private:
	// Set on local players when they're within interaction range (used to set icons)
	bool bInRange;
};
