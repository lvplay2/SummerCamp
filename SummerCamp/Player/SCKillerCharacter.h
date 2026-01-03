// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCCharacter.h"
#include "SCStengthOrWeaknessDataAsset.h"
#include "SCAnimDriverComponent.h"
#include "SCKillerCharacter.generated.h"

class ASCCounselorAIController;
class ASCCounselorCharacter;
class ASCDoor;
class ASCDynamicContextKill;
class ASCWeapon;
class ASCKillerMask;

class USCScoringEvent;
class USCVehicleSeatComponent;

#define GRAB_KILLS_MAX 4
#define ABILITY_UNLOCKING_LEVEL 150

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSCGrabbedCounselor, ASCCounselorCharacter*, Counselor);

/**
 * @struct FSenseReference
 */
USTRUCT()
struct FSenseReference
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	ASCCounselorCharacter* Counselor;

	UPROPERTY()
	class ASCCabin* Cabin;

	UPROPERTY()
	class ASCHidingSpot* HidingSpot;
};

/**
* @struct FPoliceStunInfo
*/
USTRUCT()
struct FPoliceStunInfo
{
	GENERATED_USTRUCT_BODY()
	
	FPoliceStunInfo()
	: DamageCauser(nullptr)
	, PoliceEscapeForwardDir(FVector::ZeroVector)
	, PoliceEscapeRadius(0.f)
	{

	}
	
	FPoliceStunInfo(USceneComponent* InDamageCauser, FVector InPoliceEscapeForward, float InPoliceEscapeRadius)
	: DamageCauser(InDamageCauser)
	, PoliceEscapeForwardDir(InPoliceEscapeForward)
	, PoliceEscapeRadius(InPoliceEscapeRadius)
	{

	}

	UPROPERTY()
	USceneComponent* DamageCauser;

	FVector PoliceEscapeForwardDir;

	float PoliceEscapeRadius;
};

/**
 * @class AKillerCharacter
 */
UCLASS()
class SUMMERCAMP_API ASCKillerCharacter
: public ASCCharacter
{
	GENERATED_BODY()

public:
	ASCKillerCharacter(const FObjectInitializer& ObjectInitializer);

	// Begin AActor interface
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	virtual void Destroyed() override;
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser) override;
	virtual void SetActorHiddenInGame(bool bNewHidden) override;
	// End AActor Interface

	// Begin AILLCharacter interface
	virtual void OnReceivedPlayerState() override;
	virtual void OnInteract(UILLInteractableComponent* Interactable, EILLInteractMethod InteractMethod) override;
	// End AILLCharacter interface

	// Begin ASCCharacter interface
	virtual void OnInteract1Pressed() override;
	virtual void OnClientLockedInteractable(UILLInteractableComponent* LockingInteractable, const EILLInteractMethod InteractMethod, const bool bSuccess) override;
	virtual void ClearAnyInteractions() override;
	virtual void ContextKillStarted() override;
	virtual void ContextKillEnded() override;
	virtual bool IsVisible() const override;
	virtual void BreakGrab(bool StunWithPocketKnife = false) override;
	virtual void OnAttack() override;
	virtual void OnStopAttack() override;
	virtual float GetSpeedModifier() const override;
	virtual void SetSprinting(bool bSprinting) override;
	virtual bool IsKiller() const override { return true; }
	virtual bool IsValidStunArea(AActor* Attacker = nullptr) override;
	virtual void MULTICAST_BeginStun_Implementation(UAnimMontage* StunMontage, TSubclassOf<USCStunDamageType> StunClass, const FSCStunData& StunData, bool bIsValidStunArea) override;
	virtual void MULTICAST_EndStun_Implementation() override;
	virtual bool HasItemInInventory(TSubclassOf<ASCItem> ItemClass) const override;
	virtual int32 NumberOfItemTypeInInventory(TSubclassOf<ASCItem> ItemClass) const override;
	virtual void ForceKillCharacter(ASCCharacter* Killer, FDamageEvent DamageEvent = FDamageEvent()) override;
	virtual void OnReceivedController() override;
	virtual void DrawDebug() override;
	virtual void DrawDebugStuck(float& ZPos) override;
	virtual void Suicide() override;
	virtual bool CanTakeDamage() const override;
	virtual void OnRep_CurrentThrowable() override;
	virtual void PerformThrow() override;
	virtual void EndThrow() override;
	virtual void MULTICAST_DamageTaken_Implementation(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser) override;
	virtual void ToggleCombatStance() override;
	// End ASCCharacter interface

	/** 
	 * Toggle showing the map
	 * @param ShowMap - Enable display of the map
	 * @param IsMorphMap - Should the map show the cursor and lock control rotation
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Default")
	void ToggleMap(bool bShowMap, bool bIsMorphMap);

	/** Handles toggling input control on and off and calls to the blueprint event to handle effects */
	void ToggleMap_Native(bool bShowMap, bool bIsMorphMap);

	/////////////////////////////////////////////////////////////////////////////
	// Grab Kill System

	/** Size of the killer's capsule when grabbing a counselor */
	UPROPERTY(EditDefaultsOnly, Category = "Context Action|Grab")
	float GrabbedCapsuleSize;

	float DefaultCapsuleSize;

	/** Offset on the X axis when grabbing a counselor */
	UPROPERTY(EditDefaultsOnly, Category = "Context Action|Grab")
	float GrabbedMeshOffset;

	/** The list of Grab Context kills we can perform */
	UPROPERTY(EditDefaultsOnly, Category = "Context Action")
	TSubclassOf<ASCDynamicContextKill> GrabKillClasses[GRAB_KILLS_MAX];

	/** List of spawned Grab kill actors */
	UPROPERTY(Replicated)
	ASCDynamicContextKill* GrabKills[GRAB_KILLS_MAX];

	UFUNCTION()
	bool CanGrabKill() const;

	/** 
	 * Test if a Grab kill can be performed
	 * @param RequestedKill - The index for the kill we're looking for (0 - GRAB_KILLS_MAX)
	 * @return - True if Kill volume is unobstructed
	 */
	UFUNCTION(BlueprintPure, Category = "Context Action|Grab")
	bool IsGrabKillAvailable(int32 RequestedKill) const;

	/*
	 * Get Context Kill Icon for selected grab kill
	 * @param RequestedKill - The index for the kill we're looking for (0 - GRAB_KILLS_MAX)
	 * @param HUDIcon - Do we want to return the HUD icon or the Menu Icon for the context kill
	 * @return - Texture2D context kill Icon, null if requestedKill is out of bounds
	 */
	UFUNCTION(BlueprintPure, Category = "Context Action|Grab")
	UTexture2D* GetGrabKillIcon(int32 RequestedKill, bool HUDIcon = true);

	UPROPERTY(Replicated, Transient, BlueprintReadOnly, Category = "Context Action")
	bool bGrabKillsActive;

	UFUNCTION()
	void UseTopGrabKill();

	UFUNCTION()
	void UseRightGrabKill();

	UFUNCTION()
	void UseBottomGrabKill();

	UFUNCTION()
	void UseLeftGrabKill();

	void PerformGrabKill(uint32 Kill);

	UFUNCTION(BlueprintPure, Category = "Default")
	USkeletalMeshComponent* GetKillerMaskMesh() const { return JasonSkeletalMask; }

	/** Music that plays when Jason is nearby. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Music")
	USoundBase* NearCounselorMusic;

	/** The override music provided from a skin. */
	UPROPERTY(Replicated, Transient)
	USoundBase* NearCounselorOverrideMusic;

	/** How far away Jason needs to be for his Music to start playing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Counselor Music")
	float NearCounselorMusicDistance;

	/** How far away Jason needs to be for his Music to be at a 1.0 volume. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Counselor Music")
	float NearCounselorMusicMaxVolumeDistance;

	/** Returns the music to play on counselors when Jason is near them. */
	UFUNCTION(BlueprintPure, Category = "Music")
	USoundBase* GetNearJasonMusic() const; 

	UFUNCTION(BlueprintPure, Category = "Grab")
	float GetGrabModifier() const { return GrabModifier; }

