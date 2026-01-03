// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCItem.h"
#include "SCAnimDriverComponent.h"
#include "SCCharacter.h"
#include "SCStunDamageType.h"
#include "SCTrap.generated.h"

class USCScoringEvent;

/**
 * @class ASCTrap
 */
UCLASS()
class SUMMERCAMP_API ASCTrap
: public ASCItem
{
	GENERATED_UCLASS_BODY()

public:
	// BEGIN AActor interface
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	// END AActor interface

	// BEGIN ASCItem interface
	virtual void OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod) override;
	virtual int32 CanInteractWith(class AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation) override;
	virtual bool Use(bool bFromInput) override;

protected:
	virtual void OwningCharacterChanged(ASCCharacter* OldOwner) override;
	// END ASCItem interface

public:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	void TrapCharacter(ASCCharacter* Character);

public:
	/** Reset trap rotation to default */
	FORCEINLINE void ResetRotation() { SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f)); }

	// The stun damage blueprint to use for the stun data on the left foot
	UPROPERTY(EditAnywhere, Category = "Trap|Stun")
	TSubclassOf<UDamageType> TrapStunLFClass;

	// The stun damage blueprint to use for the stun data on the left foot while sprinting
	UPROPERTY(EditAnywhere, Category = "Trap|Stun")
	TSubclassOf<UDamageType> TrapStunLFSprintingClass;

	// The stun damage blueprint to use for the stun data on the right foot
	UPROPERTY(EditAnywhere, Category = "Trap|Stun")
	TSubclassOf<UDamageType> TrapStunRFClass;

	// The stun damage blueprint to use for the stun data on the right foot while sprinting
	UPROPERTY(EditAnywhere, Category = "Trap|Stun")
	TSubclassOf<UDamageType> TrapStunRFSprintingClass;

	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	TSubclassOf<USCScoringEvent> TrapScoreEvent;

	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	TSubclassOf<USCScoringEvent> PlaceTrapScoreEvent;

	UFUNCTION()
	void SetTrapArmer(ASCCharacter* Armer) { TrapArmer = Armer; }

	UFUNCTION(BlueprintImplementableEvent, Category = "Trap")
	void OnTrapTriggered();

	/** Helper to see if this trap was placed by a killer */
	UFUNCTION(BlueprintPure, Category = "Trap")
	bool WasPlacedByKiller() const { return TrapArmer ? TrapArmer->IsKiller() : false; }

	// 2D Audio Cue the Killer hears in Stereo to alert he got a counselor.
	UPROPERTY(EditDefaultsOnly, Category = "Killer Alert")
	USoundCue* KillerAlertAudio;

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Trap")
	class USphereComponent* CollisionSphere;

	// Only used to determine if we're trying to place aonther trap on this one
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "trap")
	class UBoxComponent* PlacementCollision;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Trap|Sound")
	class UAudioComponent* TriggeredAudio;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Trap|Sound")
	class UAudioComponent* ArmingAudio;

	UPROPERTY(EditAnywhere, Category = "Trap")
	bool bStartArmed;

	UPROPERTY(EditDefaultsOnly, Category = "Trap")
	bool bCanArmerTrigger;

	UPROPERTY(EditDefaultsOnly, Category = "Trap")
	bool bCanPickUp;

	void ArmTrap(AActor* Interactor);

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_ArmTrap(AActor* Interactor);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_SetArmedRotation(const FRotator NewRotation);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_OnTriggered(FVector TriggeredLocation, bool bNotifyOwner = true);

	/**
	 * Performs a line trace to verify the space in front of the character is safe to place a trap
	 * @param OutBestCollision - Set to the HitResult you should use to place the trap
	 * @return If true, OutBestCollision has been set and the space is clear, if false, we're blocked by something or could not find the ground
	 */
	bool IsSpaceClear(FHitResult& OutBestCollision) const;

public:
	/** Play counselor escape animation for all clients */
	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_PlayEscape();

	FORCEINLINE bool GetIsArmed() const { return bIsArmed; }

	UFUNCTION(BlueprintCallable, Category = "KillerTrap")
	void ForceArmTrap() { ArmTrap(nullptr); }

	UFUNCTION(BlueprintCallable, Category = "KillerTrap")
	void ForceDisarmTrap() { bIsArmed = false; }

