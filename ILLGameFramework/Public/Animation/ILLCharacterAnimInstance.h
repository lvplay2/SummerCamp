// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Animation/AnimInstance.h"
#include "ILLCharacterAnimInstance.generated.h"

extern ILLGAMEFRAMEWORK_API TAutoConsoleVariable<int32> CVarILLDebugAnimation;
extern ILLGAMEFRAMEWORK_API TAutoConsoleVariable<float> CVarILLDebugAnimationLingerTime;

/**
 * @class UILLCharacterAnimInstance
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLCharacterAnimInstance
: public UAnimInstance
{
	GENERATED_BODY()

public:
	UILLCharacterAnimInstance(const FObjectInitializer& ObjectInitializer);

	// Begin UAnimInstance interface
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeUninitializeAnimation() override;
	// End UAnimInstance interface

	/** Gets unblended (except for those in a blendspace) curve values, if the curve is unsued returns 0 */
	UFUNCTION(BlueprintPure, Category = "ILLAnimInstance")
	float GetUnblendedCurveValue(const FName& CurveName) const { return HasUnblendedCurveValue(CurveName) ? UnblendedCurveValues[CurveName] : 0.f; }

	/**
	 * If true, we have the unblended curve value in our map
	 * If false, the curve is currently not being used or you need  to enable bAutomaticallyCollectRawCurves or call UpdateRawCurves
	 */
	UFUNCTION(BlueprintPure, Category = "ILLAnimInstance")
	bool HasUnblendedCurveValue(const FName& CurveName) const { return UnblendedCurveValues.Contains(CurveName); }

protected:
	// If true, we will pull all the raw curves from the animations currently playing (not free!)
	// Turn this off if you're not using it and want some framerate back (defaults off)
	bool bAutomaticallyCollectRawCurves;

	// Updated from UpdateRawCurves to support manual updating of curves
	float RawCurvesLastUpdated_TimeStamp;

	/** Called from NativeUpdateAnimation if bAutomaticallyCollectRawCurves is true, will update RawCurvesLastUpdated_TimeStamp regardless */
	void UpdateRawCurves();

	/** Used to log animation to the screen/log with frame numbers */
	virtual void Log(const FColor Color, const bool bShouldLinger, const FString& DebugString) const;

private:
	// Maintained by UpdateRawCurves, use GetMaxCurveValue to access
	TMap<FName, float> UnblendedCurveValues;
};
