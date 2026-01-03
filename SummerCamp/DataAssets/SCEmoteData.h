// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCEmoteData.generated.h"

class ASCCounselorCharacter;

/**
 * @class USCEmoteData
 */
UCLASS(Blueprintable, NotPlaceable)
class SUMMERCAMP_API USCEmoteData
: public UDataAsset
{
	GENERATED_BODY()

public:
	// Localized name of the emote
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default")
	FText EmoteName;

	// Flag for whether the emote animation loops
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default")
	bool bLooping;

	// Image for the emote
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UTexture2D* EmoteIcon;

	// Image for the emote unlock
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default")
	TSoftObjectPtr<UTexture2D> EmoteUnlockImg;
		
	// Animation to play for Males
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	UAnimMontage* MaleMontage;

	// Animation to play for Females
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	UAnimMontage* FemaleMontage;

	// List of counselor classes that can use this emote. If empty, ALL counselors can use it
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default")
	TArray<TSoftClassPtr<ASCCounselorCharacter>> AllowedCounselors;

	// Entitlement ID - Used if this emote is part of a DLC pack
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default")
	FString EntitlementID;

	// String ID used to tell if we've unlocked the given emote. An empty string means there is no unlock and it is always available
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default")
	FName SinglePlayerEmoteUnlockID;
};
