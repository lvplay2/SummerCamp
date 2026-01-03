// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLCharacter.h"

#include "ILLNetSerialization.h"

#include "SCPlayerController.h"
#include "SCStunDamageType.h"
#include "SCVoiceOverObject.h"
#include "StatModifiedValue.h"
#include "SCCharacter.generated.h"

extern TAutoConsoleVariable<int32> CVarDebugCombat;

enum class EILLInteractMethod : uint8;
enum class EILLHoldInteractionState : uint8;

class ASCAfterimage;
class ASCCabin;
class AContextKillVolume;
class ASCDriveableVehicle;
class ASCItem;
class ASCWaypoint;
class ASCWeapon;

class UBehaviorTree;
class UPawnNoiseEmitterComponent;
class USCCharacterAIProperties;
class USCIndoorComponent;
class USCInteractableManagerComponent;
class USCInteractComponent;
class USCMinimapIconComponent;
class USCScoringEvent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIndoorsChanged, bool, IsInside);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacterDeath);

/** Static function to make sure the intended camera is the only active camera when doing camera switches */
static inline void MakeOnlyActiveCamera(AActor* OnActor, UCameraComponent* ActiveCamera)
{
	if (!ActiveCamera)
		return;

	TArray<UActorComponent*> ChildComponents = OnActor->GetComponentsByClass(UCameraComponent::StaticClass());

	// Deactivate all cameras on the actor
	for (UActorComponent* comp : ChildComponents)
	{
		if (UCameraComponent* cam = Cast<UCameraComponent>(comp))
		{
			cam->SetActive(false);
		}
	}

	// Re-activate the one we want active
	ActiveCamera->Activate();
}

/**
 * @enum ESCCharacterStance
 */
UENUM(BlueprintType)
enum class ESCCharacterStance : uint8
{
	Standing,
	Prone,
	Crouching,
	Swimming,
	Driving,
	Combat,
	Finished,
	Grabbed,
	Trapped,
};

/**
 * @class FSCBlockData
 */
USTRUCT()
struct FSCBlockData
{
	GENERATED_BODY()

	UPROPERTY()
	bool bIsBlocking;

	UPROPERTY()
	uint8 BlockAnimSlot;
};

/**
 * @struct FSCDebugSpawners
 * @brief Used to register CVars to track spawning data/locations for different classes
 */
USTRUCT()
struct FSCDebugSpawners
{
	GENERATED_BODY()

public:
	/** Classes we currently want to draw */
	UPROPERTY(Transient)
	TArray<TSubclassOf<AActor>> ActiveClasses;

	/** Possible classes we can search for */
	TMap<FName, TSubclassOf<AActor>> ClassNameMapping;

	/** If true, we're tracking at least one class */
	uint32 bHasClasses : 1;

	/** Registers commands for auto-complete */
	void RegisterCommands(UWorld* World);

	/** Unregisters commands for auto-complete */
	void UnregisterCommands();

	/** Adds CVars to an array */
	static void AddCommandToArray(const TCHAR* Name, IConsoleObject* CVar, TArray<IConsoleObject*>& Sink);

	/** Parses command line params */
	void ParseCommand(const TArray<FString>& Args);

	/** Draws information about classes to the screen */
	void Draw(UWorld* World, APawn* LocalPlayer) const;
};

/**
 * @class ASCCharacter
 */
UCLASS(Abstract, Config=Game)
class SUMMERCAMP_API ASCCharacter
: public AILLCharacter
{
	GENERATED_BODY()

public:
	ASCCharacter(const FObjectInitializer& ObjectInitializer);

	// Begin AActor interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostInitializeComponents() override;
	virtual void Destroyed() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void FellOutOfWorld(const class UDamageType& DamageType) override;
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser) override;
	// End AActor interface

	// Begin APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	virtual FRotator GetBaseAimRotation() const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	// End APawn interface

	UPROPERTY(BlueprintReadOnly, Category = "Pawn")
	bool bIsPossessed;

	// Begin AILLCharacter interface
	virtual void OnReceivedController() override;
	virtual void OnReceivedPlayerState() override;
	virtual bool IsFirstPerson() const override { return false; }
	virtual void UpdateCharacterMeshVisibility() override;
	virtual void StartSpecialMove(TSubclassOf<UILLSpecialMoveBase> SpecialMove, AActor* TargetActor = nullptr, const bool bViaReplication = false) override;
	virtual void ServerStartSpecialMove_Implementation(TSubclassOf<UILLSpecialMoveBase> SpecialMove, AActor* TargetActor) override;
	virtual void OnSpecialMoveStarted() override;
	virtual void OnSpecialMoveCompleted() override;
	virtual void OnInteract(UILLInteractableComponent* Interactable, EILLInteractMethod InteractMethod) override;
	virtual void OnInteractAccepted(UILLInteractableComponent* Interactable, EILLInteractMethod InteractMethod) override;
	virtual bool IsInSpecialMove() const override { return (ActiveSpecialMove != nullptr || bClientInSpecialMove); }
	virtual bool CanInteractAtAll() const override;
	// End AILLCharacter interface

	/** Called when a special move is canceled, calls to OnSpecialMoveCompleted */
	UFUNCTION()
	virtual void OnSpecialMoveCanceled();

	/** Some function that is only necessary due to bad system design/use. */
	virtual void SpecialMoveSlopAssHack();

	/** Gets the character controller as an SCPlayerController */
	virtual AController* GetCharacterController() const { return Controller; }

	virtual ASCDriveableVehicle* GetVehicle() const { return nullptr; }

	void SetVehicleCollision(bool bEnable);

	virtual void CleanupLocalAfterDeath(ASCPlayerController* PlayerController);

protected:
	// Is this a beauty pose character? Used for character selection typically
	UPROPERTY(EditDefaultsOnly, Category="Character")
	bool bBeautyPose;

public:
	/** get the player state for this character. */
	virtual ASCPlayerState* GetPlayerState() const;

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_SetActorRotation(FVector_NetQuantizeNormal InRotation, const bool bShouldTeleport = false);

	UFUNCTION()
	virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	virtual void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION(BlueprintCallable, Category = "Gravity")
	void SetGravityEnabled(bool InEnable);

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_SetGravityEnabled(bool InEnable);

	UFUNCTION(NetMulticast, Reliable)
	void MULTI_SetGravityEnabled(bool InEnable);

	virtual bool IsCounselor() const { return false; }
	virtual bool IsKiller() const { return false; }

	/** @return true if the character is dodging in either direction */
	UFUNCTION(BlueprintPure, Category = "Combat")
	virtual bool IsDodging() const { return false; }

	UFUNCTION(BlueprintPure, Category = "Counselor")
	virtual bool GetIsFemale() const { return false; }

	UFUNCTION(BlueprintPure, Category = "Counselor")
	virtual bool IsInWheelchair() const { return false; }

	UFUNCTION(BlueprintPure, Category = "Escape")
	virtual bool HasEscaped() const { return false; }

	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	TSubclassOf<USCScoringEvent> EscapeJasonGrabScoreEvent;

	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	TSubclassOf<USCScoringEvent> EscortEscapeScoreEvent;

	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	TSubclassOf<USCScoringEvent> SacrificeScoreEvent;

	////////////////////////////////////////////////////////////////////////////
	// AI/Sensing
