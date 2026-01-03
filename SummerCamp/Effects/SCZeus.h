// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCZeus.generated.h"

/**
 * @struct FSCLightingTiming
 */
USTRUCT()
struct FSCLightingTiming
{
	GENERATED_BODY()

public:
	// Game time to play this strike at based on game state time elapsed
	UPROPERTY()
	int32 TimeSeconds;

	// Curve to use for this strike, based on the LightningCurves list
	UPROPERTY()
	uint8 CurveIndex;
};

/**
 * @class ASCZeus
 * @brief Zeus is our thunder god actor. He controls the directional light to simulate lightning and thunder. Should only be placed on our rain map
 */
UCLASS()
class SUMMERCAMP_API ASCZeus
: public AActor
{
	GENERATED_BODY()

public:
	ASCZeus(const FObjectInitializer& ObjectInitializer);

	// Begin UObject interface
	virtual void BeginPlay() override;
	// End UObject interface

	// Begin AActor interface
	virtual void Tick(float DeltaSeconds) override;
	// End AActor interface

	// Our directional light, replaces a normal directional light
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UDirectionalLightComponent* DirectionalLight;

	// Thunder sound, should be set to external to handle indoor sounds correctly
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UAudioComponent* ThunderAudio;

	// Rain sound, should be set to external to handle indoor sounds correctly
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UAudioComponent* RainAudio;

protected:
	// List of curves to use to scale the directional light
	UPROPERTY(EditAnywhere, Category = "Lightning")
	TArray<UCurveFloat*> LightningCurves;

	// Scale value from the lightning curves to surpass in order to play LightningSound
	UPROPERTY(EditAnywhere, Category = "Lightning")
	float LightningSoundThreshold;

	// How long (in seconds) to wait before our first lightning strike
	UPROPERTY(EditAnywhere, Category = "Lightning")
	int32 InitialDelay;

	// Minimum time (in seconds) to wait between lightning strikes
	UPROPERTY(EditAnywhere, Category = "Lightning")
	int32 MinTimeBetweenLightning;

	// Maximum time (in seconds) to wait between lightning strikes
	UPROPERTY(EditAnywhere, Category = "Lightning")
	int32 MaxTimeBetweenLightning;

	/** Adds lightining strikes for x seconds after the last current strike */
	void GenerateLightning(const int32 StartTime, const int32 SecondsToFill);

	/** Utility function to get the MPC for rain from the World Settings */
	UMaterialParameterCollection* GetRainMaterialParameterCollection() const;

private:
	// List of strike times and curves to play throughout the match, replicated once
	UPROPERTY(Transient, Replicated)
	TArray<FSCLightingTiming> LightningTimings;

	// World time we started the current curve at
	float LightingCurveStartTime;

	// Last curve value from our active curve (defaults to 1.f)
	float PreviousIntensityScale;

	// Last curve index we used
	int32 LastUsedTiming;

	// Last time we got during a tick (prevents lots of churning)
	int32 PreviousGameTime;

	// Index of the curve we're using right now, -1 if none
	int32 ActiveCurve;

	// Intensity first set on our directional light, grabbed at BeginPlay
	float DefaultIntensity;

	// Used to make sure we only play one sound effect per lightning curve
	bool bThunderPlayed;
};