protected:
	/** A float mod that adjusts the difficulty of the jasons grab **/
	UPROPERTY(EditDefaultsOnly, Category = "Grab")
	float GrabModifier;

	// Audio to play when a counselor is grabbed
	UPROPERTY(EditDefaultsOnly, Category = "Grab")
	USoundCue* GrabbedSound;

	// ~Grab Kill System
	/////////////////////////////////////////////////////////////////////////////

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USkeletalMeshComponent* JasonSkeletalMask;

	// Class reference for the mask pickup
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ASCKillerMask> KillerMaskPickupClass;

	void SetKillerOpacity(float NewOpacity);

protected:
	void SetOpacity(USkeletalMeshComponent* Mesh, float Opacity);

	/////////////////////////////////////////////////////////////////////////////
	// Abilities
public:
	/** The amount of time before each ability is unlocked. */
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	float AbilityUnlockTime[(uint8)EKillerAbilities::MAX];

	/** The amount of time it takes for an ability to recharge after it is unlocked. */
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	float AbilityRechargeTime[(uint8)EKillerAbilities::MAX];
	
	/** Returns what index in the order that param AbilityType is unlocked. */
	uint8 GetPositionInUnlockOrder(EKillerAbilities AbilityType) const;
	
	/** Returns what ability is unlocked at param Index in the custom unlock order. 0-based */
	FORCEINLINE EKillerAbilities GetAbilityAtPosition(uint8 Index) const { return UserDefinedAbilityOrder[Index]; }

	/** Returns the current percentage of time before ability is recharged. 0-1 range. */
	UFUNCTION(BlueprintPure, Category = "Abilities")
	float GetCurrentRechargeTimePercent(EKillerAbilities Ability) const;

	/** Returns if the ability is unlocked and fully charged. */
	UFUNCTION(BlueprintPure, Category = "Abilities")
	bool IsAbilityAvailable(EKillerAbilities Ability) const;

	/** Returns if the ability is unlocked and allowed to recharge. */
	UFUNCTION(BlueprintPure, Category = "Abilities")
	bool IsAbilityUnlocked(EKillerAbilities Ability) const { return (AbilityUnlocked[static_cast<uint8>(Ability)] != 0); }

	/** Returns the current percentage of time befor the ability is unlocked. 0-1 range. */
	UFUNCTION(BlueprintPure, Category = "Abilities")
	float GetAbilityUnlockPercent(EKillerAbilities Ability) const;

	UFUNCTION(BlueprintPure, Category = "Abilities")
	bool IsAbilityActive(EKillerAbilities Ability) const;

protected:
	/* If an ability is unlocked and can recharge. */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_AbilityUnlocked)
	uint8 AbilityUnlocked[(uint8) EKillerAbilities::MAX];

	UPROPERTY()
	uint8 OldUnlocked[(uint8) EKillerAbilities::MAX];

	UFUNCTION()
	virtual void OnRep_AbilityUnlocked();

	/* The timer for the current ability unlock. */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = "Abilities")
	FFloat_NetQuantize AbilityUnlockTimer;

	/* The current timers for the ability recharges. */
	UPROPERTY(Transient, Replicated)
	FFloat_NetQuantize AbilityRechargeTimer[(uint8) EKillerAbilities::MAX];

	/* Used to update the ability's unlock and recharge timers. */
	void UpdateAbilities(float DeltaSeconds);

	/** Loads the default ability unlocking order. Clears out existing order before loading in default values. */
	void LoadDefaultAbilityUnlockOrder();

	// The player's ability unlock order. 0 index is unlocked first, EKillerAbilities::MAX-1 is last to unlock. 
	TArray<EKillerAbilities> UserDefinedAbilityOrder;


	// ~Abilities
	/////////////////////////////////////////////////////////////////////////////

protected:
	/** Blueprint damage type to stun jason when grab is broken out of with a pocket knife. */
	UPROPERTY(EditDefaultsOnly, Category = "Grab")
	TSubclassOf<USCStunDamageType> BreakGrabPocketKnifeJasonStunClass;

	/** Blueprint damage type to stun jason when grab is broken out of without a pocket knife. */
	UPROPERTY(EditDefaultsOnly, Category = "Grab")
	TSubclassOf<USCStunDamageType> BreakGrabJasonStunClass;

	UPROPERTY(Transient)
	bool bPocketKnifed;

	/////////////////////////////////////////////////////////////////////////////
	// Pamela's Sweater
protected:

	/** Is Jason currently stunned by the sweater */
	bool bStunnedBySweater;

	/** Stun data for Pamela's sweater */
	UPROPERTY(EditDefaultsOnly, Category = "Pamela's Sweater")
	TSubclassOf<USCStunDamageType> PamelasSweaterStun;

	/** The blueprint class for the jason kill object */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class ASCContextKillActor> JasonKillClass;

	UPROPERTY()
	ASCContextKillActor* JasonKillObject;

	/** Montage when jason enters the downed kill stance */
	UPROPERTY(EditDefaultsOnly)
	UAnimMontage* KillStanceMontage;

	UFUNCTION(BlueprintPure, Category = "Customize")
	TSubclassOf<class ASCContextKillActor> GetJasonKillClass() const;

public:
	/** The killer is stunned by Pamela's sweater */
	void StunnedBySweater(ASCCounselorCharacter* SweaterInstigator);

	/** Return if Jason is currently stunned by the sweater */
	FORCEINLINE bool IsStunnedBySweater() const { return bStunnedBySweater; }

	/** Check if the kill we're trying to active belongs to this killer. */
	FORCEINLINE bool HasActiveKill(class ASCContextKillActor* InContextKill) { return InContextKill == JasonKillObject ; }

	/** Is this killer in the kill stance */
	FORCEINLINE bool IsInKillStance() { return JasonKillObject != nullptr; }

	/** Console command to force Jason into the Kill stance */
	UFUNCTION(Exec)
	void EnterKillStance();

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_EnterKillStance();

	// ~Pamela's Sweater
	/////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////
	// Combat