public:
	// Noise emitting component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Noise")
	UPawnNoiseEmitterComponent* NoiseEmitter;

	/** Serves as a replacement for MakeNoise which takes Stealth into account. Name kinda sucks. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="AI", meta=(BlueprintProtected = "true"))
	virtual void MakeStealthNoise(float Loudness = 1.f, APawn* NoiseInstigator = nullptr, FVector NoiseLocation = FVector::ZeroVector, float MaxRange = 0.f, FName Tag = NAME_None)
	{
		MakeNoise(Loudness, NoiseInstigator, NoiseLocation, MaxRange, Tag);
	}

	////////////////////////////////////////////////////////////////////////////
	// Voice
public:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UAudioComponent* VoiceAudioComponent;

	/////////////////////////////////////////////////////////////////////////////
	// UI
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default")
	USCMinimapIconComponent* MinimapIconComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default")
	int32 UnlockLevel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Entitlements")
	FString UnlockEntitlement;

	/////////////////////////////////////////////////////////////////////////////
	// Controls
public:
	virtual void OnInteract1Pressed();
	virtual void OnInteract1Released();
	virtual void OnInteractSkill1Pressed();
	virtual void OnInteractSkill2Pressed();
	virtual void OnForwardPC(float Val);
	virtual void OnForwardController(float Val);
	virtual void OnRightPC(float Val);
	virtual void OnRightController(float Val);
	virtual void AddControllerYawInput(float Val) override;
	virtual void AddControllerPitchInput(float Val) override;
	virtual void OnCombatPressed();
	virtual void OnCombatReleased();
	virtual void UpdateInteractHold(float DeltaSeconds);
	virtual void ModifyInputSensitivity(FKey InputKey, float SensitivityMod);
	virtual void RestoreInputSensitivity(FKey InputKey);

	virtual bool IsInteractGameActive() const;

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void SERVER_OnInteract(UILLInteractableComponent* Interactable, EILLInteractMethod InteractMethod);

	virtual void StartTalking();
	virtual void StopTalking();
	virtual void OnShowScoreboard();
	virtual void OnHideScoreboard();

protected:
	bool bScoreboardActive;

public:
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void OnAttack();
	virtual void OnStopAttack();

	virtual void OnHeavyAttack();

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_Attack(bool HeavyAttack = false);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_Attack(bool HeavyAttack = false);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void PlayRecoil(const FHitResult& Hit);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_PlayRecoil(const FHitResult& Hit);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void PlayWeaponImpact(const FHitResult& Hit);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_PlayWeaponImpact(const FHitResult& Hit);

	virtual void ToggleInput(bool Enable);

	UPROPERTY()
	UInputComponent* InteractMinigameInputComponent;

	UPROPERTY()
	UInputComponent* WiggleInputComponent;

	UPROPERTY()
	UInputComponent* CombatStanceInputComponent;

	/** Make sure the server can let the clients know that the special move should play at a faster rate */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_SpeedUpSpecialMove();

	/** Make sure remote clients special moves at faster rate */
	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_SpeedUpSpecialMove();

	/** Time limit for double press, retreived from DefaultInput.ini */
	static float DoublePressTime;

	virtual float GetStatInteractMod() const { return 1.f; }
	virtual float GetPerkInteractMod() const { return 1.f; }

protected:
	/** Rotation rate (degrees/second scaled by input modifier (defaults to 2)) for the camera to handle delta time correctly. Pitch/Yaw/Roll */
	UPROPERTY(EditDefaultsOnly)
	FVector CameraRotateRate;

	/////////////////////////////////////////////////////////////////////////////
	// Interactables
public:
	/** The time in seconds to finish interacting with the nearest interactable */
	UFUNCTION(BlueprintPure, Category="Interaction")
	virtual float GetInteractTimePercent() const;

	void SetInteractTimePercent(float NewInteractTime);

	virtual USCInteractableManagerComponent* GetInteractableManagerComponent() const { return InteractableManagerComponent; }

	USCInteractComponent* GetInteractable() const;

	void SetInteractSpeedModifier(float Modifier) { InteractSpeedModifier = Modifier; }
	float GetInteractSpeedModifier() const { return InteractSpeedModifier; }

	bool InteractTimerPassedMinHoldTime() const { return MinInteractHoldTime < InteractTimer; }

	// Flag set when we call OnInteract on the local client.
	bool bAttemptingInteract;

	/** Minimum amount of time to hold the interact button before considering it held. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	float MinInteractHoldTime;

	bool bHoldInteractStarted;

	TArray<USCInteractComponent*> InRangeInteractables;

	/** Called when hold interact is begun or canceled */
	void OnHoldInteract(USCInteractComponent* Interactable, EILLHoldInteractionState NewState);

	void PlayInteractAnim(USCInteractComponent* Interactable);
	void FailureInteractAnim();
	void CancelInteractAnim();
	void FinishInteractAnim();

	void ForceStopInteractAnim();

	UFUNCTION(Client, Reliable)
	void CLIENT_CancelRepairInteract();

	UFUNCTION()
	virtual void Native_NotifyMinigameFail(USoundCue* FailSound);

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void SERVER_NotifyMinigameFail(USoundCue* FailSound);

	UFUNCTION(Client, Reliable)
	virtual void CLIENT_NotifyMinigameFail(USoundCue* FailSound);

	/** When called should clear any interactions including minigames, special moves and hold interactions */
	virtual void ClearAnyInteractions();

	/** @return true if the interact button is pressed */
	FORCEINLINE bool IsHoldInteractButtonPressed() const {	return InteractPressed;	}

protected:
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_PlayInteractAnim(USCInteractComponent* Interactable);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_PlayInteractAnim(USCInteractComponent* Interactable);

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_FailureInteractAnim();

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_FailureInteractAnim();

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_CancelInteractAnim();

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_CancelInteractAnim();

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_FinishInteractAnim();

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_FinishInteractAnim();

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_ForceStopInteractAnim();

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_ForceStopInteractAnim();

	/** All interaction delegates should be called from the server */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_OnHoldInteract(USCInteractComponent* Interactable, EILLHoldInteractionState NewState);

	/** Called from the interactable manager when locking an interaction is attempted */
	UFUNCTION()
	virtual void OnClientLockedInteractable(UILLInteractableComponent* LockingInteractable, const EILLInteractMethod InteractMethod, const bool bSuccess);

	/** Called from the server if attempting the interaction is canceled before actually calling AttemptInteract */
	UFUNCTION(Client, Reliable)
	void CLIENT_CancelInteractAttempt();

	UFUNCTION(Client, Reliable)
	void CLIENT_CancelInteractAttemptFlag();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Character", meta = (AllowPrivateAccess = "true"))
	USCInteractableManagerComponent* InteractableManagerComponent;

	float InteractSpeedModifier;

	// Interaction time to record how long we hold the interact button
	float InteractTimer;
	bool InteractPressed;

	/////////////////////////////////////////////////////////////////////////////
	// Movement
