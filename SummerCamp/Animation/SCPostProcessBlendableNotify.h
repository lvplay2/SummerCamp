// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "SCPostProcessBlendableNotify.generated.h"

USTRUCT()
struct FBlendableTextureOverride
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FName ParameterName;

	UPROPERTY(EditAnywhere)
	UTexture* Texture;
};

USTRUCT()
struct FBlendableIntesityHooks
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FName ParameterName;

	UPROPERTY(EditAnywhere)
	bool UseBlendIn;

	UPROPERTY(EditAnywhere)
	bool UseBlendOut;
};

/**
 * 
 */
UCLASS()
class SUMMERCAMP_API USCPostProcessBlendableNotify 
: public UAnimNotifyState
{
	GENERATED_BODY()
	
	USCPostProcessBlendableNotify(const FObjectInitializer& ObjectInitializer);

	// Begin UAnimNotifyState interface
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	virtual void BranchingPointNotifyBegin(FBranchingPointNotifyPayload& BranchingPointPayload);
	virtual void BranchingPointNotifyTick(FBranchingPointNotifyPayload& BranchingPointPayload, float FrameDeltaTime);
	virtual void BranchingPointNotifyEnd(FBranchingPointNotifyPayload& BranchingPointPayload);
	// End UANIMNotifyState interface

	virtual void BeginDestroy() override;

	virtual FString GetNotifyName_Implementation() const override;

	float TimePassed;
	float BlendOutStart;
	float Intensity;

	TWeakObjectPtr<APostProcessVolume> PostProcessVolume;

	UPROPERTY(EditAnywhere)
	bool PlayForAllClients;

	/** Minimum intensity for post process effect */
	UPROPERTY(EditAnywhere)
	float MinIntensity;

	/** Maximum intensity for post process effect */
	UPROPERTY(EditAnywhere)
	float MaxIntensity;

	/** 
	 * The material we want to use
	 * Blendables do not like layering the same material even instanced ones.
	 * All layered blendables need to use completely separate materials.
	 */
	UPROPERTY(EditAnywhere)
	UMaterialInterface* MaterialInstance;

	/** Created dynamic material for this notify */
	TWeakObjectPtr<UMaterialInstanceDynamic> DynamicInstance;

	/** material parameter list */
	UPROPERTY(EditAnywhere)
	TArray<FBlendableTextureOverride> OverrideTextures;

	/** Intensity will ramp from minIntensity to maxIntensity over this time. Intensity hooks can be used to listen for the updated intensity */
	UPROPERTY(EditAnywhere)
	float BlendInTime;

	/** Intensity will ramp from maxIntensity to minIntensity over this time. Intensity hooks can be used to listen for the updated intensity */
	UPROPERTY(EditAnywhere)
	float BlendOutTime;

	/** intensity parameter hooks */
	UPROPERTY(EditAnywhere)
	TArray<FBlendableIntesityHooks> IntensityHooks;

	UPROPERTY(EditAnywhere)
	FPostProcessSettings DesiredSettings;
};
