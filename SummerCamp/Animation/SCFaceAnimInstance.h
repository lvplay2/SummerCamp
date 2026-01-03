// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Animation/AnimInstance.h"
#include "SCFaceAnimInstance.generated.h"

/**
 * @struct FMorphTargetValues - Used to make sure we don't double set morph target values
 */
USTRUCT(BlueprintType)
struct FMorphTargetValues
{
	GENERATED_BODY()

public:
	// Name of the body curve to get the value from
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default")
	FName CurveName;

	// Name of the morph target
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default")
	FName MorphTargetName;

	// Used so we don't double set values and don't have to look up the new morph value every time
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Default")
	float LastSetValue;
};

/**
 * @class USCFaceAnimInstance - What's wrong with your FACE?!
 */
UCLASS()
class SUMMERCAMP_API USCFaceAnimInstance
: public UAnimInstance
{
	GENERATED_BODY()

public:
	USCFaceAnimInstance(const FObjectInitializer& ObjectInitializer);

	// Begin UAnimInstance interface
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	// End UAnimInstance interface

	// Names of the curves we want to track from the body
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Face")
	TArray<FName> CurveNames;

	// Assigned by SCCharacter, links the head to the body
	UPROPERTY(BlueprintReadOnly, Category = "Face")
	UAnimInstance* BaseAnimInstance;

	/**
	 * Gets a curve value from the body animation instance
	 * @param CurveName - Name of the curve we want (MUST be in CurveNames)
	 * @return Value of the curve on the body, 0 if not found/active
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Face")
	float GetBodyCurveValue(const FName CurveName) const;

	///////////////////////////////////////
	// Morph Targets
protected:

	// Morph targets to update while playing a montage
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Morph Targets")
	TArray<FMorphTargetValues> MontageFaceNames;

	// Morph targets to turn off when a montage starts
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Morph Targets")
	TArray<FName> FacesOfFearNames;

	// Upcast of our owning character
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Morph Targets")
	class ASCCounselorCharacter* Counselor;

	// If true, we dead
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Morph Targets")
	bool bIsDead;

	// If true, the body this face is attached to is playing a montage
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Morph Targets")
	bool bIsBodyPlayingMontage;

	// Name of the body curve to drive JawAlpha
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Morph Targets")
	FName JawAlphaName;

	// Value of the body cruve for JawAlphaName
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Morph Targets")
	float JawAlpha;

	// Name of the body curve to drive BrowFear
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Morph Targets")
	FName BrowFearName;

	// Value of the body cruve for BrowFearName
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Morph Targets")
	float BrowFear;

	// Name of the body curve to drive BrowShock
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Morph Targets")
	FName BrowShockName;

	// Value of the body cruve for BrowShockName
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Morph Targets")
	float BrowShock;

	// Name of the body curve to drive BrowAnger
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Morph Targets")
	FName BrowAngerName;

	// Value of the body cruve for BrowAngerName
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Morph Targets")
	float BrowAnger;

	// Name of the body curve to drive BrowLAlpha
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Morph Targets")
	FName BrowLAlphaName;

	// Value of the body cruve for BrowLAlphaName
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Morph Targets")
	float BrowLAlpha;

	// Name of the body curve to drive BrowRAlpha
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Morph Targets")
	FName BrowRAlphaName;

	// Value of the body cruve for BrowRAlphaName
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Morph Targets")
	float BrowRAlpha;

	// Blink override body curve name
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Morph Targets")
	FName BlinkOverrideCurveName;

	// Blink morph target for the left eye
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Morph Targets")
	FName BlinkLeftName;

	// Blink morph target for the right eye
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Morph Targets")
	FName BlinkRightName;

	///////////////////////////////////////
	// Fear
protected:
	// If fear alpha is greater than this value, we'll play some facial expressions for fear
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear")
	float FearFaceMin;

	// If fear alpha is greater than this value, we'll play some facial expressions for shock
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear")
	float ShockFaceMin;

	// How quickly should the fear face return to neutral after dropping below FearFaceMin
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear")
	float FearToNeutralInterpSpeed;

	// Name of the fear morph target
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear")
	FName FearMorphTargetName;

	// Name of the shock morph target
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear")
	FName ShockMorphTargetName;

	// Curve to drive shock/fear faces based on fear, input time is current game time % max time
	// X output is shock value, Y output is fear value
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear")
	UCurveVector* EmotionCurve;

	// Counselor fear value scaled down to [0->1]
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Fear")
	float FearAlpha;

	// Value we want for the fear morph target based on the current fear value
	float TargetFear;

	// Value we want for the shock morph target based on the current fear value
	float TargetShock;

	///////////////////////////////////////
	// Eyes
public:

	/** Force the eyes closed */
	UFUNCTION(BlueprintCallable, Category = "Face|Eyes")
	void CloseEyes();

	/** Force the eyes open */
	UFUNCTION(BlueprintCallable, Category = "Face|Eyes")
	void OpenEyes();

	/** Enable/disable animation override of the eyes */
	UFUNCTION(BlueprintCallable, Category = "Face|Eyes")
	void AnimationOverridesEyes(bool Override) { bAnimationOverride = Override; }

	// Distance (in cm) to place look point at when looking around
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Face|Eyes")
	float LookDistance;

	// Max yaw (+/-) offset for the eyes to look (from center)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Face|Eyes")
	float MaxEyeLookYawOffset;

	// Max pitch (+/-) offset for the eyes to look (from center)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Face|Eyes")
	float MaxEyeLookPitchOffset;

	// Bone name for the left eye
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Face|Eyes")
	FName LeftEyeBoneName;

	// Bone name for the right eye
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Face|Eyes")
	FName RightEyeBoneName;

	// Rotation of the left eye
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Face|Eyes")
	FRotator LeftEyeRotation;

	// Rotation or the right eye
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Face|Eyes")
	FRotator RightEyeRotation;

	// Used to turn off eye movement during context kills
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Face|Eyes")
	float EyeMovementOverride;

	// Speed the eyes move
	UPROPERTY(EditDefaultsOnly, Category = "Face|Eyes")
	float EyeMovementSpeed;

	// Min time (in seconds) before the eyes look around
	UPROPERTY(EditDefaultsOnly, Category = "Face|Eyes", meta=(ClampMin="0.0"))
	float MinLookUpdateFrequency;

	// Max time (in seconds) before the eyes look around
	UPROPERTY(EditDefaultsOnly, Category = "Face|Eyes", meta = (ClampMin = "0.0"))
	float MaxLookUpdateFrequency;

	// Min weight value for the eyelids and eyelashes
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Face|Eyes", meta=(ClampMin="-1.0", ClampMax="1.0"))
	float MinEyelidWeight;

	// Max weight value for the eyelids and eyelashes
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Face|Eyes", meta=(ClampMin = "-1.0", ClampMax = "1.0"))
	float MaxEyelidWeight;

	// Morph target for eyelids
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Face|Eyes")
	FName EyelidMorphTarget;

	// Morph target for eyelashes
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Face|Eyes")
	FName EyelashMorphTarget;

	// Speed the eyelids and eyelashes move
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Face|Eyes")
	float EyelidMovementSpeed;

	// Min time (in seconds) before the eyes should blink
	UPROPERTY(EditDefaultsOnly, Category = "Face|Eyes", meta = (ClampMin = "0.0"))
	float MinBlinkUpdateFrequency;

	// Max time (in seconds) before the eyes should blink
	UPROPERTY(EditDefaultsOnly, Category = "Face|Eyes", meta = (ClampMin = "0.0"))
	float MaxBlinkUpdateFrequency;

private:
	/** Update where the eyes are looking and what the eyelids are doing */
	void UpdateEyes(float DeltaSeconds);

protected:
	// Should animation override what the eyes are doing
	UPROPERTY(BlueprintReadOnly, Category = "Face|Eyes")
	bool bAnimationOverride;

	// Should the eyes be looking around
	bool bShouldLookAround;

	// Should the eyes blink
	bool bShouldBlink;

	// Desired eye rotation (where the eyes should be looking)
	FRotator DesiredLeftEyeRotation;
	FRotator DesiredRightEyeRotation;
	FVector RelativeLookPosition;
	bool bIsLookingForward;

	// Current eyelid weight
	float CurrentEyelidWeight;

	// Desired eyelid target
	float DesiredEyelidWeight;
	
public:
	// Max time (in seconds) the eyes should focus on the random target
	UPROPERTY(EditDefaultsOnly, Category = "Face|Eyes", meta=(ClampMin="0.0"))
	float MaxLookTime;

private:
	// Time the eyes should keep looking at the random target
	float LookTime;

	// Update time for when the eyes should look around
	float LookUpdateTime;

	// Update time for when the eyes should blink
	float BlinkUpdateTime;

	// Interal map of all curves from the body, cleared/updated every frame
	TMap<FName, float> BodyCurveValues;

	void TryGetParentAnimInstance();
};
