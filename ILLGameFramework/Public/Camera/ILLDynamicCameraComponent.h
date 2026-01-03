// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Components/SceneComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "ILLDynamicCameraComponent.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UCurveFloat;

USTRUCT(BlueprintType)
struct FCameraBehaviorSettings
{
	GENERATED_USTRUCT_BODY()

	FCameraBehaviorSettings();
	void Apply(class UILLDynamicCameraComponent* DynamicCamera) const;
	static FCameraBehaviorSettings Lerp(const FCameraBehaviorSettings& A, const FCameraBehaviorSettings& B, float Alpha);

	/* Spring Arm Overrides */
	UPROPERTY(BlueprintReadWrite, Category = "Overrides", meta=(PinHiddenByDefault))
	uint32 bOverride_TargetArmLength : 1;
	UPROPERTY(BlueprintReadWrite, Category = "Overrides", meta=(PinHiddenByDefault))
	uint32 bOverride_SocketOffset : 1;
	UPROPERTY(BlueprintReadWrite, Category = "Overrides", meta=(PinHiddenByDefault))
	uint32 bOverride_TargetOffset : 1;
	UPROPERTY(BlueprintReadWrite, Category = "Overrides", meta=(PinHiddenByDefault))
	uint32 bOverride_UsePawnControlRotation : 1;
	UPROPERTY(BlueprintReadWrite, Category = "Overrides", meta=(PinHiddenByDefault))
	uint32 bOverride_InheritPitch : 1;
	UPROPERTY(BlueprintReadWrite, Category = "Overrides", meta=(PinHiddenByDefault))
	uint32 bOverride_InheritYaw : 1;
	UPROPERTY(BlueprintReadWrite, Category = "Overrides", meta=(PinHiddenByDefault))
	uint32 bOverride_InheritRoll : 1;
	UPROPERTY(BlueprintReadWrite, Category = "Overrides", meta=(PinHiddenByDefault))
	uint32 bOverride_EnableCameraLag : 1;
	UPROPERTY(BlueprintReadWrite, Category = "Overrides", meta=(PinHiddenByDefault))
	uint32 bOverride_EnableCameraRotationLag : 1;
	UPROPERTY(BlueprintReadWrite, Category = "Overrides", meta=(PinHiddenByDefault))
	uint32 bOverride_UseCameraLagSubstepping : 1;
	UPROPERTY(BlueprintReadWrite, Category = "Overrides", meta=(PinHiddenByDefault))
	uint32 bOverride_CameraLagSpeed : 1;
	UPROPERTY(BlueprintReadWrite, Category = "Overrides", meta=(PinHiddenByDefault))
	uint32 bOverride_CameraRotationLagSpeed : 1;
	UPROPERTY(BlueprintReadWrite, Category = "Overrides", meta=(PinHiddenByDefault))
	uint32 bOverride_CameraLagMaxTimeStep : 1;
	UPROPERTY(BlueprintReadWrite, Category = "Overrides", meta=(PinHiddenByDefault))
	uint32 bOverride_CameraLagMaxDistance : 1;

	/* Camera Overrides */
	UPROPERTY(BlueprintReadWrite, Category = "Overrides", meta=(PinHiddenByDefault))
	uint32 bOverride_FieldOfView : 1;
	UPROPERTY(BlueprintReadWrite, Category = "Overrides", meta=(PinHiddenByDefault))
	uint32 bOverride_OrthoWidth : 1;
	UPROPERTY(BlueprintReadWrite, Category = "Overrides", meta=(PinHiddenByDefault))
	uint32 bOverride_OrthoNearClipPlane : 1;
	UPROPERTY(BlueprintReadWrite, Category = "Overrides", meta=(PinHiddenByDefault))
	uint32 bOverride_OrthoFarClipPlane : 1;
	UPROPERTY(BlueprintReadWrite, Category = "Overrides", meta=(PinHiddenByDefault))
	uint32 bOverride_AspectRatio : 1;
	UPROPERTY(BlueprintReadWrite, Category = "Overrides", meta=(PinHiddenByDefault))
	uint32 bOverride_UseFieldOfViewForLOD : 1;

