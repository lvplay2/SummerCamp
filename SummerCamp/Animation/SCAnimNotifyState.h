// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "SCAnimNotifyState.generated.h"

/**
 * 
 */
UCLASS()
class SUMMERCAMP_API USCAnimNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

	// Begin UAnimNotifyState interface
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	virtual FString GetNotifyName_Implementation() const override;
	// End UAnimNotifyState interface
	
	/** Distinct notify name to report when calling to AnimDriverComponents */
	UPROPERTY(EditAnywhere, Category = "AnimNotify")
	FName NotifyName;
	
	/** Store Our found anim driver reference for each world so each world updates properly */
	UPROPERTY()
	TArray<class USCAnimDriverComponent*> AnimDrivers;
};
