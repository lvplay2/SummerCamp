// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCAnimNotify_PlayCinematicMusic.h"

#include "SCCounselorCharacter.h"
#include "SCKillerCharacter.h"
#include "SCMusicComponent.h"
#include "SCMusicComponent_Killer.h"
#include "ILLGameBlueprintLibrary.h"

USCAnimNotify_PlayCinematicMusic::USCAnimNotify_PlayCinematicMusic()
: Super()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(63, 191, 63, 255);
#endif // WITH_EDITORONLY_DATA
}

void USCAnimNotify_PlayCinematicMusic::Notify(class USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation)
{
	if (ASCCharacter* Owner = Cast<ASCCharacter>(MeshComp->GetOwner()))
	{
		// check to see if this player is being spectated.
		bool bIsSpectator = false;
		if (UWorld* World = Owner->GetWorld())
		{
			if (ASCPlayerController* PC = Cast<ASCPlayerController>(UILLGameBlueprintLibrary::GetFirstLocalPlayerController(World)))
			{
				if (PC->GetSpectatingPlayer() == Owner)
				{
					bIsSpectator = true;
				}
			}
		}

		if (Owner->IsLocallyControlled() || bIsSpectator)
		{
			if (ASCCharacter* Character = Cast<ASCCharacter>(Owner))
			{
				if (USCMusicComponent* MusicComponent = Character->MusicComponent)
				{
					MusicComponent->PlayCinematicMusic(MusicTrackToPlay);
				}
			}
		}
	}
}

FString USCAnimNotify_PlayCinematicMusic::GetNotifyName_Implementation() const
{
	if (MusicTrackToPlay)
	{
		return MusicTrackToPlay->GetName();
	}
	else
	{
		return Super::GetNotifyName_Implementation();
	}
}
