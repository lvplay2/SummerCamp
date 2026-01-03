// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCCounselorCharacter.h"
#include "SCConversationData.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FConversationDoneEvent);
DECLARE_DYNAMIC_DELEGATE_OneParam(FConversationComplete, USCConversation*, InConversation);

USTRUCT(BlueprintType)
struct FConversationData
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	ASCCounselorCharacter* CounselorCharacter;

	// TheVoice line to be played
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TAssetPtr<USoundCue> VoiceOverCue;

	// Optional delay after the VO is finished to wait before playing the next line. Can also be used to delay a call the OnConversationDone.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float PostDelay;
};

/**
* @struct FSCConversationVOData
*/
USTRUCT(BlueprintType)
struct FSCConversationVOData
{
	GENERATED_USTRUCT_BODY()

public:
	// The character to play this Voice line from
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	ASCCounselorCharacter* CounselorCharacter;

	// TheVoice line to be played
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TAssetPtr<USoundCue> VoiceOverCue;

	// Optional delay after the VO is finished to wait before playing the next line. Can also be used to delay a call the OnConversationDone.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float PostDelay;

	bool operator==(const FSCConversationVOData& other) const;
};


/**
* @struct FSCConversationData
*/
USTRUCT(BlueprintType)
struct FSCConversationData
{
	GENERATED_USTRUCT_BODY()

public:

	// Priority value for the conversation. Conversations with a higher priority will be played even if another with a lower priority is currently playing.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 ConversationPriority;

	// The characters and their voice lines for this conversation.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FSCConversationVOData> VOData;

	// The list of characters that should be participating in this conversation.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<ASCCounselorCharacter*> Participants;

	bool operator==(const FSCConversationData& other) const;
};


/**
* @class USCConversation
*/
UCLASS()
class SUMMERCAMP_API USCConversation
: public UObject
{
	GENERATED_UCLASS_BODY()

	// Begin UObject Interface
	virtual UWorld* GetWorld() const override { return GetOuter() ? GetOuter()->GetWorld() : nullptr; }
	// End UObject Interface

	/** Play next voice line in the conversation */
	UFUNCTION()
	void PlayNextLine();

	/** Abort the conversation. Will not continue next voice line. */
	UFUNCTION(BlueprintCallable, Category = "Conversation")
	void AbortConversation();

	/** Pause the Conversation and kill the currently playing Voice line. */
	UFUNCTION()
	void PauseConversation();

	/** Resume conversation from the last played voice line. */
	UFUNCTION()
	void ResumeConversation();

	/** Initialize the data for the new conversation object. */
	UFUNCTION()
	void InitializeConversation(FSCConversationData InConversationData, USCConversationManager* Manager);

	/** Return the priority from our conversation data. */
	UFUNCTION()
	FORCEINLINE int32 GetConversationPriority() const { return ConversationData.ConversationPriority; }

	/** Return the list of Characters participating in this conversation */
	UFUNCTION()
	FORCEINLINE TArray<ASCCounselorCharacter*> GetConversingCharacters() { return ConversationData.Participants; }

	/** Returns wether or not the conversation is paused */
	UFUNCTION(BlueprintPure, Category="Conversation")
	FORCEINLINE bool GetIsPaused() const { return bConversationPaused; }

	// Event for when the conversation has completed successfully
	UPROPERTY(BlueprintAssignable)
	FConversationDoneEvent OnConversationDone;

	// Event for when the conversation was aborted
	UPROPERTY(BlueprintAssignable)
	FConversationDoneEvent OnConversationAborted;

protected:

	// The voice line we need to play next. Also used to tell if we're at the end of the conversation.
	UPROPERTY(BlueprintReadOnly)
	int32 NextVoiceLineIndex;

	// Timer handle for delaying the next audio line.
	FTimerHandle TimerHandle_VoiceOverPostDelay;

	// Internal delegate bound for the Conversation manager to know when conversations complete.
	UPROPERTY()
	FConversationComplete ConversationComplete;

	// The stored conversation data.
	UPROPERTY()
	FSCConversationData ConversationData;

	// Is the conversation suspended for some reason? (A counselor is alerted but not running yet?)
	UPROPERTY()
	bool bConversationPaused;


};