public:
	bool IsSprinting() const { return bIsSprinting; }
	bool IsRunning() const { return bIsRunning; }
	bool IsCrouching() const { return (CurrentStance == ESCCharacterStance::Crouching); }

	virtual void SetSprinting(bool bSprinting);
	virtual void SetRunning(bool bRunning);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetSprinting(bool bSprinting);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetRunning(bool bRunning);

	/** Get move speed modifier */
	UFUNCTION(BlueprintPure, Category = "Pawn|Movement")
	virtual float GetSpeedModifier() const;

	/** get the modifier value for running speed */
	UFUNCTION(BlueprintCallable, Category = "Pawn|Movement")
	virtual float GetRunningSpeedModifier() const;

	// Called after a block hit reaction from AnimBP
	UFUNCTION(BlueprintCallable, Category = "Pawn|Blocking")
	void OnEndBlockReation();

protected:

	// Nudge players away from nearby players if they're too close.
	UFUNCTION()
	void CharacterBlockingUpdate();

	UPROPERTY(EditDefaultsOnly, Category = "Pawn|Blocking")
	bool bAllowBlockingCorrection;

	UPROPERTY(EditDefaultsOnly, Category = "Pawn|Blocking", meta = (ClampMin = 0, UIMin = 0))
	float BlockingBufferDistance;

	UPROPERTY(EditDefaultsOnly, Category = "Pawn|Blocking", meta = (ClampMin = 0, UIMin = 0, ClampMax = 1, UIMax = 1))
	float CorrectionStrength;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Pawn|Movement")
	bool bIsSprinting;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Pawn|Movement")
	bool bIsRunning;

	UPROPERTY(Replicated, Transient)
	bool bNoWriggleOverride;

	/////////////////////////////////////////////////////////////////////////////
	// Water/Swimming
public:
	/** Get how bouyant the character should be in the water given their speed */
	UFUNCTION(BlueprintPure, Category = "Movement|Swimming")
	float GetBuoyancy();

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Swimming")
	bool bInWater;

	UFUNCTION(BlueprintImplementableEvent, Category = "Swimming")
	void OnInWaterChanged(bool InWater);

	/** Blueprint event to activate drown cam and any effects we need while playing the kill */
	UFUNCTION(BlueprintImplementableEvent, Category = "Swimming|Interaction")
	void ActivateDrown();

	/** Gives the current depth (in cm) of the water the character is in. If they are not in water, returns 0 */
	UFUNCTION(BlueprintPure, Category = "Swimming")
	virtual float GetWaterDepth() const;

	UFUNCTION(BlueprintPure, Category = "Swimming")
	virtual float GetWaterZ() const { if (CurrentWaterVolume) return CurrentWaterVolume->GetActorLocation().Z + CurrentWaterVolume->GetSimpleCollisionHalfHeight(); return 0.f; }

	/** Update our swimming stance/movement (by default Characters don't support any) */
	virtual void UpdateSwimmingState() { }

protected:
	UPROPERTY()
	AVolume* CurrentWaterVolume;

	/////////////////////////////////////////////////////////////////////////////
	// Health/Damage
public:
	// We need a multicast to handle simulated shit on taking damage
	UFUNCTION(NetMulticast, Reliable)
		virtual void MULTICAST_DamageTaken(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser);

	/** 
	* Checks if the character is in a state to take damage
	* @Return true if the character should take damage
	*/
	virtual bool CanTakeDamage() const;

	UFUNCTION(BlueprintCallable, Category = "Pawn|Health")
	void SetMaxHealth(float _Health);

	/** get max health */
	UFUNCTION(BlueprintPure, Category = "Pawn|Health")
	int32 GetMaxHealth() const;

	/** check if pawn is still alive */
	UFUNCTION(BlueprintPure, Category = "Pawn|Health")
	bool IsAlive() const;

	/** Pawn suicide */
	virtual void Suicide();

	/** Kill this pawn */
	virtual void KilledBy(class APawn* EventInstigator);

	/** Returns True if the pawn can die in the current state */
	virtual bool CanDie(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser) const;

	/**
	 * Kills pawn, Server/authority only
	 * @param KillingDamage - Damage amount of the killing blow
	 * @param DamageEvent - Damage event of the killing blow
	 * @param Killer - Who killed this pawn
	 * @param DamageCauser - The Actor that directly caused the damage (i.e. the Weapon)
	 * @returns true if allowed
	 */
	virtual bool Die(float KillingDamage, struct FDamageEvent const& DamageEvent, class AController* Killer, class AActor* DamageCauser);

	/** Returns true if in ragdoll */
	UFUNCTION(BlueprintPure, Category = "Physics")
	bool IsRagdoll() const;

	UFUNCTION(BlueprintPure, Category = "Pawn|Health")
	float GetForceKillTimerElapsed();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "Physics")
	bool bIgnoreDeathRagdoll;

	bool bPauseAnimsOnDeath;

	UFUNCTION()
	virtual void ForceKillCharacter(ASCCharacter* Killer, FDamageEvent DamageEvent = FDamageEvent());

	/** Identifies if the pawn is in its dying state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	uint32 bIsDying : 1;

	/** Current health of the Pawn */
	UPROPERTY(Replicated, Transient, BlueprintReadOnly, Category = "Health")
	FFloat_NetQuantize Health;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float MaxHealth;

	/** list of actors being hit */
	UPROPERTY()
	TArray<AActor*> HitActors;

	/** switch to ragdoll */
	UFUNCTION()
	void SetRagdollPhysics(FName BoneName = NAME_None, bool bSimulate = true, float BlendWeight = 1.f, bool SkipCustomPhys = false, bool MakeServerCall = false);
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_SetRagdollPhysics(FName BoneName = NAME_None, bool Simulate = true, float BlendWeight = 1.f, bool SkipCustomPhys = false);
	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_SetRagdollPhysics(FName BoneName = NAME_None, bool Simulate = true, float BlendWeight = 1.f, bool SkipCustomPhys = false);

	/** Delegate for when this character has been killed */
	UPROPERTY(BlueprintAssignable, Category="Death")
	FOnCharacterDeath OnCharacterDeath;

