// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCProjectile.h"

#include "SCNoiseMaker_Projectile.generated.h"

class USCScoringEvent;

/**
 * @class ASCNoiseMaker_Projectile
 */
UCLASS()
class SUMMERCAMP_API ASCNoiseMaker_Projectile
: public ASCProjectile
{
	GENERATED_UCLASS_BODY()

public:
	// BEGIN AActor Interface
	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaSeconds) override;
	// End AActor interface

	/**
	 * Triggered on component overlap begin
	 * @param Actor overlapping
	 * @param Other component
	 * @param Bone index
	 * @param From sweep
	 * @param Sweep hit results
	 */
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	virtual void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// Begin ASCProjectile interface
	virtual void InitVelocity(const FVector& Direction) override;
	// End ASCProjectil interface

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UAudioComponent* Audio;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USkeletalMeshComponent* Mesh;

	// Activate damage/stun if this volume is entered
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USphereComponent* BlastComponent;

	float SoundBlipFrequencyTimer;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay")
	float ActivationTime;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay")
	float NoiseTimer;

	UPROPERTY(Replicated)
	bool bActivated;

	// Stun data for when Jason is inside the BlastRadius.
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UDamageType> KillerStunType;

	// The amount of damage Jason will take when inside the BlastRadius.
	UPROPERTY(EditDefaultsOnly)
	int32 KillerDamage;

	// Stun data for when a Counselor is inside the BlastRadius.
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UDamageType> CounselorStunType;

	// The amount of damage a COunselor will take when inside the BlastRadius.
	UPROPERTY(EditDefaultsOnly)
	int32 CounselorDamage;

	// Track which characters have been damaged by this noise maker so we can't damage them again.
	UPROPERTY()
	TArray<class ASCCharacter*> DamagedCharacters;

	// Score event for stunning Killer
	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	TSubclassOf<USCScoringEvent> StunJasonScoreEventClass;
};