public:

	/** Chance of dismembering a counselor when attacking them in combat */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = 0, UIMin = 0, ClampMax = 100, UIMax = 100))
	int CombatDismemberChance;

	/** Multiplier for damage when blocking */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	float BlockedHitDamageModifier;

	/** Multiplier for damage when stunned */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	float StunDamageModifier;

	/** How much to scale the killers damage when applied to other characters */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	float DamageCharacterModifier;

	/** How much to scale the killers damage when applied to world objects */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	float DamageWorldModifier;

	/** How much time before the killer can attack again. Timer begins at the START of an attack. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float AttackDelay;

private:
	float CurrentAttackDelay;

	// ~Combat
	/////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////
	// Stun
public:

	/** The size of the box area behind Jason to check if we can do a fall back stun.
	 * Use sc.DebugFallStunArea 1 to view the size of the area
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Stun")
	FVector StunCheckAreaSize;

	// ~Stun
	/////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////
	// Sense
public:
	UPROPERTY(EditDefaultsOnly, Category = "Sense")
	float SenseDuration;

	/* The curve to use for the glow intensity for sense shadows based on distance. */
	UPROPERTY(EditDefaultsOnly, Category = "Sense")
	UCurveFloat* SenseIntensityCurve;

	/* The curve to use for scaling the sense shadows based on distance. */
	UPROPERTY(EditDefaultsOnly, Category = "Sense")
	UCurveFloat* SenseScaleCurve;

	/* The curve to use for scaling the sense radius based on fear. */
	UPROPERTY(EditDefaultsOnly, Category = "Sense")
	UCurveFloat* SenseFearRadiusCurve;

	/* The curve to used to determine the additional follow time based on fear. */
	UPROPERTY(EditDefaultsOnly, Category = "Sense")
	UCurveFloat* SenseFearHidingRadiusCurve;

	/* The colorgrade to use when in sense mode. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense")
	UTexture* SenseColorGrade;

	UPROPERTY(EditDefaultsOnly, Category = "Sense")
	UMaterial* SenseCounselorMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Sense")
	UMaterial* SenseCabinMaterial;

	void OnSenseActive();

	UFUNCTION(Reliable, Server, WithValidation)
	void SERVER_OnSenseActive();

	UFUNCTION()
	void CancelSense();

	UFUNCTION(Reliable, Server, WithValidation)
	void SERVER_CancelSense();

	UFUNCTION(BlueprintImplementableEvent, Category = "Sense")
	void OnSenseChanged(bool bActive);

protected:
	/* Is true when the sense ability is active. */
	UPROPERTY(BlueprintReadOnly, Transient, ReplicatedUsing = OnRep_SenseActive, Category = "Sense")
	bool bIsSenseActive;

	/* The timer for how long sense is active. */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = "Sense")
	FFloat_NetQuantize ActiveSenseTimer;

	UPROPERTY()
	UMaterialInstanceDynamic* SenseCounselorMaterialInstance;

	UPROPERTY()
	UMaterialInstanceDynamic* SenseCabinMaterialInstance;

	UPROPERTY()
	TArray<FSenseReference> SenseReferences;

	void UpdateSense(float DeltaSeconds);

	void StartSense();
	void EndSense();

	UFUNCTION()
	void OnRep_SenseActive();

	// ~Sense
	/////////////////////////////////////////////////////////////////////////////


	/////////////////////////////////////////////////////////////////////////////
	// Rage
public:

	UPROPERTY(EditDefaultsOnly, Category = "Rage")
	float RageUnlockTime;
	/* The speed increase ability charging should have while rage is active. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rage")
	float RagedRechargeTimeModifier;

	/* The distance increase sense should have when rage is active. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rage")
	float RagedSenseDistanceModifier;

	UPROPERTY(EditDefaultsOnly, Category = "Rage")
	UCurveFloat* RageSenseDistanceCurve;

	bool IsRageActive() const;

	UFUNCTION(BlueprintPure, Category = "Rage")
	float GetRageUnlockPercentage() const { return RageUnlockTimer / RageUnlockTime; }

protected:
	UPROPERTY(Replicated, Transient)
	FFloat_NetQuantize RageUnlockTimer;

	UPROPERTY(BlueprintReadOnly, Transient, ReplicatedUsing=OnRep_Rage, Category = "Rage")
	bool bIsRageActive;

	UFUNCTION()
	void OnRep_Rage();

	// ~Rage
	/////////////////////////////////////////////////////////////////////////////

protected:
	/** Weapon class */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSoftClassPtr<ASCWeapon> WeaponClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TArray<TSoftClassPtr<ASCWeapon>> AllowedWeaponClasses;

	/** Is our weapon bloody */
	bool bIsWeaponBloody;
public:
	FORCEINLINE TSoftClassPtr<ASCWeapon> GetDefaultWeaponClass() const { return WeaponClass; }

	FORCEINLINE const TArray<TSoftClassPtr<ASCWeapon>>& GetAllowedWeaponClasses() const {return AllowedWeaponClasses;}

	/** Is our weapon bloody? */
	FORCEINLINE bool IsWeaponBloody() const { return bIsWeaponBloody; }

	/** Apply blood to weapon */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ApplyBloodToWeapon(int32 MaterialIndex, FName MaterialParameterName, float MaterialParameterValue);

	/** Apply blood to weapon on clients */
	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_ApplyBloodToWeapon(int32 MaterialIndex, FName MaterialParameterName, float MaterialParameterValue);

protected:
	/** Default amount of damage to deal when swinging your weapon */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float MaskPopOffThreshold;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float MaskPopOffHitThreshold;

	/** Default amount of damage to deal when swinging your weapon */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float MurderHealthBonus;

	/** The minimum amount a killers health can drop to while acruing regular damage. **/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float MinimumHealth;

	/** List of Counselor PlayerStates that have damaged us */
	UPROPERTY()
	TArray<class APlayerState*> DamageCausers;

public: // FUCK. THE. LAW.
	UFUNCTION()
	virtual void CancelShift();

	UFUNCTION()
	virtual void ForceCancelShift();

	UFUNCTION()
	virtual void CancelMorph();

	UFUNCTION(BlueprintCallable, Category = "Sense")
	void CancelScreenEffectAbilities();
		
protected:

	UPROPERTY(BlueprintReadOnly, Transient, ReplicatedUsing = OnRep_MaskOn, Category = "Default")
	bool bMaskOn;

	UFUNCTION()
	void OnRep_MaskOn();

	UFUNCTION(BlueprintImplementableEvent, Category = "Mask")
	void OnMaskOnChanged(bool MaskOn);

	FORCEINLINE void SetMaskOn(bool MaskOn) { bMaskOn = MaskOn; OnRep_MaskOn(); }

	UPROPERTY(EditDefaultsOnly, Category = "Mask|Stun")
	TSubclassOf<USCStunDamageType> MaskKnockedOffStun;

	/////////////////////////////////////////////////////////////////////////////
	// Shift