protected:
	/** Play placement animation on character for all clients */
	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_PlayPlacement(class ASCCharacter* Character);

	/** Ray trace and place trap on the ground in front of the counselor */
	UFUNCTION()
	void PlaceTrap();

	// Arming trap animation to play
	UPROPERTY(EditDefaultsOnly, Category = "Trap|Animation")
	UAnimSequenceBase* ArmedAnimation;

	// Triggered trap animation to play
	UPROPERTY(EditDefaultsOnly, Category = "Trap|Animation")
	UAnimSequenceBase* TriggeredAnimation;

	// Played when a character escapes the trap
	UPROPERTY(EditDefaultsOnly, Category = "Trap|Animation")
	UAnimSequenceBase* EscapedAnimation;

	// Counselor trap placement montage
	UPROPERTY(EditDefaultsOnly, Category = "Trap|Animation")
	UAnimMontage* CounselorPlaceMontage;

	// Counselor left foot caught in trap
	UPROPERTY(EditDefaultsOnly, Category = "Trap|Animation|Counselor")
	UAnimMontage* CounselorLeftFootCaught;

	// Counselor left foot caught in trap while sprinting
	UPROPERTY(EditDefaultsOnly, Category = "Trap|Animation|Counselor")
	UAnimMontage* CounselorLeftFootCaughtSprinting;

	// Counselor right foot caught in trap
	UPROPERTY(EditDefaultsOnly, Category = "Trap|Animation|Counselor")
	UAnimMontage* CounselorRightFootCaught;

	// Counselor right foot caught in trap while sprinting
	UPROPERTY(EditDefaultsOnly, Category = "Trap|Animation|Counselor")
	UAnimMontage* CounselorRightFootCaughtSprinting;

	// Jason left foot caught in trap while sprinting
	UPROPERTY(EditDefaultsOnly, Category = "Trap|Animation|Killer")
	UAnimMontage* JasonLeftFootCaught;

	// Jason right foot caught in trap
	UPROPERTY(EditDefaultsOnly, Category = "Trap|Animation|Killer")
	UAnimMontage* JasonRightFootCaught;

	// Damage applied when trap is triggered
	UPROPERTY(EditDefaultsOnly, Category = "Trap|Damage")
	float TrapDamage;

	// How much to scale damage by when trapping a killer rather than a counselor
	UPROPERTY(EditDefaultsOnly, Category = "Trap|Damage")
	float KillerDamageMultiplyer;

	// Distance (in cm) from the character the trap should be placed
	UPROPERTY(EditDefaultsOnly, Category = "Trap|Default")
	float PlacementDistance;

	// Can't pickup a trap after it's been escaped for this timelimit in seconds
	UPROPERTY(EditDefaultsOnly, Category = "Trap|Default")
	float TimelimitToPickup;

	// Arm the trap from idle special move
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Trap|Interaction")
	class USCSpecialMoveComponent* ArmTrap_SpecialMoveHandler;

	// disarm the trap from idle special move
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Trap|Interaction")
	class USCSpecialMoveComponent* DisarmTrap_SpecialMoveHandler;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCAnimDriverComponent* DisarmDriver;

	UFUNCTION()
	void OnLocationReached(ASCCharacter* Interactor);

	UFUNCTION()
	void OnTrapArmed(ASCCharacter* Interactor);

	UFUNCTION()
	void OnTrapDisarmed(ASCCharacter* Interactor);

	UFUNCTION()
	void OnTrapArmAborted(ASCCharacter* Interactor);

	UFUNCTION()
	void OnDisarmBeginDriver(FAnimNotifyData NotifyData);

	UFUNCTION()
	void OnDisarmStarted(ASCCharacter* Interactor);

	UFUNCTION()
	void OnDisarmComplete(ASCCharacter* Interactor);

	UFUNCTION()
	void OnDisarmAborted(ASCCharacter* Interactor);

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Default")
	bool bIsArmed;

	bool bIsPlacing;
	bool bIsArming;

private:
	UPROPERTY()
	ASCCharacter* TrapArmer;

	/** Sometimes we overlap a character but don't want to catch them in the trap yet */
	UPROPERTY()
	ASCCharacter* OverlappedCharacter;

	void StopUpdatingSkeleton(const float Delay);

	FTimerHandle TimerHandle_PlaceTrap;
	FTimerHandle TimerHandle_AnimationPlaying;

	// What time did we escape from the trap? Prevents automatically picking up the trap after breaking free
	float Escape_Timestamp;

	void TurnOffSkeletalUpdates();
};
