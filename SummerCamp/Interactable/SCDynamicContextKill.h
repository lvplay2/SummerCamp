// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "SCAnimDriverComponent.h"
#include "SCContextKillActor.h"
#include "SCDynamicContextKill.generated.h"

/**
 * @class ASCDynamicContextKill
 */
UCLASS()
class SUMMERCAMP_API ASCDynamicContextKill
: public ASCContextKillActor
{
	GENERATED_BODY()

public:
	ASCDynamicContextKill(const FObjectInitializer& ObjectInitializer);

	// BEGIN AActor interface
	virtual void BeginPlay();
	// END AActor interface

	/** Activate the ContextKillComponent */
	UFUNCTION()
	virtual void ActivateKill(AActor* Interactor, EILLInteractMethod InteractMethod) override;

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_DetachFromParent();

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_DetachFromParent();

	/** Move the dynamic volume to new location */
	UFUNCTION()
	void PlaceVolume(const FVector NewLocation, const FRotator NewRotation, AActor* Interactor, bool bForcePosition = false);

	/** Return maximum preview distance */
	UFUNCTION()
	float GetPreviewDistance() const;

	/** Returns true if the Kill volume is unobstructed. */
	UFUNCTION()
	bool DoesKillVolumeFit(const AActor* Interactor) const;

	/** Disable Kill preview */
	UFUNCTION()
	void ForceDisableCustomRender();

	// Callback for when the kill finishes so we can attach the kill back to Jason
	UFUNCTION()
	void ReattachKill(ASCCharacter* Interactor, ASCCharacter* Victim);

protected:
	/** Store the previous location and rotation in case the volume doesn't fit the new location */
	FVector PreviousLocation;
	FRotator PreviousRotation;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UBoxComponent* KillBounds;

	UPROPERTY(EditAnywhere)
	bool bReattachToKiller;
};
