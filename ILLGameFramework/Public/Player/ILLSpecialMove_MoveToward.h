// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLSpecialMoveBase.h"
#include "ILLSpecialMove_MoveToward.generated.h"

/**
 * @enum EMoveTowardRotatorMode
 */
UENUM()
enum class EMoveTowardRotatorMode : uint8
{
	Epsilon,
	Flatten,
	Free,
	MAX UMETA(Hidden)
};

/**
 * @class UILLSpecialMove_MoveToward
 */
UCLASS(Abstract)
class ILLGAMEFRAMEWORK_API UILLSpecialMove_MoveToward
: public UILLSpecialMoveBase
{
	GENERATED_UCLASS_BODY()

	// Begin UILLSpecialMoveBase interface
	virtual void TickMove(const float DeltaSeconds) override;
	// End UILLSpecialMoveBase interface

	/** @return Move destination. */
	virtual bool GetMoveDestination(const float DeltaSeconds, FVector& OutDestination);

	/** @return Look destination. */
	virtual bool GetLookDestination(const float DeltaSeconds, FVector& OutDestination);

	/** Notification for when our move destination is reached. */
	virtual void DestinationReached(const FVector& Destination);

	/** Notification for when we have an invalid move destination, but still wanted to move. Defaults to calling DestinationReached. */
	virtual void MoveFailed();

protected:
	// Move towards our move destination?
	UPROPERTY(EditDefaultsOnly, Category="SpecialMove: MoveToward")
	bool bMoveToward;

	// Movement scale
	UPROPERTY(EditDefaultsOnly, Category="SpecialMove: MoveToward")
	float MoveScale;

	// Distance threshold for the move destination
	UPROPERTY(EditDefaultsOnly, Category="SpecialMove: MoveToward")
	float MoveThreshold;

	// Look towards our look destination after reaching our destination?
	UPROPERTY(EditDefaultsOnly, Category="SpecialMove: MoveToward")
	bool bLookAfterReachedDestination;

	// Look towards our look destination?
	UPROPERTY(EditDefaultsOnly, Category="SpecialMove: MoveToward")
	bool bLookToward;

	// Rate to change rotation, <0 for instant
	UPROPERTY(EditDefaultsOnly, Category="SpecialMove: MoveToward")
	float LookTowardRate;

	// Snap rotation towards the destination when the move completes?
	UPROPERTY(EditDefaultsOnly, Category="SpecialMove: MoveToward")
	bool bSnapLookOnDestinationReached;

	// How should bLookToward adjust pitch?
	UPROPERTY(EditDefaultsOnly, Category="SpecialMove: MoveToward")
	EMoveTowardRotatorMode LookPitchMode;

	// Epsilon to use when LookPitchMode is EMoveTowardRotatorMode::Epsilon
	UPROPERTY(EditDefaultsOnly, Category="SpecialMove: MoveToward")
	float LookPitchEpsilon;

	// How should bLookToward adjust yaw?
	UPROPERTY(EditDefaultsOnly, Category="SpecialMove: MoveToward")
	EMoveTowardRotatorMode LookYawMode;

	// Epsilon to use when LookYawMode is EMoveTowardRotatorMode::Epsilon
	UPROPERTY(EditDefaultsOnly, Category="SpecialMove: MoveToward")
	float LookYawEpsilon;

	// Have we reached the destination?
	UPROPERTY(BlueprintReadOnly, Transient)
	bool bReachedDestination;
};