public:
	FORCEINLINE bool IsShiftActive() const { return bIsShiftActive; }
	FORCEINLINE float GetShiftSpeed() const { return ShiftSpeed; }

	UFUNCTION(BlueprintImplementableEvent, Category = "Bloodlust")
	void OnBlink(bool Enabled);

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_OnTeleportEffect(bool Enable);

	/** Update our current voyeur status */
	UFUNCTION(BlueprintCallable, Category = "Single Player")
	void SetVoyeurMode(bool InVoyeur);

	/** return if we're in voyeur mode */
	UFUNCTION(BlueprintCallable, Category = "Single Player")
	bool IsInVoyeurMode() const { return bInVoyeurMode; }

protected:

	// Are we currently in voyeur mode staring at some unsuspecting teens?
	UPROPERTY()
	bool bInVoyeurMode;
	
	/* Is true when the sense ability is active. */
	UPROPERTY(BlueprintReadOnly, Transient, ReplicatedUsing = OnRep_ShiftActive, Category = "Shift")
	bool bIsShiftActive;

	UFUNCTION(Reliable, Server, WithValidation)
	void SERVER_SetShift(bool bShift);

	UPROPERTY(EditDefaultsOnly, Category = "Shift")
	float ShiftSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Shift")
	float ShiftTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shift")
	UMaterial* ShiftScreenEffect;

	UPROPERTY()
	UMaterialInstanceDynamic* ShiftScreenEffectInst;

	float CurrentShiftTime;
	float ShiftWarmUpTime;

	UFUNCTION()
	void OnRep_ShiftActive();

	FTimerHandle TimerHandle_ShiftMusicPitchDelayTime;

	UPROPERTY()
	class UInputComponent* ShiftInputComponent;

	UPROPERTY(ReplicatedUsing=OnRep_Teleport, Transient)
	bool EnableTeleportEffect;

	float TeleportEffectAmount;

	UFUNCTION()
	void OnRep_Teleport();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Teleport")
	UMaterial* TeleportScreenEffect;

	UPROPERTY()
	UMaterialInstanceDynamic* TeleportScreenEffectInst;

	UPROPERTY(EditDefaultsOnly, Category = "Teleport")
	UCurveFloat* TeleportJitterCurve;

	UPROPERTY(EditDefaultsOnly, Category = "Teleport")
	float TeleportScreenTime;

	float TeleportScreenTimer;

	UPROPERTY()
	bool bHasTeleportEffectActive;

	FTransform ClosestPoint;
	FVector MorphLocation;
	FRotator MorphRotation;

	UPROPERTY(EditDefaultsOnly, Category = "Teleport")
	float MaxTeleportViewDistance;
	// ~Shift
	/////////////////////////////////////////////////////////////////////////////

	UFUNCTION()
	void PlayDistortionEffect();
	/////////////////////////////////////////////////////////////////////////////
	// Morphing
public:

	/** Teleport Character to new morph position */
	UFUNCTION(BlueprintCallable, Category = "Transform")
	virtual bool Morph(FVector NewLocation);

	UFUNCTION(Reliable, Server, WithValidation)
	void SERVER_OnMorph(FVector_NetQuantize NewLocation, FVector_NetQuantizeNormal NewRotation);

	void OnMorphEnded();

	UFUNCTION(Reliable, Client)
	void CLIENT_OnMorphEnded();

	void OnAttemptMorph();

	virtual void MorphCursorUp(float Val);
	virtual void MorphCursorRight(float Val);

	FORCEINLINE bool IsMorphing() const { return bIsMorphActive; }

	virtual ASCCounselorCharacter* GetLocalCounselor() const;

protected:
	UPROPERTY(BlueprintReadWrite, Category = "Morph")
	bool bShowMorphMap;

	UPROPERTY(BlueprintReadOnly, Category = "Morph")
	bool bActivateMorph;

	/* Is true when the sense ability is active. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Morph")
	bool bIsMorphActive;

	FTimerHandle TimerHandle_Teleport;

	// ~Morphing
	/////////////////////////////////////////////////////////////////////////////

	UFUNCTION()
	void OnToggleFastWalk();

	UFUNCTION()
	void OnStartFastWalk();

	UFUNCTION()
	void OnStopFastWalk();

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	bool bIsMapEnabled;

	void OnMapToggle();

	/////////////////////////////////////////////////////////////////////////////
	// Grabbing
public:
	UFUNCTION(BlueprintPure, Category = "Grab")
	ASCCounselorCharacter* GetGrabbedCounselor() const;

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_GrabSuccess(ASCCounselorCharacter* CounselorToGrab);

	// Server will call this if the grab failed to make sure clients don't remain in a grab state.
	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_GrabFailed();

	UPROPERTY(BlueprintReadWrite, Category = "Grab")
	float GrabValue;

	/** If true, we are moving the killer and counselor into position to line everything up for grabbing */
	UPROPERTY(BlueprintReadOnly, Category = "Grab")
	bool bIsPickingUpCounselor;

	// ~Grabbing
	/////////////////////////////////////////////////////////////////////////////

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	UMaterial* AOELightScreenEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	float AOELightRadius;

private:

	UPROPERTY()
	UMaterialInstanceDynamic* AOELightScreenEffectInst;

protected:

	/** [server] Perform non context grab kill kill */
	void PerformGrabKill();

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_PerformGrabKill();

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_PerformGrabKill(int32 SelectedGrabKill);

	// Character that is held by the killer
	UPROPERTY(Transient, ReplicatedUsing=OnRep_GrabbedCounselor)
	ASCCounselorCharacter* GrabbedCounselor;

	// Called when GrabbedCounselor is set to a counselor (also called when the grabbed counselor is cleared)
	UPROPERTY(BlueprintAssignable)
	FSCGrabbedCounselor OnGrabbedCounselor;

	UFUNCTION()
	void OnRep_GrabbedCounselor();

	// Maximum distance from grab target
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grab")
	float MaxGrabDistance;

	// Min distance to ignore angle and just grab the character (still does half-space check -- we're not allowing 360 noscopes)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grab")
	float MinGrabDistance;

	// Minimum angle Jason can be facing from his grab target
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grab")
	float MinGrabAngle;

	/////////////////////////////////////////////////////////////////////////////
	// Bloodlust
public:
	FORCEINLINE bool IsPowersEnabled() const { return bEnabledPowers; }

protected:

	/** if bBloodlustActive, perform bloodlust01 action */
	void ActivateBloodlust01();

	/** if bBloodlustActive, perform bloodlust02 action */
	void ActivateBloodlust02();

	/** if bBloodlustActive, perform bloodlust03 action */
	void ActivateBloodlust03();

	/** if bBloodlustActive, perform bloodlust04 action */
	void ActivateBloodlust04();

	void OnEnablePowersPressed();
	void OnEnablePowersReleased();

	UPROPERTY(BlueprintReadOnly, Category = "Powers")
	bool bEnabledPowers;

	UPROPERTY()
	class UInputComponent* AbilitiesInputComponent;

	UPROPERTY()
	class UInputComponent* MorphInputComponent;

	UPROPERTY()
	class UInputComponent* GrabInputComponent;


	// ~Bloodlust
	/////////////////////////////////////////////////////////////////////////////

public:
	UFUNCTION(NetMulticast, Reliable)
	void Clients_KilledCounselor();


	/////////////////////////////////////////////////////////////////////////////
	// Stalking
