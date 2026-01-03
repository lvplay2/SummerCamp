// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCAnimNotify_PlayCounselorVOBySoundCue.h"

#include "SCCounselorCharacter.h"
#include "SCVoiceOverComponent_Counselor.h"
#include "SCVoiceOverComponent.h"
#include "SCPlayerState.h"

USCAnimNotify_PlayCounselorVOBySoundCue::USCAnimNotify_PlayCounselorVOBySoundCue()
: Super()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(63, 191, 63, 255);
#endif // WITH_EDITORONLY_DATA
}

void USCAnimNotify_PlayCounselorVOBySoundCue::Notify(class USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation)
{
	if (ASCCharacter* Owner = Cast<ASCCharacter>(MeshComp->GetOwner()))
	{
		if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Owner))
		{
			if (USCVoiceOverComponent* VOComponent = Counselor->VoiceOverComponent)
			{
				VOComponent->PlayVoiceOverBySoundCue(VoiceOverSoundCue);
			}
		}
	}
}

FString USCAnimNotify_PlayCounselorVOBySoundCue::GetNotifyName_Implementation() const
{
	static const FString NotifyName(TEXT("Voice Line"));

	return NotifyName;
}
