// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ILLAnimNotifyState_PlayLoopingSound.generated.h"

class USoundBase;
class UAudioComponent;

/**
 * @class UILLAnimNotifyState_PlayLoopingSound
 */
UCLASS(meta=(DisplayName="ILL Play Looping Sound"))
class ILLGAMEFRAMEWORK_API UILLAnimNotifyState_PlayLoopingSound
: public UAnimNotifyState
{
	GENERATED_BODY()

public:

	UILLAnimNotifyState_PlayLoopingSound(const FObjectInitializer& ObjectInitializer);

	// Begin UAnimNotifyState interface
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	virtual FString GetNotifyName_Implementation() const override;
#if WITH_EDITOR
	virtual void PreEditChange(UProperty* PropertyAboutToChange) override;
#endif
	// End UAnimNotifyState interface

	// Sound to Play
	UPROPERTY(EditAnywhere, Category="AnimNotify")
	USoundBase* Sound;

	// Volume Multiplier
	UPROPERTY(EditAnywhere, Category="AnimNotify")
	float VolumeMultiplier;

	// Pitch Multiplier
	UPROPERTY(EditAnywhere, Category="AnimNotify")
	float PitchMultiplier;

	// Time (in seconds) to fade out the sound when exiting (0 will stop instantly)
	UPROPERTY(EditAnywhere, Category="AnimNotify")
	float FadeOutTime;

	// Socket or bone name to attach sound to
	UPROPERTY(EditAnywhere, Category="AnimNotify")
	FName SocketName;

#if WITH_EDITORONLY_DATA
	// The following arrays are used to handle property changes during a state. Because we can't
	// store any stateful data here we can't know which audio component is ours. The best metric we have
	// is an audio component on our Mesh Component with the same base sound and socket name we have defined.
	// Because these can change at any time we need to track previous versions when we are in an
	// editor build. Refactor when stateful data is possible, tracking our component instead.
	UPROPERTY(transient)
	TArray<USoundBase*> PreviousBaseSounds;

	UPROPERTY(transient)
	TArray<FName> PreviousSocketNames;
	
#endif
};
