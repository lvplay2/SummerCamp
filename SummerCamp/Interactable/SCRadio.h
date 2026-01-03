// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "SCAnimDriverComponent.h"
#include "SCRadio.generated.h"

enum class EILLInteractMethod : uint8;

class USCInteractComponent;
class USCScoringEvent;
class USCSpecialMoveComponent;

/**
 * @class ASCRadio
 */
UCLASS()
class SUMMERCAMP_API ASCRadio
: public AActor
{
	GENERATED_BODY()

public:
	ASCRadio(const FObjectInitializer& ObjectInitializer);

	// BEGIN AActor interface
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// END AActor interface

	UFUNCTION()
	void OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod);

	UFUNCTION()
	int32 CanInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation);

	// Special move handler for breaking the radio
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSpecialMoveComponent* Destroy_SpecialMoveHandler;

	// Special move handler for turning the radio on/changing songs
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSpecialMoveComponent* CounselorUse_SpecialMoveHandler;

	FORCEINLINE bool IsPlaying() const { return bIsPlaying; }

	FORCEINLINE bool IsBroken() const { return bIsBroken; }

	FORCEINLINE USCInteractComponent* GetInteractComponent() const { return InteractComponent; }

	float SoundBlipFrequencyTimer;

	void UpdateStreamerMode(bool bStreamerMode);

protected:
	// Handles destroying the radio at the right moment in the animation
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCAnimDriverComponent* DestructionDriver;

	// Handles adjusting radio features at the right moment in the animation
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCAnimDriverComponent* UseRadioDriver;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	UAudioComponent* MusicComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	UAudioComponent* SoundEffectsComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	USCInteractComponent* InteractComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Perks")
	USphereComponent* PerkStaminaModifierVolume;

protected:
	/** Notify when a counselor is in the perk modifier volume */
	UFUNCTION()
	void OnBeginPerkVolumeOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Notify when a counselor leaves the perk modifier volume */
	UFUNCTION()
	void OnEndPerkVolumeOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	

	///////////////////////////////////////////////////
	// Use Radio
protected:
	/** SpecialMove handler for starting using the radio */
	UFUNCTION()
	void OnUseRadioStarted(ASCCharacter* Interactor);

	/** SpecialMove handler for finishing using the radio */
	UFUNCTION()
	void OnUseRadioComplete(ASCCharacter* Interactor);
	
	/** AnimDriver callback for using the radio */
	UFUNCTION()
	void OnUseRadio(FAnimNotifyData NotifyData);

	// Is this interaction turning the radio on or changing tracks? (SERVER ONLY)
	UPROPERTY(Replicated, Transient)
	bool bIsChangingTracks;

public:

	/** Toggle the radio on and off */
	UFUNCTION(BlueprintCallable, Category="Radio")
	void ToggleMusic();

	/** Play the next track */
	UFUNCTION(BlueprintCallable, Category = "Radio")
	void NextTrack();

	///////////////////////////////////////////////////

	///////////////////////////////////////////////////
	// Break the radio
protected:
	/** SpecialMove handler for starting to break the radio */
	UFUNCTION()
	void DestructionStarted(ASCCharacter* Interactor);

	/** SpecialMove handler for finishing breaking the radio */
	UFUNCTION()
	void DestructionComplete(ASCCharacter* Interactor);

	/** AnimDriver callback for breaking the radio */
	UFUNCTION()
	void OnBreakRadio(FAnimNotifyData NotifyData);

	void BreakRadio();

	/** BP Function to handle destruction */
	UFUNCTION(BlueprintImplementableEvent, Category = "Radio")
	void OnRadioDestroyed();
	///////////////////////////////////////////////////
	
	UPROPERTY(ReplicatedUsing=OnRep_IsPlaying, BlueprintReadOnly, Category = "Default")
	bool bIsPlaying;

	UPROPERTY(ReplicatedUsing=OnRep_Broken, BlueprintReadOnly, Category = "Default")
	bool bIsBroken;

private:
	UFUNCTION()
	void OnRep_IsPlaying();

	UFUNCTION()
	void OnRep_Broken();

	UFUNCTION()
	void PlaySong();

	UFUNCTION()
	void StartMusic(const TAssetPtr<USoundCue>& SongToPlay, float StartTime);

	UFUNCTION()
	void StopMusic();

	float PendingMusicStartTime;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TArray<TAssetPtr<USoundCue>> Songs;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TAssetPtr<USoundCue> StreamerModeSong;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	USoundCue* TurnOnSound;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	USoundCue* TurnOffSound;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	USoundCue* BreakSound;

	int32 CurrentSong;

	FTimerHandle TimerHandle_NextTrack;

	bool IsStreamerMode();

	///////////////////////////////////////////////////
	// Badges
public:
	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	TSubclassOf<USCScoringEvent> TurnRadioOnScoreEvent;
};
