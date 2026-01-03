// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"

#include "SCConversationManager.h"
#include "SCVoiceOverComponent.h"

USCConversationManager::USCConversationManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

USCConversation* USCConversationManager::PlayConversation(FSCConversationData& InConversation)
{
	// Does the new conversation need characters from an existing conversation?
	// Does the new conversation have a higher priority than an existing conversation?
	for (int ConversationIndex = 0; ConversationIndex < ActiveConversations.Num(); ++ConversationIndex)
	{
		USCConversation* ActiveConvo = ActiveConversations[ConversationIndex];

		for (const FSCConversationVOData& VOData : InConversation.VOData)
		{
			// If one of the counselors is already in offline mode don't play the conversation.
			if (VOData.CounselorCharacter != nullptr && VOData.CounselorCharacter->IsUsingOfflineLogic())
				return nullptr;

			if (ActiveConvo->GetConversingCharacters().Contains(VOData.CounselorCharacter))
			{
				// Character is already in a convo, if the priority of the new one isn't greater than the active one then we're not playing it.
				if (InConversation.ConversationPriority <= ActiveConvo->GetConversationPriority())
				{
					return nullptr;
				}
				else
				{
					ActiveConvo->AbortConversation();
				}
			}
		}
	}

	//Lets play it. Create a new conversation object.
	USCConversation* NewConvo = NewObject<USCConversation>(this);
	NewConvo->InitializeConversation(InConversation, this);

	ActiveConversations.Add(NewConvo);

	return NewConvo;
}

bool USCConversationManager::PlayVoiceLine(ASCCounselorCharacter* Counselor, USoundCue* InAudioClip, bool bAbortActiveConversation)
{
	// No counselor or no audio means we have a problem.
	if (!Counselor || !InAudioClip)
		return false;

	// Don't play a voice line if we've been spooked into offline mode.
	if (Counselor->IsUsingOfflineLogic())
		return false;

	for (int ConversationIndex = 0; ConversationIndex < ActiveConversations.Num(); ++ConversationIndex)
	{
		USCConversation* ActiveConvo = ActiveConversations[ConversationIndex];

		if (ActiveConvo->GetConversingCharacters().Contains(Counselor))
		{
			// Character is already in a convo, if the conversation is not to be aborted with this line then skip playing it.
			if (!bAbortActiveConversation)
				return false;

			if (Counselor->VoiceOverComponent)
				ActiveConvo->AbortConversation();
		}
	}

	if (Counselor->VoiceOverComponent)
	{
		Counselor->VoiceOverComponent->PlayVoiceOverBySoundCue(InAudioClip);
		return true;
	}

	return false;
}

void USCConversationManager::PauseConversationByCounselor(ASCCounselorCharacter* CounselorCharacter)
{
	for (USCConversation* ActiveConvo : ActiveConversations)
	{
		if (ActiveConvo->GetConversingCharacters().Contains(CounselorCharacter))
		{
			// Pause it and kill the VO???
			ActiveConvo->PauseConversation();
		}
	}
}

void USCConversationManager::ResumeConversationByCounselor(ASCCounselorCharacter* CounselorCharacter)
{
	for (USCConversation* ActiveConvo : ActiveConversations)
	{
		if (ActiveConvo->GetIsPaused() && ActiveConvo->GetConversingCharacters().Contains(CounselorCharacter))
		{
			ActiveConvo->ResumeConversation();
		}
	}
}

void USCConversationManager::AbortConversationByCounselor(ASCCounselorCharacter* CounselorCharacter)
{
	for (int ConversationIndex = 0; ConversationIndex < ActiveConversations.Num(); ++ConversationIndex)
	{
		USCConversation* ActiveConvo = ActiveConversations[ConversationIndex];

		if (ActiveConvo->GetConversingCharacters().Contains(CounselorCharacter))
		{
			ActiveConvo->AbortConversation();
			--ConversationIndex;
		}
	}
}

void USCConversationManager::ConversationComplete(USCConversation* InConversation)
{
	ActiveConversations.Remove(InConversation);
}
