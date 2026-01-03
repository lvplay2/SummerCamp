// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "ILLCameraShakeAnimNotify.generated.h"

/**
 * @class UILLCameraShakeAnimNotify
 */
UCLASS(meta=(DisplayName="ILL Camera Shake"))
class ILLGAMEFRAMEWORK_API UILLCameraShakeAnimNotify
: public UAnimNotify
{
	GENERATED_UCLASS_BODY()

public:

	// Begin UAnimNotify interface
	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	// End UAnimNotify interface

	// Play this shake for all network clients?
	UPROPERTY(EditAnywhere, Category = "AnimNotify")
	bool bPlayForAllClients;

	// Camera shake we want to play when this notify hits
	UPROPERTY(EditAnywhere, Category="AnimNotify")
	TSubclassOf<UCameraShake> CameraShake;

	// Scale to play the camer shake at
	UPROPERTY(EditAnywhere, Category="AnimNotify")
	float Scale;

	// Range (in cm) to play shakes on nearby players (0 for local only)
	UPROPERTY(EditAnywhere, Category="AnimNotify")
	float ShakeRange;

	// Distance (in cm) to begin a linear falloff for scale on nearby players
	UPROPERTY(EditAnywhere, Category="AnimNotify")
	float ShakeFalloffStart;

	// Starting scale for nearby players, will then perform linear falloff from ShakeFalloffStart to ShakeRange
	UPROPERTY(EditAnywhere, Category="AnimNotify")
	float NearbyScale;
};