protected:
	/** Notification when killed, for both the server and client. */
	virtual void OnDeath(float KillingDamage, struct FDamageEvent const& DamageEvent, ASCPlayerState* KillerPlayerState, AActor* DamageCauser);

	/** Notifies clients to play a death. */
	UFUNCTION(Reliable, NetMulticast)
	void ClientsOnDeath(float KillingDamage, struct FDamageEvent const& DamageEvent, ASCPlayerState* KillerPlayerState, AActor* DamageCauser);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_OnHit(float ActualDamage, ESCCharacterHitDirection HitDirection, int32 SelectedHit, bool bBlocking);

	/** Get the direction we were hit from */
	ESCCharacterHitDirection GetHitDirection(const FVector& VectorToDamageCauser);

	/** animation played on dying */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* DeathAnim;

	/** sound played on death, local player only */
	UPROPERTY(EditDefaultsOnly, Category = "Pawn")
	USoundCue* DeathSound;

	/** sound played on spawn */
	UPROPERTY(EditDefaultsOnly, Category = "Pawn")
	USoundCue* SpawnSound;

	UPROPERTY()
	FTimerHandle TimerHandle_ForceKill;

	virtual void ProcessWeaponHit(const FHitResult& Hit);

	/////////////////////////////////////////////////////////////////////////////
	// Stun
public:

	/** Are we currently under stun effects? */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Stun")
	bool bInStun;

	/** Set this player in the stunned state with the given stun data. */
	UFUNCTION(BlueprintCallable, Category = "Stun")
	virtual void BeginStun(TSubclassOf<USCStunDamageType> StunClass, const FSCStunData& StunData, ESCCharacterHitDirection HitDirection = ESCCharacterHitDirection::MAX);

	/** Pick the animation to play and send it to all clients */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_BeginStun(TSubclassOf<USCStunDamageType> StunClass, const FSCStunData& StunData, ESCCharacterHitDirection HitDirection = ESCCharacterHitDirection::MAX);

	/** Play stun on the client */
	UFUNCTION(NetMulticast, Reliable)
	virtual void MULTICAST_BeginStun(UAnimMontage* StunMontage, TSubclassOf<USCStunDamageType> StunClass, const FSCStunData& StunData, bool bIsValidStunArea);

	UFUNCTION(BlueprintCallable, Category = "Stun")
	virtual void EndStun();
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_EndStun();
	UFUNCTION(NetMulticast, Reliable)
	virtual void MULTICAST_EndStun();

	UFUNCTION(BlueprintPure, Category = "Stun")
	bool IsStunned() const;

	UFUNCTION(Server, Reliable, WithValidation)
	void SetStruggling(bool InStruggling);

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Stun")
	bool bIsStruggling;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grab")
	TSubclassOf<class USCSpecialMove_SoloMontage> GrabBreakSpecialMove;

	/** The socket name we want to attach other characters to */
	UPROPERTY(EditDefaultsOnly, Category = "Grab")
	FName CharacterAttachmentSocket;

	/** Attach this character to AttachTo's CharacterAttachmentSocket */
	UFUNCTION()
	virtual void AttachToCharacter(ASCCharacter* AttachTo);

	/** Remove this character from it's attachment */
	UFUNCTION(BlueprintCallable, Category = "Attachment")
	virtual void DetachFromCharacter();

	UFUNCTION(BlueprintCallable, Category = "Grab")
	virtual void BreakGrab(bool bStunWithPocketKnife = false);

	UFUNCTION(BlueprintImplementableEvent, Category = "Grab")
	void BP_BreakGrab(bool bWithPocketKnife = false);

	UFUNCTION(BlueprintCallable, Category = "Camera")
	void ActivateSplineCamera(class USCSplineCamera* ToActivate);

	UFUNCTION(BlueprintCallable, Category = "Camera")
	void ReactivateDynamicCams();

	/** Timer to know when a stun is finished */
	FTimerHandle TimerHandle_Stun;

	UFUNCTION(BlueprintPure, Category = "Stun")
	bool IsRecoveringFromStun() const { return bIsRecoveringFromStun; }

	/** Time the character is invulnerable after recovering from a stun */
	UPROPERTY(EditDefaultsOnly, Category = "Stun")
	float InvulnerabilityTime;

	UFUNCTION(BlueprintPure, Category = "Stun")
	bool IsInvulnerable() const { return bInvulnerable; }

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Stun|UI")
	FText StunText;

	/** The amount of time after a stun before another stun can be applied. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stun")
	float InvulnerableStunDuration;

private:
	/** Timer to know when the character is done recovering from a stun */
	FTimerHandle TimerHandle_StunRecovery;

	/** Is this character recovering from a stun */
	bool bIsRecoveringFromStun;

	/** Timer to know when the character is no longer invulnerable */
	FTimerHandle TimerHandle_Invulnerability;

	/** Is this character invulnerable */
	bool bInvulnerable;

	/** Check if the area around the character is a valid stun area */
	virtual bool IsValidStunArea(AActor* Attacker = nullptr) { return true; }

	/** The timer to handle how long a after a stun before another stun can be applied. */
	UPROPERTY()
	float InvulnerableStunTimer;

	UPROPERTY(Transient)
	const USCStunDamageType* ActiveStunDefaults;

	/////////////////////////////////////////////////////////////////////////////
	// Stun Wiggle
public:
	/** Initialize stamina total, stamina cost, and depletion rate from provided stun data. */
	virtual void InitStunWiggleGame(const USCStunDamageType* StunDefaults, const FSCStunData& StunData);

	/** Gets the current wiggle value */
	UFUNCTION(BlueprintPure, Category="Stun|Wiggle")
	float GetWiggleValue() const { return WiggleValue; }

	/**
	* The maximum number of wiggles possible per second.
	* This is used to keep players from possibly completing the mini game too fast if they can somehow wiggle too fast.
	*/
	UPROPERTY(EditAnywhere, Category = "Stun|Wiggle")
	int32 MaxWigglesPerSecond;

	UPROPERTY(EditAnywhere, Category = "Stun|Wiggle")
	float MinWiggleDelta;

protected:
	/** Update values for stun wiggle mini game */
	void UpdateStunWiggleValues(float DeltaTime);

	/** Resets wiggle values back to default */
	void ResetStunWiggleValues();

	/** The ratio for completion 0=failed/empty 1=successful/full */
	float WiggleValue;

	/** Track which direction the last wiggle registered was moving. */
	float LastMoveAxisValue;

	/** The number of wiggles registered. Reset every second to manage wiggles per second. */
	int32 NumWiggles;

	/** Timer for managing wiggles per second. */
	float RollingTime;

	/** Total time accrued during the mini game. */
	float TotalTime;

	/** Delta of the mini game to remove per wiggle (independent of stamina). */
	float DeltaPerWiggle;

	/** The stamina cost per wiggle registered. */
	float StaminaPerWiggle;

	/** The base depletion rate if the user is not interacting with the mini game. */
	float DepletionRate;

	/** Forces stun to have an upper bound */
	float MaxStunTime;

	/////////////////////////////////////////////////////////////////////////////
	// Attacking
