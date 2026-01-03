// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLSpecialMoveBase.generated.h"

/** Callback delegate for when a special move completes. */
DECLARE_MULTICAST_DELEGATE(FOnILLSpecialMoveCompleted);

/**
 * @class UILLSpecialMoveBase
 */
UCLASS(Abstract, Within=ILLCharacter, Config=Game)
class ILLGAMEFRAMEWORK_API UILLSpecialMoveBase
: public UObject
{
	GENERATED_UCLASS_BODY()

	// Begin UObject interface
	virtual void BeginDestroy() override;
	virtual class UWorld* GetWorld() const override;
	// End UObject interface

	/** Starts this special move. */
	virtual void BeginMove(AActor* InTargetActor, FOnILLSpecialMoveCompleted::FDelegate InCompletionDelegate);

	/** Updates this move every character tick. */
	virtual void TickMove(const float DeltaSeconds) {}

	/** Resets the MoveTimeLimitHit timer to NewTimeout. */
	virtual void ResetTimeout(const float NewTimeout);

	/** @return true if the outer character is allowed to fire during this special move. */
	virtual bool CanFire() const { return bCanFire; }

	/** @return true if the outer character is allowed to move during this special move. */
	virtual bool CanMove() const { return bCanMove; }

	/** @return true if the outer character can rotate on yaw during this special move. */
	virtual bool CanRotateYaw() const { return bCanRotateYaw; }

	/** @return true if the outer character can rotate on pitch during this special move. */
	virtual bool CanRotatePitch() const { return bCanRotatePitch; }

	/** @return The TargetActor for this special move, nullptr if not set. */
	AActor* GetTargetActor() const { return TargetActor; }

	/** Add a custom delegate to the on finished call */
	virtual void AddCompletionDelegate(FOnILLSpecialMoveCompleted::FDelegate InCompletionDelegate);

	/** Remove all completion deleates for a given object */
	virtual void RemoveCompletionDelegates(UObject* ForObject);

	/** Returns the time remaining for the special move timer (-1 if the timer is not running) */
	float GetTimeRemaining() const;

protected:
	/** Called when MoveTimeLimit is hit. */
	virtual void MoveTimeLimitHit();

	// Can we fire while performing this special move?
	UPROPERTY(EditDefaultsOnly, Category="SpecialMove")
	bool bCanFire;

	// Can we move while performing this special move?
	UPROPERTY(EditDefaultsOnly, Category="SpecialMove")
	bool bCanMove;

	// Can we rotate on yaw while performing this special move?
	UPROPERTY(EditDefaultsOnly, Category="SpecialMove")
	bool bCanRotateYaw;

	// Can we rotate on pitch while performing this special move?
	UPROPERTY(EditDefaultsOnly, Category="SpecialMove")
	bool bCanRotatePitch;

	// Optional target actor for the special move
	UPROPERTY()
	AActor* TargetActor;

	// Time limit setting, to prevent moves from getting stuck forever
	float MoveTimeLimit;

	// Timer handle for MoveTimeLimit
	FTimerHandle TimerHandle_TimeLimit;

protected:
	// Completion delegate
	FOnILLSpecialMoveCompleted CompletionDelegate;
};
