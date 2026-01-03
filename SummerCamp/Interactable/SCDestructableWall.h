// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCContextKillActor.h"

#include "DestructibleComponent.h"

#include "ILLInteractableComponent.h"

#include "SCContextComponent.h"
#include "SCDestructableWall.generated.h"

class AILLCharacter;
class USCContextKillComponent;
class USCSpecialMoveComponent;

/**
 * @struct FSCWallHealth
 */
USTRUCT(BlueprintType)
struct FSCWallHealth
{
	GENERATED_BODY()

	/** Actual value of the helath of the wall */
	UPROPERTY(BlueprintReadOnly)
	float Health;

	/** Side of the wall the last attacker was on */
	UPROPERTY(BlueprintReadOnly)
	bool bLastHitFromInside;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		uint8 ByteHealth = FMath::Quantize8UnsignedByte(Health);
		Ar << ByteHealth;
		Ar.SerializeBits(&bLastHitFromInside, 1);
		Health = (float)ByteHealth / 255.f;

		bOutSuccess = true;
		return true;
	}
};

template<>
struct TStructOpsTypeTraits<FSCWallHealth>
: public TStructOpsTypeTraitsBase2<FSCWallHealth>
{
	enum
	{
		WithNetSerializer = true
	};
};

/**
 * @class ASCDestructableWall
 */
UCLASS()
class SUMMERCAMP_API ASCDestructableWall
: public ASCContextKillActor
{
	GENERATED_BODY()

public:
	ASCDestructableWall(const FObjectInitializer& ObjectInitializer);

	// BEGIN AActor interface
	virtual void PostInitializeComponents() override;
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// END AActor interface

	/** Callback for interaction with the InteractComponent */
	UFUNCTION()
	void OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod);

	/** Callback for when our hold state changes */
	UFUNCTION()
	void OnHoldStateChanged(AActor* Interactor, EILLHoldInteractionState NewState);

	/** Callback for checking if a given character can interact with the InteractComponent */
	UFUNCTION()
	int32 CanInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation);

	/** Callback to check if we should highlight the InteractComponent */
	UFUNCTION()
	bool ShouldHighlight(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation);

	/** If true, this wall been blown up! */
	UFUNCTION(BlueprintPure, Category = "Wall|Destruction")
	FORCEINLINE bool IsDestroyed() const { return bIsDestroyed; }

	/** Checks which side of the wall the passed in interactor is on */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsInteractorInside(const AActor* Interactor) const;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UStaticMeshComponent* ContextMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default")
	USCContextKillComponent* OuterContextKillComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UDestructibleComponent* DestructibleSection;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UBoxComponent* BlockingCollision;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UArrowComponent* FacingDirection;

	UPROPERTY(EditDefaultsOnly, Category = "Default")
	UDestructibleMesh* LightDestructionInsideMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Default")
	UDestructibleMesh* LightDestructionOutsideMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Default", meta=(ClampMin = 0.1f, ClampMax = 1.f, UIMin = 0.1f, UIMax = 1.f))
	float LightDestructionThreshold;

	UPROPERTY(EditDefaultsOnly, Category = "Default")
	UDestructibleMesh* MediumDestructionInsideMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Default")
	UDestructibleMesh* MediumDestructionOutsideMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Default", meta=(ClampMin = 0.1f, ClampMax = 1.f, UIMin = 0.1f, UIMax = 1.f))
	float MediumDestructionThreshold;

	UPROPERTY(EditDefaultsOnly, Category = "Default")
	UDestructibleMesh* HeavyDestructionInsideMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Default")
	UDestructibleMesh* HeavyDestructionOutsideMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Default", meta=(ClampMin = 0.1f, ClampMax = 1.f, UIMin = 0.1f, UIMax = 1.f))
	float HeavyDestructionThreshold;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UParticleSystem* HitParticle;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UParticleSystem* DestroyParticle;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue* WallDestroySoundCue;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue* WallHitSoundCue;

	//////////////////////////////////////////////////////////////////////////////////
	// Destroy Wall