public:
	/** check if pawn can use weapon */
	UFUNCTION(BlueprintPure, Category = "Combat")
	virtual bool CanAttack() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	virtual bool CanBlock() const;

	/** get attacking state */
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsAttacking() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsHeavyAttacking() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsRecoiling() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsWeaponStuck() const;

	/** Cached previous Attack trace weapon location */
	UPROPERTY()
	FVector PreviousTraceLocation;

	/** Cached previous attack trace weapon rotation */
	UPROPERTY()
	FRotator PreviousTraceRotation;

	/** Process the attack on this frame to see if we should damage a character */
	UFUNCTION(BlueprintCallable, Category = "Attack")
	void AttackTrace();

	/** Function call to set PreviousTraceLocation and PreviousTraceRotation to something more meaningful for the first frame. */
	UFUNCTION(BlueprintCallable, Category = "Attack")
	void AttackTraceBegin();

protected:
	float AttackHoldTimer;
	bool bAttackHeld;
	bool bHeavyAttacking;
	
	// For the current Attack Trace, did we already hit a blocking object other than another player? 
	// NOTE: This variable assumes that the "WeaponHits" TArray used in AttackTrace() contains hits in the order they are traced.
	bool bHitAnObstaclePrior;

public:
	UPROPERTY(BlueprintReadWrite, Category = "Attack")
	bool bPerformAttackTrace;

protected:
	bool bWeaponHitProcessed;

	/////////////////////////////////////////////////////////////////////////////
	// Inventory
public:
	void EquipPickingItem();
	virtual void DropEquippedItem(const FVector& DropLocationOverride = FVector::ZeroVector, bool bOverrideDroppingCharacter = false);

	/** @return Currently equipped weapon in our inventory. */
	UFUNCTION(BlueprintPure, Category = "Pawn|Inventory")
	ASCWeapon* GetCurrentWeapon() const;

	/** HACK: This is a temporary fix to allow us access to weapons that are spawned by the animation that Jason doesn't actually have an SCWeapon for */
	UPROPERTY()
	USkeletalMeshComponent* TempWeapon;

	// Keeps track of how many prop spawners are trying to hide the current item
	int32 PropSpawnerHiddingItem;

	UFUNCTION(BlueprintPure, Category = "Pawn|Inventory")
	ASCItem* GetEquippedItem() const;

	void ClearEquippedItem() { EquippedItem = nullptr; }

	UFUNCTION(BlueprintPure, Category = "Pawn|Inventory")
	bool IsEquippedItemNonWeapon() const;

	UFUNCTION(BlueprintCallable, Category = "Pawn|Inventory")
	void DestroyCurrentWeapon();

	/** Add Item to appropriate inventory, swap out items if inventory is over max item count */
	UFUNCTION(BlueprintCallable, Category = "Pawn|Inventory")
	virtual bool AddOrSwapPickingItem();

	UFUNCTION(BlueprintCallable, Category = "Pawn|Inventory")
	void SimulatePickupItem(ASCItem* Item);

	virtual void OnItemPickedUp(ASCItem* item) {};

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_SimulatePickupItem(ASCItem* Item);

	/** PickupItem hook. Plays montage. */
	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_SimulatePickupItem(ASCItem* Item);

	UPROPERTY(Replicated, Transient)
	ASCItem* PickingItem;

	// Called when any montage ends
	UFUNCTION()
	void OnMontageEnded(UAnimMontage* InMontage, bool bInterrupted);

	/** @return true if the player can pickup/take the item */
	bool CanTakeItem(const ASCItem* Item) const;

	/** Returns true if the player has this item class in their inventory */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pawn|Inventory")
	virtual bool HasItemInInventory(TSubclassOf<ASCItem> ItemClass) const;

	/** Returns the number of this type of item in the inventory */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pawn|Inventory")
	virtual int32 NumberOfItemTypeInInventory(TSubclassOf<ASCItem> ItemClass) const;

	/** Spawns a new item of the specified class and adds it to the inventory */
	void GiveItem(TSubclassOf<ASCItem> ItemClass);

	/** Spawns a new item of the specified class and adds it to the inventory asyncronously */
	void GiveItem(TSoftClassPtr<ASCItem> SoftItemClass);

protected:
	/** socket or bone name for attaching weapon mesh */
	UPROPERTY(EditDefaultsOnly, Category = "Pawn|Inventory")
	FName WeaponAttachPoint;

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_EquippedItem, BlueprintReadOnly, Category = "Pawn|Inventory")
	ASCItem* EquippedItem;

	UFUNCTION()
	virtual void OnRep_EquippedItem() {}

	// ~Inventory
	/////////////////////////////////////////////////////////////////////////////


	/////////////////////////////////////////////////////////////////////////////
	// Wiggle
public:
	UPROPERTY(BlueprintReadOnly, Category = "Wiggle")
	bool bWiggleGameActive;

	/////////////////////////////////////////////////////////////////////////////
	// Stance
public:
	UPROPERTY(ReplicatedUsing = OnRep_CurrentStance, BlueprintReadOnly, Category = "Pawn|Movement")
	ESCCharacterStance CurrentStance;

	/** For going back to the previous stance after leaving water or combat stance. */
	ESCCharacterStance PreviousStance;

	UFUNCTION()
	virtual void OnRep_CurrentStance();

	UFUNCTION(BlueprintImplementableEvent, Category = "Stance")
	void OnStanceChanged(ESCCharacterStance NewStance);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnCombatModeChanged(bool Enabled);

	/** @return true if player is in combat stance */
	FORCEINLINE bool InCombatStance() const { return (CurrentStance == ESCCharacterStance::Combat); }

	/** change the character's stance */
	virtual void SetStance(ESCCharacterStance NewStance);

	virtual void ToggleCombatStance();
	virtual void SetCombatStance(const bool bEnable);

	UFUNCTION(Client, Reliable)
	void CLIENT_SetCombatStance(const bool bEnable);

protected:
	/** [server] change the character's stance to be replicated */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetStance(ESCCharacterStance NewStance);

	UPROPERTY()
	float OnHitTimer;

	/////////////////////////////////////////////////////////////////////////////
	// Lock On
public:
	UPROPERTY(EditDefaultsOnly, Category = "Pawn|Sight")
	float MaxVisibilityDistance;

	UPROPERTY(EditDefaultsOnly, Category = "Pawn|Sight")
	float MaxNearbyDistance;

