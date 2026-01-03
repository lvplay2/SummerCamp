// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Object.h"
#include "SCVoiceOverObject.generated.h"

/**
* @struct FVoiceOverData
*/
USTRUCT(BlueprintType)
struct FVoiceOverData
{
	GENERATED_USTRUCT_BODY()

	FVoiceOverData()
		: VoiceOverName(NAME_None)
		, VoiceOverFile(nullptr)
	{
	}

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName VoiceOverName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TAssetPtr<USoundBase> VoiceOverFile;
};

/**
 * 
 */
UCLASS(Blueprintable)
class SUMMERCAMP_API USCVoiceOverObject
: public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breathing")
	TAssetPtr<USoundCue> Jogging_Inhale;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breathing")
	TAssetPtr<USoundCue> Jogging_Exhale;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breathing")
	float JoggingWarmUpTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breathing")
	float JoggingCoolDownTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breathing")
	TAssetPtr<USoundCue> Sprinting_Inhale;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breathing")
	TAssetPtr<USoundCue> Sprinting_Exhale;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breathing")
	float SprintWarmUpTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breathing")
	float SprintCoolDownTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breathing")
	TAssetPtr<USoundCue> LowStamina_Inhale;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breathing")
	TAssetPtr<USoundCue> LowStamina_Exhale;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breathing")
	float LowStaminaWarmUpTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breathing")
	float LowStaminaCoolDownTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breathing")
	TAssetPtr<USoundCue> Hiding_Inhale;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breathing")
	TAssetPtr<USoundCue> Hiding_Exhale;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breathing")
	float HideWarmUpTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breathing")
	float HideCoolDownTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Voice Over")
	TArray<FVoiceOverData> VoiceOverData;

	FVoiceOverData GetVoiceOverDataByVOName(const FName VOName) const;
};
