// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCCounselorCharacter.h"
#include "SCConversationData.h"
#include "SCConversationManager.generated.h"

/**
* @class USCConversationManager
*/
UCLASS()
class SUMMERCAMP_API USCConversationManager
: public UObject
{
	GENERATED_UCLASS_BODY()

public:

	// Begin UObject Interface
	virtual UWorld* GetWorld() const override { return GetOuter() ? GetOuter()->GetWorld() : nullptr; }
	// End UObject Interface

	/** Attempt to play a new conversation. Will fail if counselors needed are already in a higher priority conversation. */
	UFUNCTION()
	USCConversation* PlayConversation(FSCConversationData& InConversation);

	/** Attempt to play a voice line on a character that could possibly be in a conversation. */
	UFUNCTION()
	bool PlayVoiceLine(ASCCounselorCharacter* Counselor, USoundCue* InAudioClip, bool AbortActiveConversation);

	/** Called from the Conversation Object when the conversastion is aborted or completed so that it is removed from the active list and allowed to be garbage colleccted. */
	UFUNCTION()
	void ConversationComplete(USCConversation* InConversation);

	/** Pause any conversation that the given counselor is active in. */
	UFUNCTION()
	void PauseConversationByCounselor(ASCCounselorCharacter* CounselorCharacter);

	/** Resume any paused conversations the given counselor is active in. */
	UFUNCTION()
	void ResumeConversationByCounselor(ASCCounselorCharacter* CounselorCharacter);

	/** Abort any conversation that the given counselor is participating in. */
	UFUNCTION()
	void AbortConversationByCounselor(ASCCounselorCharacter* CounselorCharacter);

protected:

	// A list of all conversations that are currently playing.
	UPROPERTY()
	TArray<USCConversation*> ActiveConversations;

};