public:

	/**
	 * Check if this character has line of sight to another actor.
	 *
	 * @param	OtherActor - The actor we want to know if we can see.
	 * @param	MinFacingAngle - Minimum angle (1 to -1) to be considered facing.
	 * @param	allowFromMesh - check line of sight with this actor and its forward vector.
	 * @param	allowFromCamera - check line of sight with the camera and its forward vector.
	 */
	UFUNCTION(BlueprintPure, Category = "Pawn|Sight")
	bool LineOfSight(ASCCharacter* OtherActor, float MinFacingAngle, bool AllowFromMesh = true, bool AllowFromCamera = true);

	/** True if we have a lock on target */
	FORCEINLINE bool LockOnTarget() const { return LockOntoCharacter ? true : false; }

	/** Pointer to the lock onto character */
	FORCEINLINE ASCCharacter* GetLockedOnTarget() const { return LockOntoCharacter; }

	virtual float GetLockOnDistance() const { return LockOnDistance; }

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_OnCharacterInView(ASCCharacter* VisibleCharacter);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_OnCharacterInView(ASCCharacter* VisibleCharacter);

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	virtual void OnCharacterInView(ASCCharacter* VisibleCharacter) {}

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_OnCharacterOutOfView(ASCCharacter* VisibleCharacter);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_OnCharacterOutOfView(ASCCharacter* VisibleCharacter);

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	virtual void OnCharacterOutOfView(ASCCharacter* VisibleCharacter) {}

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_OnCharactersNearby(bool IsNearby, bool IsFriendlysNear);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_OnCharactersNearby(bool IsNearby, bool IsFriendlysNear);

	/** Is called when the amount of characters nearby a player changes.
	 * @param IsNearby - True if players are nearby.
	 * @param IsFriendlysNear - If any of the near actors are friendly, always false for Killer
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Pawn|Sight")
	void OnCharactersNearby(bool IsNearby, bool IsFriendlysNear);

	virtual bool IsVisible() const { return true; }

	bool IsCharacterVisible(const ASCCharacter* Character) const { return PlayerVisibilities.Contains(Character) ? PlayerVisibilities[Character] : false; }

protected:
	/** Checks the visibility of other characters in the game, calling SERVER_OnCharacterInView or SERVER_OnCharacterOutOfView for changes. Expensive! */
	virtual void UpdateCharacterVisibility();

	// Delay between calls to UpdateCharacterVisibility
	float VisibilityUpdateDelay;

	// Time until we call UpdateCharacterVisibility
	float TimeUntilVisibilityUpdate;

	/** If true, we can lock onto the nearest enemy when in combat stance */
	virtual bool CanUseHardLock() const { return CurrentStance == ESCCharacterStance::Combat && !IsStunned(); }

	/** If true, we can turn towards the nearest enemy when not in combat stance */
	virtual bool CanUseSoftLock() const { return CurrentStance != ESCCharacterStance::Combat && IsAttacking(); }

	/** Does dot product and distance checks on the character to see if they're in range for soft locking */
	virtual bool CanSoftLockOntoCharacter(const ASCCharacter* TargetCharacter) const;

	/** TEMP: Stub to expand out for when we have more than one killer or counselor v. counselor (also not a great name) */
	virtual bool IsOnSameTeamAs(const ASCCharacter* OtherCharacter) const { return OtherCharacter ? IsKiller() == OtherCharacter->IsKiller() : false; }

	/**
	 * Set character's lock on target.
	 * @param	Target The character to lock the camera on.
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_SetLockOnTarget(ASCCharacter* Target);

	// Distance (in cm) to allow locking on to a character
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	float LockOnDistance;

	// Soft lock distance (in cm) to apply a rotation when attacking
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	float SoftLockDistance;

	// Soft lock angle (in degrees) a chracter can be offset from us when we start attacking to rotate towards them
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	float SoftLockAngle;

	UPROPERTY(Replicated, Transient)
	ASCCharacter* LockOntoCharacter;

	UPROPERTY(BlueprintReadOnly, Transient)
	bool bPlayersNearby;

	UPROPERTY(BlueprintReadOnly, Transient)
	bool bFriendlysNearby;

	UPROPERTY()
	TMap<ASCCharacter*, bool> PlayerVisibilities;

	/////////////////////////////////////////////////////////////////////////////
	// Block
public:
	/** @return true if character is currently blocking */
	UFUNCTION(BlueprintPure, Category = "Combat")
	FORCEINLINE bool IsBlocking() const { return BlockData.bIsBlocking; }

	UFUNCTION()
	void OnStartBlock();

	UFUNCTION()
	void OnEndBlock();

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_OnBlock(bool ShouldBlock);

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Block, Transient)
	FSCBlockData BlockData;

	UFUNCTION()
	void OnRep_Block();

	/////////////////////////////////////////////////////////////////////////////
	// Indoors
public:
	/** @return true if this character is inside of a cabin. */
	UFUNCTION(BlueprintCallable, Category = "Indoors")
	bool IsIndoors() const;

	/** @return true if this character is in the same cabin(s) as OtherCharacter. */
	UFUNCTION(BlueprintCallable, Category = "Indoors")
	bool IsIndoorsWith(ASCCharacter* OtherCharacter) const;

	/** @return true if this character is inside of Cabin. */
	UFUNCTION(BlueprintCallable, Category = "Indoors")
	bool IsInsideCabin(ASCCabin* Cabin) const;

	/** Forces this character to be indoors. */
	UFUNCTION(BlueprintCallable, Category = "Indoors")
	void ForceIndoors(bool IsInside);

	/** Tells the server we were forced to be indoors. */
	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable, Category = "Indoors")
	void SERVER_ForceIndoors(bool IsInside);

	/** Blueprint implementable for indoor event changes. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Indoors")
	void OnIndoorsChanged(bool IsInside);

	// Current indoor components we are overlapping
	UPROPERTY(Transient)
	TArray<USCIndoorComponent*> OverlappingIndoorComponents;

	// Delegate fired when the inside status of this character changes
	FOnIndoorsChanged OnIsInsideChanged;

private:
	/** Replication handler for bIsInside. */
	UFUNCTION()
	void OnRep_IsInside();

	// Are we inside of any OverlappingIndoorComponents?
	UPROPERTY(Transient, ReplicatedUsing=OnRep_IsInside)
	bool bIsInside;

	/////////////////////////////////////////////////////////////////////////////
	// Special Moves/Context Kills
public:
	UFUNCTION(BlueprintPure, Category = "Context Kill")
	bool IsInContextKill() const { return bInContextKill; }

	/** Clears context kill (called from AnimInstance) */
	FORCEINLINE void ClearContextKill() { bInContextKill = false; bInvulnerable = false; }

protected:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Context Kill")
	bool bInContextKill;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation")
	bool bPlayedDeathAnimation;

	UPROPERTY(Replicated, Transient)
	bool bClientInSpecialMove;

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_FinishedSpecialMove(const bool bWasAborted = false);

	/** Called when a server (or owning client) decides to cancel a special move */
	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_AbortSpecialMove(TSubclassOf<USCSpecialMove_SoloMontage> MoveClass);