	// Minimum speed to enter into this camera setting
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blend")
	float MinimumCharacterSpeed;

	// Exponent value to adjust the curve of the ease in/out on MovementBasedSettings blends (1.0 for linear blend)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blend")
	float BlendExponent;

	// Blend operation to use while switching between movement based settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blend")
	TEnumAsByte<EViewTargetBlendFunction> BlendFunc;

	// Time in seconds to delay before starting to blend into these camera settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blend")
	float BlendInDelay;

	// Time in seconds (after BlendInDelay) to take to blend into these camera settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blend")
	float BlendInTime;

	// Natural length of the spring arm when there are no collisions
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringArm", meta=(UIMin = "0.0", EditCondition="bOverride_TargetArmLength"))
	float TargetArmLength;

	// offset at end of spring arm; use this instead of the relative offset of the attached component to ensure the line trace works as desired
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringArm", meta=(EditCondition="bOverride_SocketOffset"))
	FVector SocketOffset;

	// Offset at start of spring, applied in world space. Use this if you want a world-space offset from the parent component instead of the usual relative-space offset.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringArm", meta=(EditCondition="bOverride_TargetOffset"))
	FVector TargetOffset;

	/**
	 * If this component is placed on a pawn, should it use the view/control rotation of the pawn where possible?
	 * @see APawn::GetViewRotation()
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringArm", meta=(EditCondition="bOverride_UsePawnControlRotation"))
	uint32 bUsePawnControlRotation : 1;

	// Should we inherit pitch from parent component. Does nothing if using Absolute Rotation.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringArm", meta=(EditCondition="bOverride_InheritPitch"))
	uint32 bInheritPitch : 1;

	// Should we inherit yaw from parent component. Does nothing if using Absolute Rotation.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringArm", meta=(EditCondition="bOverride_InheritYaw"))
	uint32 bInheritYaw : 1;

	// Should we inherit roll from parent component. Does nothing if using Absolute Rotation.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringArm", meta=(EditCondition="bOverride_InheritRoll"))
	uint32 bInheritRoll : 1;

	/**
	 * If true, camera lags behind target position to smooth its movement.
	 * @see CameraLagSpeed
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringArm", meta=(EditCondition="bOverride_EnableCameraLag"))
	uint32 bEnableCameraLag : 1;

	/**
	 * If true, camera lags behind target rotation to smooth its movement.
	 * @see CameraRotationLagSpeed
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringArm", meta=(EditCondition="bOverride_EnableCameraRotationLag"))
	uint32 bEnableCameraRotationLag : 1;

	/**
	 * If bUseCameraLagSubstepping is true, sub-step camera damping so that it handles fluctuating frame rates well (though this comes at a cost).
	 * @see CameraLagMaxTimeStep
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringArm", meta=(EditCondition="bOverride_UseCameraLagSubstepping"))
	uint32 bUseCameraLagSubstepping : 1;

	// If bEnableCameraLag is true, controls how quickly camera reaches target position. Low values are slower (more lag), high values are faster (less lag), while zero is instant (no lag).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringArm", meta=(EditCondition="bOverride_CameraLagSpeed"))
	float CameraLagSpeed;

	// If bEnableCameraRotationLag is true, controls how quickly camera reaches target position. Low values are slower (more lag), high values are faster (less lag), while zero is instant (no lag).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringArm", meta=(EditCondition="bOverride_CameraRotationLagSpeed"))
	float CameraRotationLagSpeed;

	// Max time step used when sub-stepping camera lag.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringArm", meta=(EditCondition="bOverride_CameraLagMaxTimeStep"))
	float CameraLagMaxTimeStep;

	// Max distance the camera target may lag behind the current location. If set to zero, no max distance is enforced.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringArm", meta=(EditCondition="bOverride_CameraLagMaxDistance"))
	float CameraLagMaxDistance;

	// The horizontal field of view (in degrees) in perspective mode (ignored in Orthographic mode)
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Camera", meta=(UIMin = "5.0", UIMax = "170", ClampMin = "0.001", ClampMax = "360.0", EditCondition="bOverride_FieldOfView"))
	float FieldOfView;

	// The desired width (in world units) of the orthographic view (ignored in Perspective mode)
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Camera", meta=(EditCondition="bOverride_OrthoWidth"))
	float OrthoWidth;

	// The near plane distance of the orthographic view (in world units)
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Camera", meta=(EditCondition="bOverride_OrthoNearClipPlane"))
	float OrthoNearClipPlane;

	// The far plane distance of the orthographic view (in world units)
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Camera", meta=(EditCondition="bOverride_OrthoFarClipPlane"))
	float OrthoFarClipPlane;

	// Aspect Ratio (Width/Height)
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Camera", meta=(ClampMin = "0.1", ClampMax = "100.0", EditCondition="bOverride_AspectRatio"))
	float AspectRatio;

	// If true, account for the field of view angle when computing which level of detail to use for meshes.
	UPROPERTY(Interp, EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Camera", meta=(EditCondition="bOverride_UseFieldOfViewForLOD"))
	uint32 bUseFieldOfViewForLOD : 1;
};

/**
 * Dynamic camera supports basic movement based offsets and ellipsoidal rotation for a more cinematic presentation
 */
UCLASS(ClassGroup="ILLCamera", BlueprintType, Blueprintable, HideCategories=(LOD, Rendering), meta=(BlueprintSpawnableComponent, ShortTooltip="Dynamic camera supports basic movement based offsets and ellipsoidal rotation for a more cinematic presentation"))
class ILLGAMEFRAMEWORK_API UILLDynamicCameraComponent
: public USceneComponent
{
	GENERATED_BODY()

public:
	UILLDynamicCameraComponent(const FObjectInitializer& ObjectInitializer);

	// Begin UActorComponent interface
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual void Activate(bool bReset = false) override;
	virtual void Deactivate() override;
	// End UActorComponent interface

	// SpringArm component name
	static FName SpringArmComponentName;

	// Spring arm
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USpringArmComponent* SpringArm;

	// Camera component name
	static FName CameraComponentName;

	// Camera
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UCameraComponent* Camera;

	/** 
	 * Will adjust the camera boom length based on the pitch of the controller
	 * pitch value will be normalized to [-90,90] and input as time, a spring arm length scalar will be the output value
	 * Valid keys are: T:-90,V:1.25 T:0,V:1.0 T:90,V:0.33
	 * 
	 * If no curve is provided, spring arm length will not be adjusted
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MovementBasedSettings")
	UCurveFloat* SpringArmAdjustPitchCurve;

	/**
	 * Will adjust the camera boom length based on the actors speed (verticle can be disabled with bVerticleSpeedAffectsSpringArmLength)
	 * speed in cm/s will be the input as time, a spring arm length scalar will be the output value
	 * Valid keys are: T:0,V:1.0 T:350,V:1.15 T:600,V:1.5
	 * 
	 * If no curve is provided, spring arm length will not be adjusted
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementBasedSettings")
	UCurveFloat* SpringArmAdjustSpeedCurve;

	// If true the speed input will only use XY, if false Z will also be used
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementBasedSettings")
	bool bUse2DVelocity;

	/** Spring arm settings for moving (overrides default SpringArm values) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MovementBasedSettings")
	TArray<FCameraBehaviorSettings> MovementBasedSettings;

	void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos);

private:
	// Default length is pull from SpringArm at game start
	float DefaultSpringArmLength;

	// Current index we're using from MovementBasedSettings
	int32 CurrentOffsetIndex;

	// Index we're lerping from in MovementBasedSettings
	FCameraBehaviorSettings PreviousSettings;

	// Timestamp when we selected the CurrentOffsetIndex
	float CurrentOffsetStartTimestamp;
};
