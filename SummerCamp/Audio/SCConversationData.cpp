// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"

#include "SCConversationData.h"
#include "SCGameInstance.h"
#include "SCGameState.h"
#include "SCVoiceOverComponent.h"

bool FSCConversationVOData::operator==(const FSCConversationVOData& other) const
{
	return (other.CounselorCharacter == CounselorCharacter
		&& other.VoiceOverCue == VoiceOverCue
		&& other.PostDelay == PostDelay);
}

bool FSCConversationData::operator==(const FSCConversationData& other) const
{
	return (other.VOData == VOData);
}

USCConversation::USCConversation(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void USCConversation::PlayNextLine()
{
	UWorld* World = GetWorld();
	if (!World)
		return;

	if (bConversationPaused)
		return;

	if (ConversationData.VOData.IsValidIndex(NextVoiceLineIndex))
	{
		FSCConversationVOData& CurrentVOData = ConversationData.VOData[NextVoiceLineIndex];
		// if for some reason the index is null play the next line instead.
		if (CurrentVOData.VoiceOverCue.IsNull())
		{
			++NextVoiceLineIndex;
			PlayNextLine();
			return;
		}

		USoundCue* VoiceOverSoundCue = CurrentVOData.VoiceOverCue.Get();

		// The audio isn't loaded yet.
		if (!VoiceOverSoundCue)
		{
			USCGameInstance* GameInstance = Cast<USCGameInstance>(World->GetGameInstance());
			GameInstance->StreamableManager.RequestAsyncLoad(CurrentVOData.VoiceOverCue.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &ThisClass::PlayNextLine));
			return;
		}

		// Everything is loaded and ready to go, play that shit.
		ASCCounselorCharacter* CounselorCharacter = CurrentVOData.CounselorCharacter;
		if (CounselorCharacter && CounselorCharacter->VoiceOverComponent)
		{
			CounselorCharacter->VoiceOverComponent->PlayVoiceOverBySoundCue(VoiceOverSoundCue);

#if WITH_EDITOR // GetDuration() doesn't play nice in editor, We'll grab it ourselves.
			const float TimeToDelay = FMath::Max(VoiceOverSoundCue->Duration + CurrentVOData.PostDelay, 0.01f); // Make sure the conversation doesn't hang.
#else
			const float TimeToDelay = FMath::Max(VoiceOverSoundCue->GetDuration() + CurrentVOData.PostDelay, 0.01f);
#endif

			++NextVoiceLineIndex;
			World->GetTimerManager().SetTimer(TimerHandle_VoiceOverPostDelay, FTimerDelegate::CreateUObject(this, &ThisClass::PlayNextLine), TimeToDelay, false);
		}
		else
		{
			// No counselor or VO component??? Skip please.
			++NextVoiceLineIndex;
			PlayNextLine();
		}
	}
	else if (ConversationData.VOData.Num() > 0)
	{
		ConversationComplete.ExecuteIfBound(this);
		OnConversationDone.Broadcast();
		ConditionalBeginDestroy();
	}
}

void USCConversation::AbortConversation()
{
	if (!GetWorld())
		return;

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_VoiceOverPostDelay);

	ASCCounselorCharacter* CounselorCharacter = ConversationData.VOData[FMath::Max(--NextVoiceLineIndex, 0)].CounselorCharacter;
	if (CounselorCharacter && CounselorCharacter->VoiceOverComponent)
	{
		CounselorCharacter->VoiceOverComponent->StopVoiceOver();
	}

	ConversationComplete.ExecuteIfBound(this);
	OnConversationAborted.Broadcast();
	ConditionalBeginDestroy();
}

void USCConversation::PauseConversation()
{
	if (!GetWorld())
		return;

	ASCCounselorCharacter* CounselorCharacter = ConversationData.VOData[FMath::Max(--NextVoiceLineIndex, 0)].CounselorCharacter;
	if (CounselorCharacter && CounselorCharacter->VoiceOverComponent)
	{
		CounselorCharacter->VoiceOverComponent->StopVoiceOver();
	}

	bConversationPaused = true;
}

void USCConversation::ResumeConversation()
{
	if (!GetWorld())
		return;

	bConversationPaused = false;
	NextVoiceLineIndex = FMath::Max(--NextVoiceLineIndex, 0);
	PlayNextLine();
}

void USCConversation::InitializeConversation(FSCConversationData InConversationData, USCConversationManager* Manager)
{
	ConversationData = InConversationData;
	for (const FSCConversationVOData& VOData : ConversationData.VOData)
	{
		if (!ConversationData.Participants.Contains(VOData.CounselorCharacter))
			ConversationData.Participants.AddUnique(VOData.CounselorCharacter);
	}

	PlayNextLine();

	ConversationComplete.BindDynamic(Manager, &USCConversationManager::ConversationComplete);
}
