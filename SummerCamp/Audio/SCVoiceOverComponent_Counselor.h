// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCVoiceOverComponent.h"
#include "SCVoiceOverComponent_Counselor.generated.h"

class ASCCounselorCharacter;

/**
 * @enum EBreathType
 */
UENUM(BlueprintType)
enum class EBreathType : uint8
{
	INHALE,
	EXHALE
};

/**
 * @enum EBreathingState
 */
UENUM(BlueprintType)
enum class EBreathingState : uint8
{
	NONE,
	RUNNING,
	SPRITING,
	LOWSTAMINA,
	HIGHFEAR,
	HIDING,
};

/**
 * @class USCVoiceOverComponent_Counselor
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SUMMERCAMP_API USCVoiceOverComponent_Counselor
: public USCVoiceOverComponent
{
	GENERATED_BODY()

public:
	USCVoiceOverComponent_Counselor(const FObjectInitializer& ObjectInitializer);

	// Begin UActorComponent interface
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	// End UActorComponent interface

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breathing")
	float BreathRunningMinimum;

	virtual void PlayVoiceOver(const FName VoiceOverName, bool CinematicKill = false, bool Queue = false) override;

	virtual void PlayVoiceOverBySoundCue(USoundCue *SoundCue) override;

	virtual void StopVoiceOver() override;

	virtual void InitVoiceOverComponent(TSubclassOf<class USCVoiceOverObject> VOClass) override;

	virtual void CleanupVOComponent() override;

	virtual bool IsVoiceOverPlaying(const FName VoiceOverName) override;

	bool IsCounselorTalking() const;

	void HoldBreath(bool bHoldBreath);

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UAudioComponent* AudioComponent;

	bool PlayKillerInRangeWhileHidingVO(float KillerDistanceSq);

	bool PlayKillerInRangeVO(float KillerDistanceSq);
	void KillerOutOfView();

	/** Event called when the player is getting chased by Jason. */
	void OnKillerClosingInDuringChase(bool bGettingChased);

	void PlayCinematicVoiceOver(const FName VoiceOverName);

	float KillerChaseVODistance;

private:
	void DeferredPlayVoiceOver();
	void DeferredPlayCinematicVoiceOver();
	void DeferredPlayBreath();

	TAssetPtr<USoundBase> PendingVoiceOver;
	TAssetPtr<USoundBase> PendingCinematicVoiceOver;
	TAssetPtr<USoundBase> PendingBreathVoiceOver;

	UPROPERTY()
	TArray<USoundWave*> LocalizedSoundWaves;

	UFUNCTION()
	void OnCurrentPlayingVOEventFinished();

	void ProcessNextBreath();

	TAssetPtr<USoundCue> GetCurrentBreathSoundCue(EBreathingState CurrentState);

	float GetCoolDownTime(EBreathingState CurrentState);
	float GetWarmupTime(EBreathingState CurrentState);
	FString DebugBreathingStateString(EBreathingState CurrentState);

	void CheckBreathing();
	void PlayBreath(USceneComponent* ActorRootComponent);

	FTimerHandle TimerHandle_BreathTimer;
	FTimerHandle TimerHandle_BreathingWarmupTimer;
	FTimerHandle TimerHandle_BreathingCooldownTimer;

	EBreathType CurrentBreath;
	EBreathingState CurrentBreathingState;
	EBreathingState NextBreathingState;

	bool RanOutOfStamina;

	float DeadFriendlyVODistance;
	float KillerHidingVODistance;
	float KillerSpottedDistance;

	FName CurrentPlayingVOEvent;
	FName NextVoiceOverName;

	FTimerHandle TimerHandle_HidingFromKillerCooldown;
	float HidingFromKillerCooldownTime;

	FTimerHandle TimerHandle_SpottedKillerCooldownTime;
	float SpottedKillerCooldownTime;

	FTimerHandle TimerHandle_GettingChasedCooldown;
	float GettingChasedCooldown;

	bool KillerSpottedAndInRange;

	bool HasKillerClosingInDuringChase;

	bool IsInCinematicKill;
};
