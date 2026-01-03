// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Animation/SCAnimInstance.h"
#include "SCCharacter.h"
#include "SCKillerAnimInstance.generated.h"

class ASCKillerCharacter;

/**
 * @class USCKillerAnimInstance
 */
UCLASS()
class SUMMERCAMP_API USCKillerAnimInstance
: public USCAnimInstance
{
	GENERATED_UCLASS_BODY()
	
public:
	// Begin UAnimInstance interface
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	// End UAnimInstance interface

	// Begin USCAnimInstance interface
	virtual float GetMoveSpeedState(const float CurrentSpeed) const override;
	virtual EMovementSpeed GetDesiredStartMoveState() const override;
	virtual EMovementSpeed GetDesiredStopMoveState(const float CurrentSpeed, float& SpeedDifference) const override;
	// End USCAnimInstance interface

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grab")
	TArray<UAnimMontage*> GrabKillMontages;

	/** Upcast of our owner to make accessing data easier (may be NULL because Persona is a JERK) */
	UPROPERTY(BlueprintReadOnly, Category = "Default")
	ASCKillerCharacter* Killer;

	int32 ChooseGrabKillAnim() const;
	void PlayGrabKillAnim(int32 Index = 0);

	FORCEINLINE bool IsGrabKilling() const { return bIsGrabKilling; }

	/** If true, will use the sprint state for fast movement, false will use the run state (fast walk) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Killer")
	bool bCanRun;

	/** Blend Alpha for the Grab State */
	UPROPERTY(BlueprintReadOnly, Category = "Grab")
	float GrabWeight;

	UPROPERTY(BlueprintReadOnly, Category = "IKData")
	FVector LeftHandIKLocation;

	UPROPERTY(BlueprintReadOnly, Category = "IKData")
	FVector RightHandIKLocation;

	UFUNCTION(BlueprintCallable, Category = "IKData")
	void UpdateHandIK(const FVector& LeftHandLocation, const FVector& RightHandLocation);

	UFUNCTION(BlueprintImplementableEvent, Category = "Teleport")
	void OnTeleport();

	/** Play knife throw montage or proceed to throw section */
	UFUNCTION()
	void PlayThrow();

	/** Stop knife throw montage */
	UFUNCTION()
	void EndThrow();

	// The anim instance get initialized before Jason receives his selected weapon so instead of caching it we're just going to use this as a getter.
	UFUNCTION(BlueprintCallable, Category = "Weapon", meta = (BlueprintThreadSafe))
	virtual UAnimSequenceBase* GetCurrentWeaponCombatPose() const override;

	virtual void HandleFootstep(const FName Bone, const bool bJustLanded = false) override;

	/** Update our grab state to grab success if we found a counselor to grab */
	UFUNCTION()
	float UpdateGrabState(bool HasCounselor);

	UFUNCTION(BlueprintPure, Category = "Grab")
	bool IsGrabEnterPlaying() const { return bIsGrabEnterPlaying; }

protected:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Grab")
	bool bIsGrabKilling;

	// Result of Killer->GetGrabbedCounselor()->bIsStruggling (false if no grabbed counselor)
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Grab")
	bool bIsCounselorStruggling;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Grab")
	UAnimSequenceBase* WeaponIdle;

	// Result of Montage_IsPlaying(GrabMontage)
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Grab")
	bool bIsGrabEnterPlaying;

	void OnEndGrabKill();

	UFUNCTION(BlueprintCallable, Category = "Grab")
	void KillGrabbedCounselor();

	/** Will only function if GrabWeight is 1, when it is will clear bIsContextKill on the killer and grabbed counselor */
	UFUNCTION(BlueprintCallable, Category = "Grab")
	void ClearContextKillForGrab();

	UPROPERTY(BlueprintReadOnly, Category = "Abilities")
	bool bIsRageEnabled;

	/** Knife throw anim montage */
	UPROPERTY(EditDefaultsOnly, Category = "Throw")
	UAnimMontage* ThrowMontage;

private:
	FTimerHandle GrabKillTimer;

	///////////////////////////////////////////////////
	// Blueprint Cache
protected:
	// If true, we're in a special move or a montage with root motion
	UPROPERTY(Transient, BlueprintReadOnly, Category = "State Machine")
	bool bShouldIgnoreMovementStateMachine;

	// Value to apply to the neck for looking around (based on grabbing)
	UPROPERTY(Transient, BlueprintReadOnly, Category = "State Machine")
	float GrabWeight_Neck01;

	// Value to apply to the spine for looking around (inverse of GrabWeight_Neck01)
	UPROPERTY(Transient, BlueprintReadOnly, Category = "State Machine")
	float GrabWeight_Spine02;
};
