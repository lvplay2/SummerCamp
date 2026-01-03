// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCAnimNotify_StopCounselorVO.h"

#include "SCCounselorCharacter.h"
#include "SCVoiceOverComponent.h"
#include "SCPlayerState.h"

USCAnimNotify_StopCounselorVO::USCAnimNotify_StopCounselorVO()
: Super()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(63, 191, 63, 255);
#endif // WITH_EDITORONLY_DATA
}

void USCAnimNotify_StopCounselorVO::Notify(class USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation)
{
	if (ASCCharacter* Owner = Cast<ASCCharacter>(MeshComp->GetOwner()))
	{
		if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Owner))
		{
			if (USCVoiceOverComponent* VOComponent = Counselor->VoiceOverComponent)
			{
				VOComponent->StopVoiceOver();
			}
		}
	}
}

FString USCAnimNotify_StopCounselorVO::GetNotifyName_Implementation() const
{
	static const FString NotifyName(TEXT("Stop Voice Line"));

	return NotifyName;
}
