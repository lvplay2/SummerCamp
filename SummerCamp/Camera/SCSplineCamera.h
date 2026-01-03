// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Camera/CameraComponent.h"
#include "SCCameraSplineComponent.h"
#include "SCSplineCamera.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCameraSplineAnimDelegate);

/**
 * @Enum SplineCaleraLoopMode
 */
UENUM(BlueprintType)
enum class ESplineCameraLoopMode : uint8
{
	SCL_None		UMETA(DisplayName="None"),
	SCL_Loop		UMETA(DisplayName="Loop"),
	SCL_PingPong	UMETA(DisplayName="PingPong")
};

/**
 * 
 */
UCLASS(ClassGroup=Utility, ShowCategories = (Mobility), HideCategories = (Physics, Lighting, Rendering, Mobile), meta=(BlueprintSpawnableComponent))
class SUMMERCAMP_API USCSplineCamera 
	: public UCameraComponent
{
	GENERATED_BODY()
	
public:

	USCSplineCamera(const FObjectInitializer& ObjectInitializer);

	// Begin UCameraComponent override
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction) override;
	virtual void PostInitProperties() override;
	virtual void BeginPlay();
	// End UCameraComponent override

	/**
	 * Activate the animation for this spline camera along it's parent spline
	 * @param FocalComponent - An optional scene component to pass for focal depth testing. helps with distance calculations for updating FOV and collision parsing 
	 */
	UFUNCTION(BlueprintCallable, Category = "SplineCamera")
	void ActivateCamera(USceneComponent* FocalComponent = nullptr, class ASCCharacter* ReturnToCharacter = nullptr);

	/** Doesn't return the camera to the player, only stops the current animation */
	UFUNCTION(BlueprintCallable, Category = "SplineCamera")
	void DeactivateCamera();

	/**
	 * Test if this camera animation is valid or blocked by geo with a single ignored actor
	 * Ignores self and owning actor by default
	 * @param TraceIgnoreActor - The actor to ignore while performing the traces
	 */
	UFUNCTION(BlueprintCallable, Category = "SplineCamera")
	bool IsCameraValid(AActor* TraceIgnoreActor) const;

	/** 
	 * Test if this camera animation is valid or blocked by geo with an ignored actor list
	 * Ignores self and owning actor by default
	 * @param TraceIgnoreActors - The actor list to ignore while performing the traces
	 */
	UFUNCTION(BlueprintCallable, Category = "SplineCamera")
	bool IsCameraValidArray(TArray<AActor*> TraceIgnoreActors) const;

	/** Retrieve the camera rotation for the first frame of the spline animation */
	UFUNCTION(BlueprintCallable, Category = "SplineCamera")
	FRotator GetFirstFrameRotation();

	/** 
	 * Blueprint Callback when the camera has reached the end of the spline.
	 * This function will only be called if the loop Mode is set to "None"
	 */
	UPROPERTY(BlueprintAssignable, Category = "SplineCamera")
	FCameraSplineAnimDelegate OnCameraAnimationEnded;

protected:

	/** Should adjusting the preview time move the viewport camera position */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Preview")
	bool ViewportPreview;

	/** Time along spline to preview (between 0 and the total duration of the spline) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Preview")
	float PreviewTime;

	/** Trace fidelity when validating the animation. Lower values = higher fidelity at the cost of more traces. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SplineCamera", meta = (ClampMin = "0.01", ClampMax = "1.0", UIMin = "0.01", UIMax = "1.0"))
	float ValidityTraceFidelity;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SplineCamera", meta = (ClampMin = "1", UIMin = "1"))
	float CameraMaxTraceDistance;

	/** Camera loop mode. If looping is enabled the camera will have to be manually disabled */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SplineCamera")
	ESplineCameraLoopMode LoopMode;

	USceneComponent* ActiveFocalComponent;
	
	/** The spline this camera is parented to */
	UPROPERTY()
	class USCCameraSplineComponent* CameraSpline;

	/** Spline duration for preview purposes, Used in editor only */
	UPROPERTY(EditDefaultsOnly, Category = "Preview")
	float SplineDuration;

	/** Initialize our CameraSpline component so we know what spline we're attached to */
	bool InitCameraSpline();

	/** Stored animation time. */
	UPROPERTY()
	float ActiveTime;

	/** When loop mode is pingpong, are we animating forward or backward? */
	UPROPERTY()
	bool bPingPongForward;

	/** Begin blending camera back this amount of time before the end of the animation. */
	UPROPERTY(EditDefaultsOnly, Category = "SplineCamera")
	float BlendOutPreTime;

	UPROPERTY(EditAnywhere, Category = "SplineCamera")
	bool bReturnCameraOnEnd;

	UPROPERTY()
	class ASCCharacter* ReturnCameraTo;

	UPROPERTY(EditDefaultsOnly, Category = "SplineCamera")
	TEnumAsByte<EViewTargetBlendFunction> BlendBackType;

	UPROPERTY(EditAnywhere, Category = "SplineCamera")
	FVector HandicamShakeFrequency;

	UPROPERTY(EditAnywhere, Category = "SplineCamera")
	float HandicamShakeFrequenceRandomizer;

	UPROPERTY(EditAnywhere, Category = "SplineCamera")
	bool bAlwaysApplyHandicam;

	UPROPERTY(EditAnywhere, Category = "SplineCamera")
	FVector DefaultHandicamStrength;

	FVector ShakeTime;

	FSCCameraSplinePoint CachedDefaultTransform;

};
