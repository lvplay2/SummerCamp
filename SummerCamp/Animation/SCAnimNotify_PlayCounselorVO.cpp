// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCAnimNotify_PlayCounselorVO.h"

#include "SCCounselorCharacter.h"
#include "SCVoiceOverComponent_Counselor.h"
#include "SCVoiceOverComponent.h"
#include "SCPlayerState.h"

USCAnimNotify_PlayCounselorVO::USCAnimNotify_PlayCounselorVO()
: Super()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(63, 191, 63, 255);
#endif // WITH_EDITORONLY_DATA
}

void USCAnimNotify_PlayCounselorVO::Notify(class USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation)
{
	if (ASCCharacter* Owner = Cast<ASCCharacter>(MeshComp->GetOwner()))
	{
		if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Owner))
		{
			if (USCVoiceOverComponent* VOComponent = Counselor->VoiceOverComponent)
			{
				if (CinematicKill)
				{
					Cast<USCVoiceOverComponent_Counselor>(VOComponent)->PlayCinematicVoiceOver(VoiceOverEvent);
				}
				else
				{
					VOComponent->PlayVoiceOver(VoiceOverEvent);
				}
			}
		}
	}
}

FString USCAnimNotify_PlayCounselorVO::GetNotifyName_Implementation() const
{
	static const FString NotifyName(TEXT("Voice Line"));

	return NotifyName;
}
