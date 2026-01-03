// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLCharacter.h"
#include "ILLVistaCamera.generated.h"

class AILLCharacter;

/**
 * @enum EConstrainAspectRatioActivationType Used to blend between aspect ratio clamping in meaningful ways, otherwise we could end up with verticle letterbox (gross)
 */
UENUM(BlueprintType)
enum class EConstrainAspectRatioActivationType : uint8 // CLEANUP: lpederson: This is named wrong
{
	// Don't constrain at all (no letterboxing)
	None UMETA(DisplayName="None"),
	// Use target camera aspect ratio (horizontal letter box snaps in at the start)
	Imediate UMETA(DisplayName="Imediate"),
	// Blend ratios over time (default Unreal behavior), can result in verticle letter boxing merging to horizontal letterboxing
	OverTime UMETA(DisplayName="Over Time"),
	// RECOMMENDED: Constrain after the current window ratio is passed, prevents verticle letterboxing (horizontal letterboxing smoothly blends in)
	PostRatio UMETA(DisplayName="Post Ratio"),
	// Constrain after camera blend finishes (horizontal letter box snaps in at the end)
	PostBlend UMETA(DisplayName="Post Blend"),

	MAX UMETA(Hidden)
};

/**
 * @struct FBlendSettings Blend settings for allowing specific blend in and out timing, blending function and letterbox behavior
 */
USTRUCT(BlueprintType)
struct FBlendSettings // CLEANUP: lpederson: This is named wrong
{
	GENERATED_BODY()

	/** Default constructor, sets up sane values */
	FBlendSettings();

	/** Should we lock the last frame when blending to or from this camera? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraBlend")
	uint32 bLockOutgoingView : 1;

	/** How to handle a constrained aspect ratio */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraBlend")
	EConstrainAspectRatioActivationType ConstrainAspectRatioBlendType;

	/** How long to blend between the player camera and the vista camera, a value of 0 will result in a smash cut */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraBlend", meta=(ClampMin="0.0", ClampMax="60.0"))
	float BlendTime;

	/** Blend function to use when blending between the player camera and the vista camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraBlend")
	TEnumAsByte<EViewTargetBlendFunction> BlendFunc;

	/** Blend exponent to smooth out non-linear blends */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraBlend", meta=(ClampMin="0.0", ClampMax="100.0"))
	float BlendExponent;
};

/**
 * @class AILLVistaCamera
 * This class supports blending from player camera to a vista view and back again. Has behavior to support 
 * rotating the camera around the vista, blending letterboxing (ConstrainAspectRatioBlendType) and generally 
 * allowing cinematic views to still allow for gameplay.
 * 
 * Player input will need to check against the global camera position rather than just the local player camera
 * when handling input
 */
