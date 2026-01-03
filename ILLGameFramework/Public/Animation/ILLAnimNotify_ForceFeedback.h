// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "ILLAnimNotify_ForceFeedback.generated.h"

/**
 * @enum EILLForceFeedbackStyle
 */
UENUM(BlueprintType)
enum class EILLForceFeedbackStyle : uint8
{
	OneOff, // Plays once and quits, doesn't affect other active force feedbacks
	Looping, // Plays with no end in sight (please call stop sometime... please)
	Stop // Stops this force feedback (if no tag is set, stops ALL)
};

/**
 * @class UILLAnimNotify_ForceFeedback
 */
UCLASS(meta=(DisplayName="ILL Force Feedback"))
class ILLGAMEFRAMEWORK_API UILLAnimNotify_ForceFeedback
: public UAnimNotify
{
	GENERATED_BODY()

public:
	UILLAnimNotify_ForceFeedback(const FObjectInitializer& ObjectInitializer);

	// Begin UAnimNotify interface
	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	// End UAnimNotify interface

	// Force feedback we want to play when this notify hits
	UPROPERTY(EditAnywhere, Category="AnimNotify")
	UForceFeedbackEffect* ForceFeedback;

	// What do we want to do with this force feedback
	UPROPERTY(EditAnywhere, Category="AnimNotify")
	EILLForceFeedbackStyle PlayStyle;

	// Tag for the force feedback, used to update or stop feedback
	UPROPERTY(EditAnywhere, Category="AnimNotify")
	FName FeedbackTag;

	// Range (in cm) to play force feedback on nearby players (0 for local only)
	UPROPERTY(EditAnywhere, Category="AnimNotify")
	float ForceFeedbackRemotePlayerRange;

private:
	void PlayForceFeedback(APlayerController* PlayerController) const;
};