public:
	/** Allows us to destroy the wall from serialization of Health */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void DestroyWall(const AActor* DamageCauser, float Impulse);

	// ~Destroy Wall
	//////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////////
	// Break Down
public:
	// Handler for whacking away at the door from the inside
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSpecialMoveComponent* BreakDownFromInside_SpecialMoveHandler;

	// Handler for whacking away at the door from the outside
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSpecialMoveComponent* BreakDownFromOutside_SpecialMoveHandler;

	// Wall break down sync point for jumping to success or not
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCAnimDriverComponent* BreakDownDriver;

	/** Wall break down sync point for looping or not */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCAnimDriverComponent* BreakDownLoopDriver;

	UPROPERTY()
	class ASCKillerCharacter* KillerBreakingDown;

	/** Break down special move start */
	UFUNCTION()
	void OnBreakDownStart(ASCCharacter* Interactor);

	/** Break down special move stop */
	UFUNCTION()
	void OnBreakDownStop(ASCCharacter* Interactor);

	/** Used to decide if this next swing is going to be the last, and we should switch to the success state */
	UFUNCTION()
	void BreakDownSync(FAnimNotifyData NotifyData);

	/** Used to process break down damage. */
	UFUNCTION()
	void BreakDownDamage(FAnimNotifyData NotifyData);

	/** Used to decide if the player still wants to attack the wall or not */
	UFUNCTION()
	void BreakDownLoop(FAnimNotifyData NotifyData);

	/** If true, we have a killer knocking at our wall */
	bool IsBeingBrokenDown() const { return KillerBreakingDown != nullptr; }

private:
	/** Stops KillerBreakingDown from hitting the wall anymore */
	bool TryAbortBreakDown(bool bForce = false);

	// If true, we shouldn't abort
	bool bBreakDownSucceeding;

	// ~Break Down
	//////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////////
	// Burst through SpecialMove
public:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCAnimDriverComponent* BurstDriver;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSpecialMoveComponent* BurstFromInsideSpecialMove;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSpecialMoveComponent* BurstFromOutsideSpecialMove;

	/** Called when the burst special move starts */
	UFUNCTION()
	void BurstStarted(ASCCharacter* Interactor);

	/** Called when the burst special move completes */
	UFUNCTION()
	void BurstComplete(class ASCCharacter* Interactor);

	UFUNCTION()
	void WallBurst(FAnimNotifyData NotifyData);

	UPROPERTY(EditDefaultsOnly, Category = "Destruction")
	float HitExplosionForce;

	UPROPERTY(EditDefaultsOnly, Category = "Destruction")
	float BurstExplosionForce;

	// ~Burst through SpecialMove
	//////////////////////////////////////////////////////////////////////////////////

	/** Returns HealthData.Health for easy access */
	UFUNCTION(BlueprintPure, Category = "Wall")
	FORCEINLINE float GetHealth() const { return HealthData.Health * BaseHealth; }

protected:
	// Serialized health with attacker direction included
	UPROPERTY(Replicated, ReplicatedUsing = OnRep_Health, transient, BlueprintReadOnly, Category = "Wall")
	FSCWallHealth HealthData;

	// Starting health for the wall (set on the server then replicated out)
	UPROPERTY(EditDefaultsOnly, Category = "Wall")
	float BaseHealth;

	// If true, we been blown up (simulated based on health)
	bool bIsDestroyed;

	// Time in seconds to cleanup the destruction mesh after being fully destroyed (handled locally)
	UPROPERTY(EditDefaultsOnly, Category = "Wall")
	float WallLifespanAfterDestruction;

	FTimerHandle TimerHandle_WallLifespan;

	/** Handles setting damaged/destroyed states based on the incomming serialized health */
	UFUNCTION()
	void OnRep_Health(FSCWallHealth OldHealthData);

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	class USCSpacerCapsuleComponent* InsideSpacer;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	class USCSpacerCapsuleComponent* OutsideSpacer;
};
