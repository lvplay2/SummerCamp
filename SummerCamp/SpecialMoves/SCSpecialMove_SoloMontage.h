// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLSpecialMove_MoveToward.h"
#include "SCSpecialMove_SoloMontage.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDestinationReachedDelegate);

/**
 * @class USCSpecialMove_SoloMontage
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class SUMMERCAMP_API USCSpecialMove_SoloMontage
: public UILLSpecialMove_MoveToward
{
	GENERATED_UCLASS_BODY()

public:
	// Begin UObject interface
	virtual void BeginDestroy() override;
	// End UObject interface

	// Begin UILLSpecialMoveBase interface
	virtual void BeginMove(AActor* InTargetActor, FOnILLSpecialMoveCompleted::FDelegate InCompletionDelegate) override;
	virtual void TickMove(const float DeltaSeconds) override;
	// End UILLSpecialMoveBase interface

	/** Notification for when our move destination is reached. */
	virtual void DestinationReached(const FVector& Destination) override;

	/** Start our context kill action after destination is reached. Used to synchronize counselor and killer. */
	virtual void BeginAction();

	FORCEINLINE bool IsKillerSpecialMove() const { return bIsKillerSpecialMove; }

	virtual void ForceDestroy();

	FORCEINLINE UAnimMontage* GetContextMontage() const { return ContextMontage; }

	FORCEINLINE bool DieOnFinish() const { return bDieOnFinish; }

	UPROPERTY()
	FDestinationReachedDelegate DestinationReachedDelegate;

	UFUNCTION()
	void SetDesiredTransform(FVector InLocation, FRotator InRotation, bool InRequiresLocation = true);

	UFUNCTION()
	bool GetRequiresLocation() const { return bMoveToward; }

	UFUNCTION()
	void CancelSpecialMove();

	UPROPERTY()
	FDestinationReachedDelegate CancelSpecialMoveDelegate;

	// If false, will use movement input to get the player to the interaction point, if true, just interpolate
	UPROPERTY(EditDefaultsOnly, Category = "Context")
	bool bInterpToInteraction;

	// If bInterpToInteraction is set, time to take to interpolate between starting positiong and interaction point
	UPROPERTY(EditDefaultsOnly, Category = "Context", meta=(EditCondition="bInterpToInteraction"))
	float InterpolationTime;

	/** Resets the finished timer to the passed in time, if less than 0 timer is cleared */
	void ResetTimer(float Time);

	/** Gets the remaining time from our timer, -1 if it is not set */
	float GetTimeRemaining() const;

	/** Gets the amount of time elapsed from our timer, -1 if it is not set */
	float GetTimeElapsed() const;

	/** Gets the amount of time over the total timer length [0->1], -1 if it is not set */
	float GetTimePercentage() const;

	// Used when a call to TrySpeedUp passes and we start playing the faster montage
	UPROPERTY()
	FDestinationReachedDelegate SpeedUpAppliedDelegate;

	/** Returns if this special move could ever move faster */
	bool CanSpeedUp() const { return ContextMontage_Fast != nullptr && SpeedUpTimelimit > 0.f; }

	/**
	 * Actually apply the speed up! (If possible)
	 * @param bForce - Used to skip timelimit to start and force remote clients to play faster (can still fail)
	 * @return Returns true if we started playing faster
	 */
	bool TrySpeedUp(bool bForce = false);

	/** Are we playing the faster version? */
	bool IsSpedUp() const { return bIsPlayingFast; }

	/** Set the montage play rate */
	FORCEINLINE void SetMontagePlayRate(const float PlayRate) { MontagePlayRate = PlayRate; }

	/** Where we want to start the special move from */
	FTransform GetDesiredTransform() const { return DesiredTransform; }

protected:
	/** Notify for when the montage has ended. */
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// Local timer to wait for montage to finish.
	FTimerHandle TimerHandle_DestroyMove;

#if WITH_EDITOR
	// Local timer to abort a special move (editor only)
	FTimerHandle TimerHandle_AlwaysFailAbort;
#endif

	// The montage to play once in position
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context")
	UAnimMontage* ContextMontage;

	// Optional montage to play if the player tries to speed up the interaction
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context")
	UAnimMontage* ContextMontage_Fast;

	// If greater than 0 will allow for pressing interaction again within the timelimt (in seconds) to switch to the faster version of the animation
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context")
	float SpeedUpTimelimit;

	// If true, we're playing the faster version
	UPROPERTY(BlueprintReadOnly, Category = "Context")
	bool bIsPlayingFast;

	// Are rotation and position reached?
	bool bMoveFinished;

	// Is this special meant for the killer?
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Context")
	bool bIsKillerSpecialMove;

	// Should the character die after proforming this special move?
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Context")
	bool bDieOnFinish;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context")
	bool bCallOnFinishedDelegates;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context")
	FName FinishAfterSectionName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context")
	bool bRunToDestination;

	// Should we attempt to do a sweep test while performing the move toward.
	UPROPERTY(EditDefaultsOnly, Category = "SpecialMove: MoveToward")
	bool bSweepTestOnLerp;

	FTransform DesiredTransform;
	FTransform StartingTransform;

	bool bRequiresLocation;
	float TickedTime;

	// Timestamp of when we started the context animation (used for trying to play the fast animation)
	float ActionStart_Timestamp;

	bool bActionStarted;

	bool CharacterRunningState;

	bool bSpecialComplete;

	bool bCanceling;

	float MontagePlayRate;
};