UCLASS(ClassGroup="Camera", BlueprintType, Blueprintable, HideCategories=(ActorTick,Rendering))
class ILLGAMEFRAMEWORK_API AILLVistaCamera
: public AActor
{
	GENERATED_UCLASS_BODY()

public:
	// Begin AActor interface
	virtual void BeginPlay() override;
	virtual void EnableInput(class APlayerController* PlayerController) override;
	virtual void Tick(float DeltaSeconds) override;
	// End AActor interface

	// Begin UObject interface
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// End UObject interfact

	/** Generic root to support adding trigger volumes at the root */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class USceneComponent* SceneRoot;

	/** Spring arm for smooth interpolation of camera movement */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class USpringArmComponent* SpringArm;

	/** Camera for what with the displaying of the vista */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class UCameraComponent* Camera;

	/** Time to fade out of vista camera, regardless of player input. 0 will never timeout */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraBlend", meta=(ClampMin="0.0", ClampMax="300.0"))
	float TimeLimit;

	/** Values to use when blending into this vista camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraBlend")
	FBlendSettings BlendIn;

	/** Values to use when blending into this vista camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraBlend")
	FBlendSettings BlendOut;

	/** Input binding name to map axis for handling yaw rotation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FName YawInputName;

	/**
	 * Time is handed in as the relative Yaw in degrees
	 * Output value is the target length of the SpringArm
	 * 
	 * Normalized with the EllipsoidalPitchCurve
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	class UCurveFloat* EllipsoidalYawCurve;

	/** If true, clamp values can be set. If false rotation will be unbound */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	uint32 bClampYaw : 1;

	/** Minimum yaw in degrees for rotating the camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta=(ClampMin="-180.0", ClampMax="0.0", EditCondition="bClampYaw"))
	float MinRelativeYaw;

	/** Maximum yaw in degrees for rotating the camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta=(ClampMin="0.0", ClampMax="180.0", EditCondition="bClampYaw"))
	float MaxRelativeYaw;


	/** Input binding name to map axis for handling pitch rotation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FName PitchInputName;

	/**
	 * Time is handed in as the relative Pitch in degrees
	 * Output value is the target length of the SpringArm
	 * 
	 * Normalized with the EllipsoidalYawCurve
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	class UCurveFloat* EllipsoidalPitchCurve;

	/** If true, clamp values can be set. If false rotation will be limited to -89 and 89 to avoid gimbal lock */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	uint32 bClampPitch : 1;

	/** Minimum pitch in degrees for rotating the camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta=(ClampMin="-89.0", ClampMax="0.0", EditCondition="bClampPitch"))
	float MinRelativePitch;

	/** Maximum pitch in degrees for rotating the camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta=(ClampMin="0.0", ClampMax="89.0", EditCondition="bClampPitch"))
	float MaxRelativePitch;

	/**
	 * Starts blending into the vista camera
	 * @param Player Character we want to blend the camera for, MUST have a local player controller attached
	 * @return Time in seconds it will take to blend into the vista camera (modified by BlendIn.BlendTime)
	 */
	UFUNCTION(BlueprintCallable, Category="Camera|Blend")
	float StartBlendIn(AILLCharacter* Player);

	/**
	 * Starts blending out of the vista camera and back into the camera stack defined in the character passed to StartBlendIn
	 * @return Time in seconds it will take to blend out of the vista camera (modified by BlendOut.BlendTime)
	 */
	UFUNCTION(BlueprintCallable, Category="Camera|Blend")
	float StartBlendOut();

private:
	/** Reverts camera and spring are back to default values */
	void ResetCameraSettings();

	/** Not all blend modes need tick enabled, turn ticking on based on the blend mode */
	void UpdateTickEnabledFromBlendType(const bool& bSourceCameraConstrained, const bool& bTargetCameraConstrained, const EConstrainAspectRatioActivationType& BlendType);

	/**
	 * Handle Pitch input
	 * @see PitchInputName
	 */
	UFUNCTION()
	void AddPitch(float Delta);

	/**
	 * Handle Yaw input
	 * @see YawInputName
	 */
	UFUNCTION()
	void AddYaw(float Delta);

	/**
	 * Updates the spring arm length based on relative pitch/yaw offsets
	 * @see EllipsodialYawCurve, EllipsoidalPitchCurve, PitchOffset, YawOffset
	 */
	void UpdateArmLength();

	/**
	 * Callback for auto-exit timer
	 * @see VistaAutoExitTimer
	 */
	void AbortVistaCamera();

	/** If true we're blending in, if false we're blending out, only relevant if tick is enabled */
	uint32 bBlendingIn : 1;

	/** Time in seconds that blending started (in or out) */
	float BlendStartTimestamp;

	/** Time in seconds that the current player entered the vista */
	FTimerHandle VistaAutoExitTimer;

	/** Default camera settings at BeginPlay() */
	FMinimalViewInfo DefaultCameraValues;

	/** Default spring world rotation at BeginPlay() */
	FRotator DefaultSpringArmRotation;

	/** Default arm length at BeginPlay() */
	float DefaultSpringArmLength;

	/** Pitch rotation offset from default position */
	float PitchOffset;

	/** Yaw rotation offset from default position */
	float YawOffset;

	/** Handle to the player controller we're adjusting the view of */
	UPROPERTY()
	APlayerController* ActivePlayerController;

	/** Character grabbed from the player controller */
	UPROPERTY()
	AILLCharacter* ActivePlayerCharacter;
};
