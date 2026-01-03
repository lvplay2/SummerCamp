// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLSpecialMove_MoveToward.h"
#include "ILLCharacter.h"
#include "ILLSpecialMove_MantleVaultBase.generated.h"

/**
 * @struct FILLMantleVaultCharacterMontage
 */
USTRUCT()
struct ILLGAMEFRAMEWORK_API FILLMantleVaultCharacterMontage
: public FILLCharacterMontage
{
	GENERATED_BODY()

	// Height from the ground to the mantle/vault edge to select this animation set
	UPROPERTY(EditDefaultsOnly)
	float VerticalDelta;

	// Camera shake to play on the local player mantling/vaulting
	UPROPERTY(EditDefaultsOnly, Category="SpecialMove")
	TSubclassOf<UCameraShake> CameraShake;
};

/**
 * @class UILLSpecialMove_MantleVaultBase
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class ILLGAMEFRAMEWORK_API UILLSpecialMove_MantleVaultBase
: public UILLSpecialMove_MoveToward
{
	GENERATED_UCLASS_BODY()

	// Begin UObject interface
	virtual void BeginDestroy() override;
	// End UObject interface

	// Begin UILLSpecialMove_MoveToward interface
	virtual bool GetMoveDestination(const float DeltaSeconds, FVector& OutDestination) override;
	virtual bool GetLookDestination(const float DeltaSeconds, FVector& OutDestination) override;
	virtual void DestinationReached(const FVector& Destination) override;
	// End UILLSpecialMove_MoveToward interface

protected:
	// Root motion animation(s) to play on the owning character
	UPROPERTY(EditDefaultsOnly, Category="SpecialMove")
	TArray<FILLMantleVaultCharacterMontage> AnimationList;

	/** Callback for when the animation(s) fired in DestinationReached complete. */
	virtual void RootMotionSequenceComplete();

private:
	// Timer handle for when the root motion animation completes
	FTimerHandle TimerHandle_AnimComplete;

	// Cached look destination
	FVector LookDestination;

	// Do we have a LookDestination?
	bool bHasLookDestination;
};
