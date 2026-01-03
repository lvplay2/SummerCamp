// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "PostProcessNotifyState.generated.h"

/**
 * @class UPostProcessNotifyState
 */
UCLASS()
class SUMMERCAMP_API UPostProcessNotifyState
: public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UPostProcessNotifyState(const FObjectInitializer& ObjectInitializer);

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

	/** PostProcess blend in time */
	UPROPERTY(EditAnywhere)
	float BlendInTime;

	/** PostProcess blend out time */
	UPROPERTY(EditAnywhere)
	float BlendOutTime;

	UPROPERTY(EditAnywhere)
	FPostProcessSettings NotifyPostProcessSettings;

	UPROPERTY(EditAnywhere)
	FString NotifyName;
};