public:
	bool CanGrab() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grab")
	bool IsCharacterInGrabRange() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stalk")
	float CombatFOV;

	// ~Stalking
	/////////////////////////////////////////////////////////////////////////////

	/** Determine if there is a counselor in range to grab and update the animation accordingly */
	UFUNCTION(BlueprintCallable, Category = "Grab")
	void GrabCounselor(ASCCounselorCharacter* CounselorOverride = nullptr);

	/** Grab a counselor and get them lined up with Jason */
	UFUNCTION()
	void FinishGrabCounselor(ASCCounselorCharacter* Counselor);

	FTimerHandle TimerHandle_GrabKill;

	/** Start the grab wind up */
	UFUNCTION()
	void BeginGrab();

	/** Make sure we call the animation on all clients */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_BeginGrab();

	/** Make sure we call the animation on all clients */
	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_BeginGrab();

	UFUNCTION(BlueprintImplementableEvent, Category = "Grab")
	void BP_BeginGrab();

	/** If we begin a grab kill we want to enlarge the capsule and move it forward, otherwise shrink and move it back */
	UFUNCTION()
	void UpdateGrabCapsule(bool BeginGrab);

	/** Stack overflow protection */
	bool bIsUpdatingGrabCapsule;

	UFUNCTION(BlueprintCallable, Category = "Grab")
	void DropCounselor(bool bMoveCapsule = true);

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_DropCounselor(bool bMoveCapsule = true);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_DropCounselor(bool bMoveCapsule = true);

	UFUNCTION(BlueprintPure, Category = "Grab")
	bool IsGrabKilling() const;

	UFUNCTION(BlueprintPure, Category = "Grab")
	bool IsAttemptingGrab() const;

	UPROPERTY()
	USCContextKillComponent* CurrentVehicleGrabKillComp;

	void GrabCounselorOutOfVehicle(ASCCounselorCharacter* Counselor, USCContextKillComponent* KillComp);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_GrabCounselorOutOfVehicle(ASCCounselorCharacter* Counselor, USCContextKillComponent* KillComp);

	UFUNCTION()
	void OnGrabCounselorOutOfVehicleStarted(ASCCharacter* Interactor, ASCCharacter* Victim);

	UFUNCTION()
	void OnGrabCounselorOutOfVehicleComplete(ASCCharacter* Interactor, ASCCharacter* Victim);

	UFUNCTION()
	void OnGrabCounselorOutOfVehicleAborted(ASCCharacter* Interactor, ASCCharacter* Victim);

	void KillCounselorInBoat(ASCCounselorCharacter* Counselor, USCContextKillComponent* KillComp);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_KillCounselorInBoat(ASCCounselorCharacter* Counselor, USCContextKillComponent* KillComp);

	UFUNCTION()
	void OnKillCounselorInBoatComplete(ASCCharacter* Interactor, ASCCharacter* Victim);

	virtual bool CanAttack() const override;

	virtual void SERVER_Attack_Implementation(bool HeavyAttack = false) override;

	/////////////////////////////////////////////////////////////////////////////
	// Combat Stance
protected:

	virtual void SetCombatStance(const bool bEnable) override;

	/** Jason needs to query the server before entering combat stance */
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void SERVER_SetCombatStance(const bool bEnable);

	virtual void ProcessWeaponHit(const FHitResult& Hit) override;

	// ~Combat Stance
	/////////////////////////////////////////////////////////////////////////////


	/////////////////////////////////////////////////////////////////////////////
	// Underwater
public:

	// Block all non water related input
	UPROPERTY()
	class UInputComponent* UnderwaterInputComponent;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_UnderWater, BlueprintReadOnly, Category = "Swimming")
	bool bUnderWater;

	UFUNCTION()
	void OnRep_UnderWater();

	/** Called when bUnderwater is changed */
	UFUNCTION(BlueprintImplementableEvent, Category = "Swimming")
	void OnUnderWater(bool UnderWater);

	/** Tick if we're under water */
	UFUNCTION(BlueprintImplementableEvent, Category = "Swimming")
	void OnUnderWaterUpdate();

	/** Update underwater root for interaction and visuals */
	UFUNCTION()
	void NativeOnUnderwaterUpdate();

	/** Get the height of the water volume we're inside. */
	UFUNCTION(BlueprintPure, Category = "Swimming")
	float GetWaterZ() const { return CurrentWaterVolume == nullptr ? 0.f : CurrentWaterVolume->GetActorLocation().Z + CurrentWaterVolume->GetSimpleCollisionHalfHeight(); }

	/** Return max speed while underwater */
	UFUNCTION()
	float GetUnderwaterSpeed() { return UnderwaterSpeed; }

	/** Return depth ratio current depth / Underwater full speed depth */
	UFUNCTION()
	float GetUnderwaterDepthModifier();

	UPROPERTY(EditDefaultsOnly, Category = "Swimming")
	TSubclassOf<ASCDynamicContextKill> UnderwaterContextKillClass;

	UPROPERTY()
	ASCDynamicContextKill* UnderwaterContextKill;

	// Max speed under water
	UPROPERTY(EditDefaultsOnly, Category = "Swimming")
	float UnderwaterSpeed;

	// Depth where we'll be at full underwater speed, calculated based on character feet to the top of the water volume
	UPROPERTY(EditDefaultsOnly, Category = "Swimming")
	float UnderwaterFullSpeedDepth;

	// Depth required to preform the underwater kill
	UPROPERTY(EditDefaultsOnly, Category = "Swimming")
	float UnderwaterKillDepth;

	// Blueprint class to spawn as the visual while jason is underwater
	UPROPERTY(EditDefaultsOnly, Category = "Swimming")
	TSubclassOf<AActor> UnderwaterVisualClass;

	// Pointer to our spawned visual class to enable/disable it
	UPROPERTY()
	AActor* UnderwaterVisuals;

	// The visual root is placed at water level and our visual class is attached to it
	UPROPERTY(BlueprintReadOnly, Category = "Swimming")
	USceneComponent* UnderwaterVisualRoot;

	// Interact component to activate overlapped counselor or context kill
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Swimming|Interaction")
	USCInteractComponent* UnderwaterInteractComponent;

	UFUNCTION()
	int32 CanInteractUnderwater(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation);

	UFUNCTION()
	void OnInteractUnderwater(AActor* Interactor, EILLInteractMethod InteractMethod);

	void CheckForDrownableCounselors(TArray<ASCCounselorCharacter*>& OutDrownableCounselors);

private:
	// Material for the underwater mesh so we can adjust the intensity
	UPROPERTY()
	UMaterialInstanceDynamic* UnderwaterVisualsMaterial;

	// ~Underwater
	/////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////
	// Sound Blips