public:
	void BeginSpecialMoveWithCallback(TSubclassOf<class UILLSpecialMoveBase> SpecialMove, class FSCSpecialMoveCreationDelegate Callback);
	class UILLSpecialMoveBase* GetActiveSpecialMove() { return ActiveSpecialMove; }

	class USCSpecialMove_SoloMontage* GetActiveSCSpecialMove() const;

	/** Called on all clients from context kill components when a kill starts 
	 * BlueprintCallable for SP super cinematic kills */
	UFUNCTION(BlueprintCallable, Category="Context Kill")
	virtual void ContextKillStarted();

	/** Called on all clients from context kill components when a kill ends
	 * BlueprintCallable for SP super cinematic kills */
	UFUNCTION(BlueprintCallable, Category = "Context Kill")
	virtual void ContextKillEnded();

	UFUNCTION(exec)
	void HideHUD(bool bHideHUD);

	/** 
	 * Return the camera to the character
	 * @param BlendTime - The time it takes for the camera to return to its desired control rotation
	 * @param BlendFunc - Blend function used for easing
	 * @param BlendExp - Blend exponent
	 * @param LockOutgoing - Lock the location of the camera at the beginning of the blend
	 * @param UpdateControlRotation - Set the control rotation Yaw to match the previous camera's Yaw. (This makes for a smoother transition back to the player camera)
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void ReturnCameraToCharacter(const float BlendTime = 0.f, const EViewTargetBlendFunction BlendFunc = EViewTargetBlendFunction::VTBlend_Linear, const float BlendExp = 0.f, const bool bLockOutgoing = true, const bool bUpdateControlRotation = true);

	/** */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void TakeCameraControl(AActor* ViewFromActor, const float BlendTime = 0.f, const EViewTargetBlendFunction BlendFunc = EViewTargetBlendFunction::VTBlend_Linear, const float BlendExp = 0.f, const bool LockOutgoing = true);

	UFUNCTION(BlueprintCallable, Category = "Camera")
	void ResetCameras();

	/** */
	UFUNCTION(NetMulticast, Reliable)
	void EnablePawnCollision(bool bEnable);

	/** */
	UFUNCTION(BlueprintCallable, Category = "Collision")
	void MoveIgnoreActorChainAdd(AActor* ActorChild);

	/** */
	UFUNCTION(BlueprintCallable, Category = "Collision")
	void MoveIgnoreActorChainRemove(AActor* ActorChild);

protected:
	//
	TArray<AContextKillVolume*> AvailableContextKills;

	//
	AContextKillVolume* ActiveContextVolume;

	/////////////////////////////////////////////////////////////////////////////
	// Afterimage
public:
	TSubclassOf<ASCAfterimage> GetAfterImageClass();

protected:
	// The class used for creating shadows of this character
	UPROPERTY(EditDefaultsOnly, Category = "Character")
	TSubclassOf<ASCAfterimage> AfterImageClass;

	/////////////////////////////////////////////////////////////////////////////
	// UI / HUD
public:
	/** @return Character preview mesh to use for the UI. */
	UFUNCTION(BlueprintPure, Category = "Character|UI")
	static USkeletalMesh* GetCharacterPreviewMesh(TSoftClassPtr<ASCCharacter> CharacterClass)
	{
		if (!CharacterClass.IsNull())
		{
			CharacterClass.LoadSynchronous(); // TODO: Make Async
			return CharacterClass.Get()->GetDefaultObject<ASCCharacter>()->GetMesh()->SkeletalMesh;
		}
		
		return nullptr;
	}

	/** @return ThumbnailImage because BlueprintReadOnly does not work on UTexture2D. */
	UFUNCTION(BlueprintPure, Category = "Character|UI")
	static UTexture2D* GetCharacterThumbnailImage(TSoftClassPtr<ASCCharacter> CharacterClass)
	{
		if (!CharacterClass.IsNull())
		{
			CharacterClass.LoadSynchronous(); // TODO: Make Async
			return CharacterClass.Get()->GetDefaultObject<ASCCharacter>()->ThumbnailImage;
		}
		
		return nullptr;
	}

	/** @return LargeImage because BlueprintReadOnly does not work on UTexture2D. */
	UFUNCTION(BlueprintPure, Category = "Character|UI")
	static UTexture2D* GetCharacterLargeImage(TSoftClassPtr<ASCCharacter> CharacterClass)
	{
		if (!CharacterClass.IsNull())
		{
			CharacterClass.LoadSynchronous(); // TODO: Make Async
			return CharacterClass.Get()->GetDefaultObject<ASCCharacter>()->GetLargeImage();
		}
		return nullptr;
	}

	FORCEINLINE UTexture2D* GetLargeImage() const { return LargeImage.LoadSynchronous(); }
	FORCEINLINE const FText& GetCharacterName() const  { return CharacterName; }
	FORCEINLINE const FText& GetCharacterTropeName() const { return CharacterTropeName; }

protected:
	// Character thumbnail image
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	UTexture2D* ThumbnailImage;

	// Character Large image. This is temporary until we have models in for the UI.
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSoftObjectPtr<UTexture2D> LargeImage;

	// Character description for display in the UI
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "UI")
	FText CharacterName;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "UI")
	FText CharacterTropeName;

	/////////////////////////////////////////////////////////////////////////////
	// Look At Animation
public:
	/** Head look-at rotation swizled to be in head joint space */
	FORCEINLINE FRotator GetHeadLookAtRotation() const { return FRotator(0.f, HeadLookAtRotation.Yaw, HeadLookAtRotation.Pitch); }

	/** Updates the look-at rotation locally then notifies the server of the result. */
	virtual void UpdateLookAtRotation(float DeltaSeconds);
	
protected:
	/** Notifies the server of our new LookAtDirection to replicate to everyone else. */
	UFUNCTION(Server, Unreliable, WithValidation)
	void SERVER_SetHeadLookAtDirection(FVector_NetQuantizeNormal LookAtDirection);

private:
	// Head look-at direction
	UPROPERTY(Transient, Replicated)
	FVector_NetQuantizeNormal HeadLookAtDirection;

	// Cached off version of HeadLookAtDirection for locally-controlled, otherwise it is a smoothed version of the replicated HeadLookAtDirection
	FRotator HeadLookAtRotation;

	// Last HeadLookAtDirection sent
	FVector_NetQuantizeNormal LastSentHeadLookAtDirection;

	// Rate at which we send head look-at updates
	float LookAtTransmitDelay;

	// Time until we send the look-at rotation to the server
	float TimeUntilSendLookAtUpdate;

	/////////////////////////////////////////////////////////////////////////////
	// Requested Input
public:
	/** Returns the requested movement input in world space */
	FVector2D GetRequestedMoveInput() const;

	/////////////////////////////////////////////////////////////////////////////
	// Debug commands
protected:
	/** Called at the end of the tick */
	virtual void DrawDebug();

	/** Help us track down why we can't move, what we're locked onto, and why */
	virtual void DrawDebugStuck(float& ZPos);

private:
	void OnSkipCutscene();

	UFUNCTION(exec)
	void SpawnPolice();
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_SpawnPolice();

