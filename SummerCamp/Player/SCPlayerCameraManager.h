// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLPlayerCameraManager.h"
#include "SCPlayerCameraManager.generated.h"

USTRUCT()
struct FSCRainGridCell
{
	GENERATED_BODY()

	// Actual rain particle system
	UPROPERTY()
	UParticleSystemComponent* RainComponent;

	// Offset from the player for this cell
	FVector RelativeOffset;

	// Timestamp for our next ray trace
	float NextTraceTimestamp;

	// If true, we should turn the rain off for this cell (we hit something with our last trace)
	bool bIsOccluded;
};

/**
 * @class ASCPlayerCameraManager
 */
UCLASS()
class SUMMERCAMP_API ASCPlayerCameraManager
: public AILLPlayerCameraManager
{
	GENERATED_BODY()

public:
	ASCPlayerCameraManager(const FObjectInitializer& ObjectInitializer);

	// Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Destroyed() override;
	// End AActor Interface

	// Begin PlayerCameraManager Interface
	virtual void UpdateCamera(float DeltaTime) override;
	// End PlayerCameraManager Interface

	// Offset from the camera to apply to the rain particle
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rain")
	float RainZOffset;

	// If the camera travels further than this in one frame, the particle system will be reset
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rain")
	float RainResetDistance;

	// Time (in seconds) between traces, lower has better reaction time but higher CPU cost. Actual time may be double requested time in order to prevent trace overlaps.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rain")
	float RainRayTraceTickDelay;

	// Distance from a colliding ray trace to the camera along Z to be considered "indoors"
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rain")
	float RainOcclusionHeight;

	// Generates a grid of RainGridSize x RainGridSize for rain particles (even numbers are better)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rain")
	int32 RainGridSize;

	// Size of the area the rain emitter uses when spawning particles (should be square)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rain")
	float RainParticleSize;

	// Particle to use in the rain grid
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rain")
	TAssetPtr<UParticleSystem> RainParticleSystem;

protected:
	// Grid of rain particles surrounding the player camera
	UPROPERTY(Transient)
	TArray<FSCRainGridCell> RainGrid;

	// Used to track if we've built our grid or not, set based on WorldSettings
	bool bIsRaining;

	// Location last frame
	FVector PreviousLocation;

	/** Delegate bound to WorldSettings for when rain is turned on or off */
	UFUNCTION()
	void OnRainUpdatedInWorldSettings(bool bRainEnabled);

	/** Makes particle components and turns the rain on, will async load the particle system if needed */
	void EnableRain();

	/** Turns off all particles and clears out the grid */
	void DisableRain();

	/** Async callback for when the particle system is loaded, applies template to the components and activates the systems */
	UFUNCTION()
	void OnRainParticlesLoaded();

	/** Updates position/visibility of rain, can be disabled with sc.RainPauseUpdating */
	void UpdateRain(float DeltaTime);

	/** Debug output for the rain system, enabled with sc.RainDebug */
	void DebugDraw();
};