public:
	/* The actor class that spawns the sound blip. */
	UPROPERTY(EditDefaultsOnly, Category = "SoundBlip")
	TSubclassOf<AActor> SoundBlipClass;

	/* The minimum amount of time to attempt to find a counselor making noise. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SoundBlip")
	float NoiseDetectionMinTime;

	/* The maximum amount of time to attempt to find a counselor making noise. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SoundBlip")
	float NoiseDetectionMaxTime;

	/* The minimum amount of sound that has to be made to be detected. */
	UPROPERTY(EditDefaultsOnly, Category = "SoundBlip")
	float NoiseLevelThreshold;

	/* The maximum amount of distance to scale the noise level by. */
	UPROPERTY(EditDefaultsOnly, Category = "SoundBlip")
	float NoiseDistanceThreshold;

	/* The maximum offset to offset the sound blip by. */
	UPROPERTY(EditDefaultsOnly, Category = "SoundBlip")
	float SoundBlipMaxOffsetRadius;

	/* The amount of time before loosing a lock on to a noise maker. */
	UPROPERTY(EditDefaultsOnly, Category = "SoundBlip")
	float SoundLockonTime;

	/* The minimum frequency a blip should be shown. */
	UPROPERTY(EditDefaultsOnly, Category = "SoundBlip")
	float MinBlipFrequencyTime;

	/* The maximum frequency a blip should be shown. */
	UPROPERTY(EditDefaultsOnly, Category = "SoundBlip")
	float MaxBlipFrequencyTime;

	/** Anthing closer than this radius will be ignored. */
	UPROPERTY(EditDefaultsOnly, Category = "SoundBlip")
	float MinBlipRadius;

	/** If true will allow sound blips to appear, if false all active blips will be destroyed and no new blips will appear until it is re-enabled */
	UFUNCTION(BlueprintCallable, Category = "SoundBlip")
	void SetSoundBlipVisibility(bool Visible);

protected:
	void UpdateSoundBlips(float DeltaSeconds);

	bool bSoundBlipsVisible;

	// ~Sound Blips
	/////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////
	// Console Commands
public:
	// Activate and fill jason's ability timers.
	UFUNCTION(exec, BlueprintCallable, Category = "Console")
	void FillAbilities();
private:
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_FillAbilities();

	// Activate Jason's Rage
	UFUNCTION(exec, BlueprintCallable, Category = "Console")
	void ActivateRage();

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_ActivateRage();

	UFUNCTION(exec, BlueprintCallable, Category = "Console")
	void RefillKnives();

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_RefillKnives();

	UFUNCTION(exec, BlueprintCallable, Category = "Console")
	void RefillTraps();

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_RefillTraps();


	// ~Console Commands
	/////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////
	// Ragdoll dragging
public:

	UFUNCTION()
	void UpdateCounselorRagdolls();

	UFUNCTION(Server, UnReliable, WithValidation)
	void SERVER_SetCounselorRagdollPosition(ASCCounselorCharacter* Character, FVector_NetQuantize NewLocation);

	UFUNCTION(NetMulticast, UnReliable)
	void MULTICAST_SetCounselorRagdollPosition(ASCCounselorCharacter* Character, FVector_NetQuantize NewLocation);

	uint8 RagdollUpdateCounter;

	float MaxVariance;

	// ~Ragdoll dragging
	/////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////
	// Throw Functionality
public:

	/** The number of knives in Jasons posession */
	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "Throw")
	int32 NumKnives;

	UFUNCTION(BlueprintCallable, Category = "Throw")
	void AddKnives(int32 NumToAdd);

	/** Called from anim notify so that the knife is released at the proper time */
	UFUNCTION(BlueprintCallable, Category = "Throw")
	void ThrowKnife();

	/** Returns if the player is actively in the throwing state */
	UFUNCTION(BlueprintPure, Category = "Throw")
	bool IsThrowingKnife() const { return bThrowingKnife; }

	/** Returns true if the killer can throw a knife */
	bool CanThrowKnife() const;

	UFUNCTION(BlueprintPure, Category = "Customize")
	TSubclassOf<class ASCThrowable> GetThrowingKnifeClass() const;

protected:
	/** Knife class to spawn when we start to throw */
	UPROPERTY(EditDefaultsOnly, Category = "Throw")
	TSubclassOf<class ASCThrowable> ThrowingKnifeClass;

	/** Blueprint function to process camera changes for the character */
	UFUNCTION(BlueprintImplementableEvent, Category = "Throw")
	void BP_BeginThrow();

	/** Make sure we do out multicast call from the server */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_PerformThrow();

	/** Update all clients throw anim state */
	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_PerformThrow();

	/** Blueprint function to process camera changes for the character */
	UFUNCTION(BlueprintImplementableEvent, Category = "Throw")
	void BP_PerformThrow();

	/** Blueprint function to process camera changes for the character */
	UFUNCTION(BlueprintImplementableEvent, Category = "Throw")
	void BP_EndThrow();

	/** player input function/ spawn knife on server */
	UFUNCTION()
	void PressKnifeThrow();

	/** Spawn the throwing knife in our hand */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_SpawnKnife();

	/** We need this here since when you cancel a throw from the local controller it wont replicate by default */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_EndThrow();

	/** Have we initiated a knife throw action? true on LocalController from button pressed to EndThrow() */
	UPROPERTY(Transient)
	bool bThrowingKnife;

	/** The socket the knife should be attached to while aiming */
	UPROPERTY(EditDefaultsOnly, Category = "Throw")
	FName KnifeAttachSocket;

	FTimerHandle TimerHandle_EndThrow;
	FTimerHandle TimerHandle_ThrowKnife;

	UPROPERTY()
	class UInputComponent* ThrowInputComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Throw")
	float ThrowingKnifeCooldown;

	UPROPERTY()
	float ThrowingKnifeCooldownTimer;

	// ~Throw Functionality
	/////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////
	// Hunter interaction
public:

	// individual stun data for used based on number of impacts detected

	/** Stun data for light stun from the shotgun */
	UPROPERTY(EditDefaultsOnly, Category = "Hunter")
	TSubclassOf<UDamageType> LightShotgunStun;

	/** Stun data for medium stun from the shotgun */
	UPROPERTY(EditDefaultsOnly, Category = "Hunter")
	TSubclassOf<UDamageType> MediumShotgunStun;

	/** Stun data for heavy stun from the shotgun */
	UPROPERTY(EditDefaultsOnly, Category = "Hunter")
	TSubclassOf<UDamageType> HeavyShotgunStun;

	// ~Hunter interaction
	/////////////////////////////////////////////////////////////////////////////

protected:
	bool bColorGradeEnabled;
	float ExposureValue;

	/////////////////////////////////////////////////////////////////////////////
	// Score Events
public:

	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	TSubclassOf<USCScoringEvent> KnockMaskOffScoreEvent;

	// ~Score Events
	/////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////
	// UI
public:
	/** Helper function that returns all of the Killers Strengths. */
	TArray<TSubclassOf<USCStengthOrWeaknessDataAsset>> GetStrengths() const { return KillerStrengths; }

	/** Helper function that returns all of the Killers Weaknesses. */
	TArray<TSubclassOf<USCStengthOrWeaknessDataAsset>> GetWeaknesses() const { return KillerWeaknesses; }
protected:
	// Array that stores all of the Killers Strengths.
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "UI")
	TArray<TSubclassOf<USCStengthOrWeaknessDataAsset>> KillerStrengths;

	// Array that stores all of the Killers Weaknesses. 
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "UI")
	TArray<TSubclassOf<USCStengthOrWeaknessDataAsset>> KillerWeaknesses;

	// ~UI
	/////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////
	// Break Down (Door/Wall/etc.)
