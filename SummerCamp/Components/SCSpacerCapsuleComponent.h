// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Components/CapsuleComponent.h"
#include "SCSpacerCapsuleComponent.generated.h"

class ASCCharacter;

/**
 * @enum ESpacerPushType
 */
UENUM()
enum class ESpacerPushType : uint8
{
	SP_Nearest		UMETA(DisplayName="Nearest"),		// Pushes colliding characters out based on their angle to the center of the capsule.
	SP_StraightBack	UMETA(DisplayName="Straight Back"),	// Pushes colliding charcters to the same point outside the capsule (Along its X axis)
	SP_Grow			UMETA(DisplayName="Grow"),			// The capsule grows and pushes characters out over a specified time.

	SP_MAX UMETA(Hidden)
};

/**
 * @class USCSpacerCapsuleComponent
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SUMMERCAMP_API USCSpacerCapsuleComponent
: public UCapsuleComponent
{
	GENERATED_BODY()

public:
	USCSpacerCapsuleComponent(const FObjectInitializer& ObjectInitializer);

	// Begin UActorComponent interface
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaSeconds, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	// End UActorComponent interface

	/** True if this interaction is blocked. */
	UFUNCTION(BlueprintCallable, Category = "Spacer Capsule")
	bool ActivateSpacer(ASCCharacter* Interactor, float GrowTime = 0.f);

	/** Disable the spacer's collision so characters may once again pass through it. */
	UFUNCTION(BlueprintCallable, Category = "Spacer Capsule")
	void DeactivateSpacer();

protected:
	// The type of spacer this should be. Controls how we want to push characters out of the area.
	UPROPERTY(EditDefaultsOnly)
	ESpacerPushType SpacerType;

	// Collision as set by design
	UPROPERTY(Transient)
	FCollisionResponseContainer DefaultCollision;

	// Interactor with this spacer (should ignore the spacer)
	UPROPERTY(Transient)
	ASCCharacter* SpacerInteractor;

	// Actor classes that should prevent interaction due to this spacer
	UPROPERTY(EditDefaultsOnly)
	TArray<TSoftClassPtr<AActor>> BlockingActors;

	// Actor classes that can be pushed by this spacer
	UPROPERTY(EditDefaultsOnly)
	TArray<TSoftClassPtr<AActor>> PushableActors;

	// The full size unscaled radius of the spacer
	float FullRadius;

	// The total time it should take to grow the spacer if the type is set to Grow
	float TotalGrowTime;

	// The current active time maintained while the spacer is growing.
	float CurrentGrowTime;
};
