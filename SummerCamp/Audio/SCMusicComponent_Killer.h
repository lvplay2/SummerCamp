// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCMusicComponent.h"
#include "SCMusicObject_Killer.h"
#include "SCMusicComponent_Killer.generated.h"

class ASCCounselorCharacter;

/**
 * @class USCMusicComponent_Killer
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SUMMERCAMP_API USCMusicComponent_Killer
: public USCMusicComponent
{
	GENERATED_BODY()

public:
	USCMusicComponent_Killer(const FObjectInitializer& ObjectInitializer);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void CounselorInView(ASCCounselorCharacter* VisibleCharacter);
	void CounselorOutOfView(ASCCounselorCharacter* VisibleCharacter);

	virtual void CleanupMusic(bool SkipTwoMinuteWarningMusic = false);

protected:
	virtual void PostTrackFinished() override;

	virtual void PostInitMusicComponenet() override;

private:
	void HandleCounselorAIAlerted();
	void HandleCounselorInRange();

	void ChangeDefferedLayer();

	void GoToSearchingLayer();

	/** Array of Counselors currently in view. */
	UPROPERTY()
	TArray<ASCCounselorCharacter*> CounselorsInView;

	ESCKillerMusicLayer CurrentLayer;

	FTimerHandle TimerHandle_AddDefferedTimer;
	FTimerHandle TimerHandle_RemoveDefferedTimer;
	FTimerHandle TimerHandle_LostTargetCooldownTimer;

	bool bIsSinglePlayerChallenges;
};