public:
	/** Returns false when the player is still holding the attack button */
	bool ShouldAbortBreakingDown() const;

	/** Clears bWantsToBreakDown so we can stop on the next loop check */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_AbortBreakingDown();

	/** Returns the actor we're currently breaking down (could be a door or wall or whatever) */
	AActor* GetBreakDownActor() const { return BreakDownActor; }

	/** Clears the break down actor, should only be called from the actor that's being broken down */
	void ClearBreakDownActor();

private:
	/** Called from the server when you can't break down this door/wall */
	UFUNCTION(Client, Reliable)
	void CLIENT_ClearBreakDownActor();

	// If a counselor is within this distance (in cm) and on the same side of the object as us, we will not start breaking down the object
	UPROPERTY(EditDefaultsOnly, Category = "Door Break")
	float MaxAttackInteractCounselorDistance;

	// Angle (in degrees) off that the camera view can be in order to active the break down special move
	UPROPERTY(EditDefaultsOnly, Category = "Door Break")
	float MaxAttackInteractAngle;

	// Keep track of if attack is being held down or not
	UPROPERTY(Transient, Replicated)
	bool bWantsToBreakDown;

	// Used to keep track of which actor we're breaking down (could be a door or wall or whatever)
	UPROPERTY(Transient)
	AActor* BreakDownActor;

	/** Set on the server so they can apply damage correctly */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_SetBreakDownActor(AActor* BreakActor);

	/**
	 * Checks if we want to swing on an object rather than a nearby counselor
	 * @param TestComponent - The component that want to be interacted with via attacking
	 * @param CounselorCheckFunc - [](const ASCCounselorCharacter*, ...) -> bool { return true; }, if this lambda returns false the function will return false, called on each counselor that is alive and nearby
	 * @param Vars - Additional vars to pass to lambda
	 * @return If true, we're facing the correct direction and there are no counselors near enough to override the interaction
	 */
	template<typename TCounselorCheckFunc, typename... TVarTypes>
	bool ShouldInteractionTakePrecedenceOverNormalAttack(const USCInteractComponent* TestComponent, TCounselorCheckFunc CounselorCheckFunc, TVarTypes... Vars) const;

	/**
	 * Tries to start breaking down a door
	 * @param Door - Door we want to try breaking down
	 * @return True if the player is going to start breaking down the door, false if they failed any number of checks
	 */
	bool TryStartBreakingDoor(ASCDoor* Door);

	/** Only the server can start a special move correctly! */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_StartBreakingDownDoor(ASCDoor* Door);

	/**
	 * Tries to start breaking down a wall
	 * @param Wall - Wall we want to try breaking down
	 * @return True if the player is going to start breaking down the wall, false if they failed any number of checks
	 */
	bool TryStartBreakingWall(ASCDestructableWall* Wall);

	/** Only the server can start a special move correctly! */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_StartBreakingDownWall(ASCDestructableWall* Wall);

	// ~Break Down
	/////////////////////////////////////////////////////////////////////////////

protected:
	virtual void OnCharacterInView(ASCCharacter* VisibleCharacter) override;
	virtual void OnCharacterOutOfView(ASCCharacter* VisibleCharacter) override;

	/////////////////////////////////////////////////////////////////////////////
	// VO & Audio
public:
	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_PlayPamelaVOFromCounselorDeath();

	UPROPERTY(EditDefaultsOnly, Category = "Audio|Shift")
	USoundCue* ShiftAudioCue;

	UPROPERTY(EditDefaultsOnly, Category = "Audio|Shift")
	USoundMix* ShiftAudioSoundMix;

	UPROPERTY(EditDefaultsOnly, Category = "Audio|Shift")
	USoundMix* StalkAudioSoundMix;

	UPROPERTY(EditDefaultsOnly, Category = "Audio|Teleport")
	USoundCue* TeleportCueCounselorHears;

	UPROPERTY(EditDefaultsOnly, Category = "Audio|Teleport")
	USoundMix* TeleportCounselorHearsMix;

	UPROPERTY(EditDefaultsOnly, Category = "Audio|Underwater")
	USoundCue* UnderWaterCue;

	UPROPERTY(EditDefaultsOnly, Category = "Audio|Underwater")
	USoundMix* UnderwaterSoundMix;

private:
	UFUNCTION()
	void ShiftMusicPitchDelayTime();

	UPROPERTY()
	UAudioComponent* ShiftAudioComponent;

	UPROPERTY()
	UAudioComponent* GrabbedAudioComponent;

	UPROPERTY()
	UAudioComponent* UnderWaterAudioComponent;

	bool TelePortEffectOn;

	// ~VO & Audio
	/////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////
	// Stun Effects
public:
	UPROPERTY(EditDefaultsOnly, Category = "Stun|Effects")
	UMaterial* StunScreenEffectMaterial;

protected:
	UPROPERTY()
	UMaterialInstanceDynamic* StunScreenEffectMaterialInst;

private:
	float FringeIntensity;
	float StunnedDistortionDelta;
	float StunnedAlphaDelta;
	bool bIsStunned;

	// ~Stun Effects
	/////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////
	// Police Interaction
protected:
	/** Stun for when Jason gets too close to a police escape */
	UPROPERTY(EditDefaultsOnly, Category = "Police Stun")
	TSubclassOf<UDamageType> PoliceShootJasonStunClass;

	/** The size of the angle we iterate by while checking if Jason's capsule fits outside the escape volume. */
	UPROPERTY(EditDefaultsOnly, Category = "Police Stun", meta = (ClampMin = 1, UIMin = 1, ClampMax = 10, UIMax = 10))
	float PoliceStunIterationAngle;

	// True if we crossed the police stun line and we are performing a context kill
	bool bDelayPoliceStun;

	// Pending police stun info
	FPoliceStunInfo PendingStunInfo;

public:

	/** Jason is shot by the police
	 * We'll iterate toward the escape volume's forward to try and find a valid stun location
	 */
	void StunnedByPolice(USceneComponent* DamageCauser, FVector EscapeForward, float Radius);

	/** Blueprint call to activate a police stun camera */
	UFUNCTION(BlueprintImplementableEvent, Category = "PoliceStun")
	void BP_StunnedByPolice();

	/** Jason is told off by those bad police men */
	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_SpottedByPolice(USoundCue* PlayAudio, const FVector_NetQuantize Location);

	// ~Police interaction
	/////////////////////////////////////////////////////////////////////////////


	/////////////////////////////////////////////////////////////////////////////
	// Stalk ability
