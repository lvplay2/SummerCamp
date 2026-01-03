// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Components/ActorComponent.h"
#include "SCVoiceOverObject.h"
#include "SCVoiceOverComponent.generated.h"

class ASCCharacter;

/**
 * @class USCVoiceOverComponent
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SUMMERCAMP_API USCVoiceOverComponent
: public UActorComponent
{
	GENERATED_BODY()

public:
	USCVoiceOverComponent(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Voice Over")
	virtual void PlayVoiceOver(const FName VoiceOverName, bool CinematicKill = false, bool Queue = false);

	UFUNCTION(BlueprintCallable, Category = "Voice Over")
	virtual void PlayVoiceOverBySoundCue(USoundCue *SoundCue);

	UFUNCTION(BlueprintCallable, Category = "Voice Over")
	virtual void StopVoiceOver();

	UFUNCTION(BlueprintCallable, Category = "Voice Over")
	virtual bool IsVoiceOverPlaying(const FName VoiceOverName);

	virtual void InitVoiceOverComponent(TSubclassOf<USCVoiceOverObject> VOClass) { VoiceOverObjectClass = VOClass; }

	UFUNCTION()
	virtual void CleanupVOComponent();

	/** Checks to see if the passed in FName is in the list and if we're passed the timestamp specified, true if we're still on cooldown (don't play!) */
	bool IsOnCooldown(const FName VOName);

	/** Adds a name and timestamp pair to our cooldown list, IsOnCooldown will return true until the timestamp is passed. If a name already exists in the list it will not be added again (and the cooldown will not be restarted) */
	void StartCooldown(const FName VOName, const float CooldownTime);

protected:
	/** Helper function to quickly tell if this is locally controlled. */
	bool IsLocallyControlled() const;

	ASCCharacter* GetCharacterOwner() const;

	UPROPERTY()
	TSubclassOf<USCVoiceOverObject> VoiceOverObjectClass;

	// Pair of names and timestamps for when we can use this VO again (cleaned up everytime we call IsOnCooldown)
	TArray<TPair<FName, float>> VOCooldownList;
};
