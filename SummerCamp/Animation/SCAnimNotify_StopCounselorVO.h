// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Animation/AnimNotifies/AnimNotify_PlaySound.h"
#include "SCTypes.h"
#include "SCAnimNotify_StopCounselorVO.generated.h"

/**
 * @class USCAnimNotify_PlayCounselorVO
 */
UCLASS(const, hidecategories=Object, collapsecategories, meta=(DisplayName="Stop Voice Line"))
class SUMMERCAMP_API USCAnimNotify_StopCounselorVO
: public UAnimNotify
{
	GENERATED_BODY()

public:
	USCAnimNotify_StopCounselorVO();
	
	// Begin UAnimNotify interface
	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	// End UAnimNotify interface
};
