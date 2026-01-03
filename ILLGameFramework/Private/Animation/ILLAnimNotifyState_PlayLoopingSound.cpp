// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLAnimNotifyState_PlayLoopingSound.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

#if WITH_EDITOR
#include "MessageLog.h"
#include "UObjectToken.h"
#endif

UILLAnimNotifyState_PlayLoopingSound::UILLAnimNotifyState_PlayLoopingSound(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, VolumeMultiplier(1.f)
, PitchMultiplier(1.f)
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(200, 128, 255, 255);
#endif
}

void UILLAnimNotifyState_PlayLoopingSound::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	if (MeshComp->GetWorld() == nullptr)
	{
		return;
	}

	if (Sound)
	{
#if WITH_EDITOR
		if (Sound->IsLooping() == false)
		{
			FMessageLog("PIE").Error()
				->AddToken(FUObjectToken::Create(Sound->GetClass()))
				->AddToken(FTextToken::Create(FText::FromString(TEXT(" used in "))))
				->AddToken(FUObjectToken::Create(Animation->GetClass()))
				->AddToken(FTextToken::Create(FText::FromString(TEXT(" is not set to looping. Consider using the standard PlaySound notify instead. "))));
		}
#endif

		UGameplayStatics::SpawnSoundAttached(Sound, MeshComp, SocketName, FVector(ForceInit), EAttachLocation::KeepRelativeOffset, false, VolumeMultiplier, PitchMultiplier);
	}

	Received_NotifyBegin(MeshComp, Animation, TotalDuration);
}

void UILLAnimNotifyState_PlayLoopingSound::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	TArray<USceneComponent*> Children;
	MeshComp->GetChildrenComponents(false, Children);

	// Stop playing
	for (USceneComponent* Component : Children)
	{
		if (UAudioComponent* AudioComponent = Cast<UAudioComponent>(Component))
		{
			bool bSoundMatch = AudioComponent->Sound == Sound;
			bool bSocketMatch = AudioComponent->GetAttachSocketName() == SocketName;

#if WITH_EDITORONLY_DATA
			// In editor someone might have changed our parameters while we're ticking; so check 
			// previous known parameters too.
			bSoundMatch |= PreviousBaseSounds.Contains(AudioComponent->Sound);
			bSocketMatch |= PreviousSocketNames.Contains(AudioComponent->GetAttachSocketName());
#endif

			if (bSoundMatch && bSocketMatch && AudioComponent->IsPlaying())
			{
				AudioComponent->FadeOut(FadeOutTime, 0.f);
				AudioComponent->bAutoDestroy = true;

#if WITH_EDITORONLY_DATA
				// No longer need to track previous values as we've found our component
				// and removed it.
				PreviousBaseSounds.Empty();
				PreviousSocketNames.Empty();
#endif

				break;
			}
		}
	}

	Received_NotifyEnd(MeshComp, Animation);
}

#if WITH_EDITOR
void UILLAnimNotifyState_PlayLoopingSound::PreEditChange(UProperty* PropertyAboutToChange)
{
	if (Sound == nullptr)
		return;

	if (PropertyAboutToChange)
	{
		if (PropertyAboutToChange->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UILLAnimNotifyState_PlayLoopingSound, Sound) && Sound != nullptr)
		{
			PreviousBaseSounds.Add(Sound);
		}

		if (PropertyAboutToChange->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UILLAnimNotifyState_PlayLoopingSound, SocketName))
		{
			PreviousSocketNames.Add(SocketName);
		}
	}
}
#endif

FString UILLAnimNotifyState_PlayLoopingSound::GetNotifyName_Implementation() const
{
	if (Sound)
	{
		return Sound->GetName();
	}
	else
	{
		return Super::GetNotifyName_Implementation();
	}
}