public:
	void KillJasonMusic();

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_KillJasonMusic();

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_CancelStalk();

	UFUNCTION(BlueprintPure, Category = "Stalk")
	bool IsJasonMusicSilenced() const { return bIsStalkActive || !IsVisible(); }

	/* The colorgrade to use when in sense mode. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stalk")
	UTexture* StalkColorGrade;

	UFUNCTION()
	void CancelStalk();

	UFUNCTION(BlueprintCallable, Category = "Stalk")
	bool IsStalkActive() const { return bIsStalkActive; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Stalk")
	float StalkActiveTime;

	UPROPERTY(ReplicatedUsing = OnRep_Stalk, Transient, BlueprintReadOnly, Category = "Stalk")
	bool bIsStalkActive;

	UPROPERTY(EditDefaultsOnly, Category = "Stalk")
	float IdleStalkModifier;

	UPROPERTY(Replicated, Transient)
	FFloat_NetQuantize StalkActiveTimer;

	UFUNCTION()
	void OnRep_Stalk();

	// ~Stalk ability
	/////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////
	// Jason Traps
public:
	// If this is a valid location to place a trap OutImpactPoint and PutTrapRotation wil be our data for placing the spawned trap.
	UFUNCTION(BlueprintPure, Category = "Trap")
	bool CanPlaceTrap(FVector& OutImpactPoint, FRotator& OutTrapRotation) const;

	// Wrapper for CanPlaceTrap if we only need the return value.
	bool CanPlaceTrap() const;

	UFUNCTION()
	void AttemptPlaceTrap();

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_AttemptPlaceTrap();

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_PlaceTrap();

	UFUNCTION(Client, Reliable)
	void CLIENT_FailPlaceTrap();

	UPROPERTY(Replicated, Transient, BlueprintReadOnly, Category = "Trap")
	int TrapCount;

	bool IsPlacingTrap() const { return bIsPlacingTrap; }

	/** Function that is called every tenth of a second to determine if we can place a trap. Used for the UI mainly. */
	void UpdateTrapPlacementTrace();

	TArray<ASCTrap*> PlacedTraps;

	UPROPERTY(EditDefaultsOnly, Category = "Trap")
	int InitialTrapCount;

	UFUNCTION(BlueprintPure, Category = "Customize")
	TSubclassOf<class ASCTrap> GetKillerTrapClass() const;

protected:

	UPROPERTY(EditDefaultsOnly, Category = "Trap")
	TSubclassOf<class ASCTrap> KillerTrapClass;

	UPROPERTY(EditDefaultsOnly, Category = "Trap")
	float TrapPlacementDistance;

	UPROPERTY(EditDefaultsOnly, Category = "Trap")
	UAnimMontage* PlaceTrapMontage;

	FTimerHandle TimerHandle_PlaceTrap;

	UPROPERTY()
	bool bIsPlacingTrap;

	UPROPERTY()
	bool bStandingOnTerrain;

	UPROPERTY()
	FTimerHandle TimerHandle_TerrainTest;

	UPROPERTY(EditDefaultsOnly, Category = "Trap")
	float TrapRadius;

	// ~Jason Traps
	/////////////////////////////////////////////////////////////////////////////

public:
	// should only ever get called from blueprint to set the preview mesh
	UFUNCTION(BlueprintCallable, Category = "Skin")
	void SetSkin(TSubclassOf<class USCJasonSkin> NewSkin);

protected:
	/** The material set used to override Jasons default material set. */
	UPROPERTY(ReplicatedUsing = OnRep_MeshSkin, Transient)
	TSubclassOf<class USCMaterialOverride> MeshSkinOverride;

	/** The material set used to override Jasons masks default material set. */
	UPROPERTY(ReplicatedUsing = OnRep_MaskSkin, Transient)
	TSubclassOf<class USCMaterialOverride> MaskSkinOverride;

	UFUNCTION()
	void OnRep_MeshSkin();

	UFUNCTION()
	void OnRep_MaskSkin();

	//////////////////////////////////////////////////
	// Stealth
public:
	// Begin ASCCharacter interface
	virtual void MakeStealthNoise(float Loudness = 1.f, APawn* NoiseInstigator = nullptr, FVector NoiseLocation = FVector::ZeroVector, float MaxRange = 0.f, FName Tag = NAME_None) override;
	// End ASCCharacter interface

	/** @return Stealth normalized by difficulty. */
	virtual float CalcDifficultyStealth() const;

	/** @return Noise value modified against our stealth. */
	virtual float CalcStealthNoise(const float Noise = 1.f) const;

	/** Called from ASCCounselorAIController::ReceivedKillerStimulus. */
	virtual void CounselorReceivedStimulus(const bool bVisual, const float Loudness, const bool bShared);

	/** @return true if we are hearable under the requirements of stealth by HeardBy. */
	virtual bool IsStealthHearbleBy(ASCCounselorAIController* HeardBy, const FVector& Location, float Volume) const;

	/** @return true if we are visible under the requirements of stealth by SeenBy. */
	virtual bool IsStealthVisibleFor(ASCCounselorAIController* SeenBy) const;

protected:
	/** Called from Tick on Authority. */
	virtual void UpdateStealth(const float DeltaSeconds);

	// Current stealth ratio
	UPROPERTY(Transient)
	float StealthRatio;

	// Noise multiplier for full stealth
	UPROPERTY(Config)
	float StealthNoiseScale;

	// Distance multiplier for full stealth
	UPROPERTY(Config)
	float StealthRadiusScale;

	// Noise multiplier for no stealth
	UPROPERTY(Config)
	float AntiStealthNoiseScale;

	// Distance multiplier for no stealth
	UPROPERTY(Config)
	float AntiStealthRadiusScale;

	// Influence heard stimulus occurrences have on final anti-stealth
	UPROPERTY(Config)
	float AntiStealthHeardInfluence;

	// Stealth decay rate for being heard
	UPROPERTY(Config)
	float StealthHeardDecayRate;

	// Stealth been heard occurrence limit
	UPROPERTY(Config)
	float StealthHeardOccurrenceLimit;

	// Influence indirect stimulus occurrences have on final anti-stealth
	UPROPERTY(Config)
	float AntiStealthIndirectlySensedInfluence;

	// Stealth decay rate for being sensed indirectly
	UPROPERTY(Config)
	float StealtIndirectlySensedDecayRate;

	// Stealth indirectly sensed occurrence limit
	UPROPERTY(Config)
	float StealthIndirectlySensedOccurrenceLimit;

	// Influence seen stimulus occurrences have on final anti-stealth
	UPROPERTY(Config)
	float AntiStealthSeenInfluence;

	// Stealth decay rate for being seen
	UPROPERTY(Config)
	float StealtSeenDecayRate;

	// Stealth been seen occurrence limit
	UPROPERTY(Config)
	float StealthSeenOccurrenceLimit;

	// Influence made noise occurrences have on final anti-stealth
	UPROPERTY(Config)
	float AntiStealthMadeNoiseInfluence;

	// Stealth decay rate for making noise
	UPROPERTY(Config)
	float StealthMadeNoiseDecayRate;

	// Stealth make noise occurrence limit
	UPROPERTY(Config)
	float StealthMadeNoiseOccurrenceLimit;

	// Stealth accumulator for being heard
	float StealthHeardAccumulator;

	// Stealth accumulator for being indirectly sensed
	float StealthIndirectlySensedAccumulator;

	// Stealth accumulator for being seen
	float StealthSeenAccumulator;

	// Stealth accumulator for making noise
	float StealthMadeNoiseAccumulator;
};
