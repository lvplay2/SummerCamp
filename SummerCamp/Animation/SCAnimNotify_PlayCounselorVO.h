// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Animation/AnimNotifies/AnimNotify_PlaySound.h"
#include "SCTypes.h"
#include "SCAnimNotify_PlayCounselorVO.generated.h"

/**
 * @class USCAnimNotify_PlayCounselorVO
 */
UCLASS(const, hidecategories=Object, collapsecategories, meta=(DisplayName="Play Voice Line"))
class SUMMERCAMP_API USCAnimNotify_PlayCounselorVO
: public UAnimNotify
{
	GENERATED_BODY()

public:
	USCAnimNotify_PlayCounselorVO();
	
	// Begin UAnimNotify interface
	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	// End UAnimNotify interface

	UPROPERTY(EditAnywhere, Category = "AnimNotify")
	FName VoiceOverEvent;

	UPROPERTY(EditAnywhere, Category = "AnimNotify")
	bool CinematicKill;
};
