// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCVoiceOverComponent.h"
#include "SCVoiceOverComponent_Killer.generated.h"

/**
 * @class USCVoiceOverComponent_Killer
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SUMMERCAMP_API USCVoiceOverComponent_Killer
: public USCVoiceOverComponent
{
	GENERATED_BODY()

public:
	USCVoiceOverComponent_Killer(const FObjectInitializer& ObjectInitializer);

	virtual void PlayVoiceOver(const FName VoiceOverName, bool CinematicKill = false, bool Queue = false) override;

protected:
	void DeferredPlayVoiceOver();

	UPROPERTY(Transient)
	TAssetPtr<USoundBase> PendingVoiceOver;

private:
	UPROPERTY()
	TArray<USoundWave*> LocalizedSoundWaves;
};