#if !UE_BUILD_SHIPPING
	/** Handles what we're debugging and why, THERE CAN ONLY BE ONE! */
	static FSCDebugSpawners SpawnerDebugger;
#endif

	UFUNCTION(Exec)
	void ShowHealthBar(bool bShow);

	UFUNCTION(Exec)
	void ShowRandomLevel(bool bShow);

	UPROPERTY()
	bool bShowRandomLevel;

	/////////////////////////////////////////////////////////////////////////////
	// Throw Functionality
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Throw")
	float ThrowStrength;

	UPROPERTY(ReplicatedUsing=OnRep_CurrentThrowable)
	class ASCThrowable* CurrentThrowable;

	UFUNCTION()
	virtual void OnRep_CurrentThrowable() {}

public:
	UFUNCTION()
	float GetThrowStrength() const { return ThrowStrength; }

	UFUNCTION(BlueprintCallable, Category = "Throw")
	bool IsPerformingThrow() const;

	UFUNCTION()
	virtual void BeginThrow(class ASCThrowable* ThrownItem) { CurrentThrowable = ThrownItem; }
	virtual void PerformThrow() {}
	virtual void EndThrow() {}
	
	/////////////////////////////////////////////////////////////////////////////
	// Trap interaction

	/** The trap we've activated */
	UPROPERTY(ReplicatedUsing=OnRep_TriggeredTrap)
	class ASCTrap* TriggeredTrap;

	UFUNCTION()
	void OnRep_TriggeredTrap();

	/** Name of socket attached to character left foot */
	UPROPERTY(EditDefaultsOnly, Category = "Trap")
	FName LeftFootSocketName;

	/** Name of socket attached to character right foot */
	UPROPERTY(EditDefaultsOnly, Category = "Trap")
	FName RightFootSocketName;

	/**
	 * Trigger trap and return if it was with our left (0) or right(1) foot
	 * @param InTriggeredTrap - The trap we've stepped on.
	 * @param FootLocation - Return location for which foot is caught.
	 * @return - (0) for left foot (1) for right foot (-1) if A SkeletalMeshComponent is not found.
	 */
	UFUNCTION()
	virtual int32 TriggerTrap(class ASCTrap* InTriggeredTrap, FVector& FootLocation);

	/** Called when we finish the break free game while in a trap */
	UFUNCTION()
	virtual void EscapeTrap(bool bPlayEscapeAnimation = true);

	/////////////////////////////////////////////////////////////////////////////
	// Music & VO Component
public:
	/** Music Component Instance */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Music")
	class USCMusicComponent* MusicComponent;

	/** Music Object Class Reference */
	UPROPERTY(EditDefaultsOnly, Category = "Music")
	TSubclassOf<class USCMusicObject> MusicObjectClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voice Over")
	class USCVoiceOverComponent* VoiceOverComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Voice Over")
	TSubclassOf<class USCVoiceOverObject> VoiceOverObjectClass;

	/////////////////////////////////////////////////////////////////////////////
	// Rain
protected:
	// Set based on the World Settings, should only be used internally
	bool bIsRaining;

	/** Called to update rain SFX based on World Settings (handles the rain callback) */
	UFUNCTION()
	void SetRainEnabled(bool bRainEnabled);

	/////////////////////////////////////////////////////////////////////////////
	// Post Process
public:
	APostProcessVolume* GetPostProcessVolume() { return ForcedPostProcessVolume; }

	void SpawnPostProcessVolume();

private:
	UPROPERTY()
	APostProcessVolume* ForcedPostProcessVolume;

	/////////////////////////////////////////////////////////////////////////////
	// Cinematography
public:
	UFUNCTION(Client, Reliable)
	void CLIENT_SetSkeletonToCinematicMode(bool bForce = false);

	UFUNCTION(Client, Reliable)
	void CLIENT_SetSkeletonToGameplayMode(bool bForce = false);

	UPROPERTY(EditDefaultsOnly, Category = "Skeleton")
	TArray<FName> ForcedAnimationBones;

	/////////////////////////////////////////////////////////////////////////////
	// Police Arrive
public:
	/** Plays the Police Arrived SFX when they show up. */
	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_PlayPoliceArriveSFX();

	// Sound Cue for the Police Arrived SFX.
	UPROPERTY(EditDefaultsOnly, Category = "Police")
	USoundCue* PoliceArriveCue;


	/////////////////////////////////////////////////////////////////////////////
	// RPCs for other actors

	// -- Doors
public:
	/**
	 * Sets bIsOpen on the passed in door to bOpen, will call OnRep_IsOpen on Door
	 * @param Door - Door we want to open or close
	 * @param bOpen - True for open, false for close
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_OpenDoor(ASCDoor* Door, const bool bOpen);

	/**
	 * Destroys the passed in door (if it hasn't been already)
	 * Sets lifespan and then calls MULTICAST_DestroyDoor so all clients see the VFX
	 * @param Door - Door to destroy
	 * @param Impulse - Impulse to apply when destroying the door
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_DestroyDoor(ASCDoor* Door, const float Impulse);

	/////////////////////////////////////////////////////////////////////////////
	// AI
public:
	/** Set this character's StartingWaypoint */
	UFUNCTION(BlueprintCallable, Category="AI")
	void SetStartingWaypoint(ASCWaypoint* Waypoint);

	/** Sets this character's OverrideNextWaypoint. This Waypoint will be used instead of their current Waypoint's NextWaypoint when BTTask_FindNextWaypoint is executed. */
	UFUNCTION(BlueprintCallable, Category="AI")
	void SetOverrideNextWaypoint(ASCWaypoint* Waypoint);

	/** @return This character's OverrideNextWaypoint */
	UFUNCTION(BlueprintPure, Category="AI")
	ASCWaypoint* GetOverrideNextWaypoint() const;

	// Properties for the AI controller
	UPROPERTY(EditAnywhere, Category="AI")
	TSubclassOf<USCCharacterAIProperties> AIProperties;

	// Behavior tree for the AI controller to use
	UPROPERTY(EditAnywhere, Category="AI")
	UBehaviorTree* BehaviorTree;

	// Waypoint the AI should start pathing from
	UPROPERTY(EditAnywhere, Category="AI")
	ASCWaypoint* StartingWaypoint;

	/////////////////////////////////////////////////////////////////////////////
	// Disable Character
public:
	/** @return true if this character is disabled. */
	FORCEINLINE bool IsCharacterDisabled() const { return bDisabled; }

	/** Change the disabled status of this character. */
	void SetCharacterDisabled(bool Disable);

protected:
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_SetCharacterDisabled(bool Disable);

	UPROPERTY(ReplicatedUsing = OnRep_Disabled)
	bool bDisabled;

	UFUNCTION()
	void OnRep_Disabled();
	// ~Disable Character
	/////////////////////////////////////////////////////////////////////////////
};
