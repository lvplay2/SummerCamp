// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Runtime/Engine/Classes/Components/SplineComponent.h"

#include "SCCharacter.h"
#include "SCAnimDriverComponent.h"
#include "SCClothingData.h"
#include "StatModifiedValue.h"
#include "SCFearComponent.h"
#include "SCRepairPart.h"
#include "SCCounselorActiveAbility.h"
#include "SCVehicleSeatComponent.h"

#include "SCCounselorCharacter.generated.h"

class ASCDriveableVehicle;
class ASCGun;
class ASCHidingSpot;
class ASCItem;
class ASCKillerCharacter;
class ASCPamelaSweater;
class ASCRadio;
class ASCSpecialItem;

class UCameraShake;
class UILLIKFinderComponent;
class USCContextKillComponent;
class USCEmoteData;
class USCFearEventData;
class USCNameplateComponent;
class USCInventoryComponent;
class USCPerkData;
class USCScoringEvent;
class USCStatBadge;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIsHiding, bool, IsHiding);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCounselorEscaped);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGrabbed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDropped);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOfflineAIMode);

/**
 * @enum EDodgeDirection 
 */
UENUM(BlueprintType)
enum class EDodgeDirection : uint8
{
	Left,
	Right,
	Back,
};

/**
 * @enum ECounselorClassType 
 */
UENUM()
enum class ECounselorClassType : uint8
{
	Support,
	Fixer,
	Fighter,
	Leader,
	Runner
};

/**
 * @struct FGoreMaskData 
 */
USTRUCT()
struct FGoreMaskData
{
	GENERATED_BODY()

	FGoreMaskData()
	: BloodMask(nullptr)
	, TextureParameter(NAME_None)
	{
	}

	FGoreMaskData(TAssetPtr<UTexture> BloodMaskToApply, FName TextureParameterName, TArray<int32>& MaterialIndicies)
	: BloodMask(BloodMaskToApply)
	, TextureParameter(TextureParameterName)
	, MaterialElementIndices(MaterialIndicies)
	{
	}
	
	// Blood mask for damage
	UPROPERTY(EditDefaultsOnly)
	TAssetPtr<UTexture> BloodMask;

	// Texture parameter name in the material
	UPROPERTY(EditDefaultsOnly)
	FName TextureParameter;

	// The material indices the BloodMask should be applied to
	UPROPERTY(EditDefaultsOnly)
	TArray<int32> MaterialElementIndices;
};

/**
 * @struct FDamageGoreMaskData 
 */
USTRUCT()
struct FDamageGoreMaskData : public FGoreMaskData
{
	GENERATED_BODY()

	FDamageGoreMaskData()
	{
	}

	// The damage type to associate this blood mask to
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UDamageType> DamageType;

	// The lowest health this player can have for this mask to be applied
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float HealthThreshold;
};

/**
 * @class ASCCounselorCharacter
 */
UCLASS()
class SUMMERCAMP_API ASCCounselorCharacter
: public ASCCharacter
{
	GENERATED_UCLASS_BODY()

public:
	// Begin AActor Interface
	virtual void Destroyed() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void PostInitializeComponents() override;
	virtual void SetActorHiddenInGame(bool bNewHidden) override;
	// End AActor Interface

	// Begin APawn Interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	virtual bool IsLocallyControlled() const override;
	virtual bool IsPlayerControlled() const override;
	// End APawn Interface

	// Begin AILLCharacter Interface
	virtual bool ShouldShowCharacter() const override;
	virtual void UpdateCharacterMeshVisibility() override;
	virtual void OnReceivedPlayerState() override;
	virtual bool CanInteractAtAll() const override;
	virtual void OnReceivedController() override;
	virtual void StopCameraShakeInstances(TSubclassOf<UCameraShake> Shake) override;
	// End AILLCharacter interface

	// Begin ASCCharacter Interface
	virtual ASCPlayerState* GetPlayerState() const override;
	virtual bool HasEscaped() const override { return bHasEscaped; }
	virtual bool CanTakeDamage() const override;
	virtual float GetSpeedModifier() const override;
	virtual void KilledBy(APawn* EventInstigator) override;
	virtual void OnDeath(float KillingDamage, struct FDamageEvent const& DamageEvent, ASCPlayerState* KillerPlayerState, AActor* DamageCauser) override;
	virtual bool HasItemInInventory(TSubclassOf<ASCItem> ItemClass) const override;
	virtual int32 NumberOfItemTypeInInventory(TSubclassOf<ASCItem> ItemClass) const override;
	virtual void SERVER_OnInteract_Implementation(UILLInteractableComponent* Interactable, EILLInteractMethod InteractMethod) override;
	virtual void OnInteract1Pressed() override;
	virtual void OnInteract1Released() override;
	virtual void OnAttack() override;
	virtual void OnClientLockedInteractable(UILLInteractableComponent* LockingInteractable, const EILLInteractMethod InteractMethod, const bool bSuccess);
	virtual void MULTICAST_DamageTaken_Implementation(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser) override;
	virtual void CleanupLocalAfterDeath(ASCPlayerController* PlayerController) override;
	virtual AController* GetCharacterController() const override;
	virtual void ClearAnyInteractions() override;
	virtual void UpdateSwimmingState() override;
	virtual void BreakGrab(bool bStunWithPocketKnife = false) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void ContextKillStarted() override;
	virtual void InitStunWiggleGame(const USCStunDamageType* StunDefaults, const FSCStunData& StunData) override;
	virtual void DrawDebug() override;
	virtual void DrawDebugStuck(float& ZPos) override;
	// End ASCCharacter Interface

	UFUNCTION()
	virtual void AddVehicleYawInput(float Val);

	UFUNCTION()
	virtual void AddVehiclePitchInput(float Val);

	UFUNCTION(Client, Reliable)
	virtual void CLIENT_RemoveAllInputComponents();

	// Next time that we can emit a sound blip
	float NextSoundBlipTime;

	////////////////////////////////////////////////////////////////////////////////
	// Components
public:
	/** The nameplate component for all other counselors to see */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCNameplateComponent* NameplateWidget;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UStaticMeshComponent* LightMesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UStaticMeshComponent* WalkieTalkie;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USpotLightComponent* Flashlight;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USkeletalMeshComponent* Hair;

	/** How quick to interpolate towards the actual rotation of the flashlight (0 for instant) */
	UPROPERTY(EditDefaultsOnly, Category = "Pawn|Flashlight", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FlashlightLerpSpeed;

	UFUNCTION(BlueprintImplementableEvent, Category = "Pawn|Flashlight")
	void OnFlashlightToggle(bool bOn);

	/** Set a new offset and interp time for the flashlight location 
	 * @param NewOffset - the new offset to transition too, FVector::ZeroVector will move it back to hip resting position
	 * @param InterpTime - The time in seconds for the interpolation to take from current position to new offset.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pawn|Flashlight")
	void SetFlashlightDesiredOffset(const FVector NewLocationOffset, const FRotator NewRotationOffset, float InterpTime);

	/** Turns flashlight on or off */
	UFUNCTION(Server, Reliable, WithValidation)
	void ToggleFlashlight();

	/** Turns the flashlight on */
	UFUNCTION(BlueprintCallable, Category="Pawn|Flashlight")
	void TurnOnFlashlight();

	UFUNCTION(BlueprintCallable, Category = "Pawn|Flashlight")
	void ShutOffFlashlight();

	UFUNCTION()
	virtual void OnRep_FlashlightOn(bool bWasFlashlightOn);

	/** @return true if the flashlight is on. */
	FORCEINLINE bool IsFlashlightOn() const { return bIsFlashlightOn; }

private:
	/** Internal variable for tracking initial rotation of flashlight (set on BeginPlay) */
	FRotator FlashlightMeshBaseRotation;

	/** Used to lerp flashlight toward the full rotation value so we don't get off by too much */
	FRotator FlashlightCurrentRotation;

	/** Initial rotation of flashlight at game startup */
	FRotator FlashlightBaseRotation;

	/** Internal variables for tracking initial location of flashlight (set on BeginPlay) */
	FVector FlashlightBaseLocation;

	/** Internal flashlight location interpolation variables */
	FVector FlashlightDesiredLocation;

	/** The relative position when SetDesiredOffset is called */
	FVector FlashlightLerpStartLocation;

	/** Internal flashlight desired rotation offset */
	FRotator FlashlightDesiredRotationOffset;

	/** The relative rotation when SetDesiredOffset is called */
	FRotator FlashlightLerpStartRotation;

	/** Total time for flashlight location lerp */
	float FlashlightLocationLerpTime;

	/** Current timer for flashlight location lerp */
	float FlashlightCurrentLerpTime;

	/** Is flashlight turned on by the player */
	UPROPERTY(Replicated, ReplicatedUsing = OnRep_FlashlightOn)
	bool bIsFlashlightOn;

	// ~Components
	////////////////////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////////////////////
	// Limbs
public:
	// Head/Face
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCCharacterBodyPartComponent* FaceLimb;

	// Chest/Torso
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCCharacterBodyPartComponent* TorsoLimb;

	// Right Arm
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCCharacterBodyPartComponent* RightArmLimb;

	// Left Arm
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCCharacterBodyPartComponent* LeftArmLimb;

	// Right Leg
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCCharacterBodyPartComponent* RightLegLimb;

	// Left Leg
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCCharacterBodyPartComponent* LeftLegLimb;

	/** Detach the specified limb */
	UFUNCTION(BlueprintCallable, Category = "Pawn|Limbs")
	void DetachLimb(USCCharacterBodyPartComponent* Limb, const FVector Impulse = FVector::ZeroVector, TAssetPtr<UTexture> BloodMaskOverride = nullptr, const bool bShowLimb = true);

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_DetachLimb(USCCharacterBodyPartComponent* Limb, FVector_NetQuantize Impulse);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_DetachLimb(USCCharacterBodyPartComponent* Limb, FVector_NetQuantize Impulse);

	/** Used for testing */
	UFUNCTION(BlueprintCallable, Category = "Pawn|Limbs")
	void ReattachAllLimbs();

	/** Show blood on the specified limb */
	UFUNCTION(BlueprintCallable, Category = "Pawn|Limbs|Gore")
	void ShowBloodOnLimb(USCCharacterBodyPartComponent* Limb);

	void HideFace(bool bHide);

	/** 
	* Spawn blood decals around the character 
	* @param TestSocket - The socket the ray trace should start from
	* @param TestDirection - The direction the ray trace should go in relation from TestSocket
	* @param DecalTexture - The texture for the blood decal
	* @param DecalSize - Width and height of DecalTexture
	* @param TestLength - How far should the ray trace go
	*/
	UFUNCTION(BlueprintCallable, Category = "Pawn|Limbs|Gore")
	void SpawnBloodDecal(FName TestSocket, const FVector TestDirection, UMaterial* DecalTexture, const float DecalSize, const float TestLength = 5.f);

	/** Finds the first body part component with the specified bone name */
	USCCharacterBodyPartComponent* GetBodyPartWithBone(const FName BoneName) const;

protected:
	/** Detaches all limbs, will reattach when called again */
	UFUNCTION()
	void PopLimbs();

	// Allows pop limbs to toggle the limbs on/off
	bool bDebugLimbsAreDetached;

	// ~Limbs
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	// Gore
public:

	/** Gore mask to be applied when going through a broken window */
	UPROPERTY(EditDefaultsOnly, Category = "Gore")
	FGoreMaskData WindowDamageGoreMask;

	/** Gore masks to be applied to the character when they recieve a specific damage */
	UPROPERTY(EditDefaultsOnly, Category = "Gore")
	TArray<FDamageGoreMaskData> DamageGoreMasks;

	/** Apply gore masks to the body and limbs */
	void ApplyGore(const FGoreMaskData& GoreData);

	/** Remove gore from the character (called when healed) */
	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_RemoveGore();

protected:
	/** Does the actual cleanup on the character */
	UFUNCTION(BlueprintCallable, Category = "Gore")
	void RemoveGore();

private:
	/** Called when PendingGoreData loads and can be applied. */
	void DeferredApplyGoreData();

	// Gore data pending texture load to be applied in DeferredApplyGoreData
	TArray<FGoreMaskData> PendingGoreDatas;

	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> CurrentGoreMasks;

	/** Apply the gore mask associated with the passed in damge type */
	void ApplyDamageGore(TSubclassOf<UDamageType> DamageType);

	// ~Gore
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	// Health
public:
	/** When health drops to or below this threshold, the counselor is considered wounded */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Health")
	float WoundedThreshold;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_Wounded)
	bool bIsWounded;

	/** Returns true if the counselor is still alive and is at or below the wounded threshold */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Counselor|Health")
	bool IsWounded() const { return bIsWounded; }

	/** Called when health drops to or below the wounded threshold */
	UFUNCTION(BlueprintImplementableEvent, Category = "Counselor|Health")
	void OnWounded() const;

	UPROPERTY(EditDefaultsOnly, Category = "Paranoia")
	USoundCue* ImposterDeathNoise;

protected:
	UFUNCTION()
	void OnRep_Wounded();
	// ~Health
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	// Stamina
public:

	/** Total current stamina */
	UPROPERTY(Replicated, Transient, BlueprintReadOnly, Category = "Counselor|Stamina")
	FFloat_NetQuantize CurrentStamina;

	/** Max value of stamina */
	UPROPERTY(BlueprintReadOnly, Category = "Counselor|Stamina")
	float StaminaMax;

	/** When stamina is depleated, this is the minimum threshold before stamina can be used again */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Stamina")
	float StaminaMinThreshold;

	/** When set, we have to wait for stamina to pass the StaminaMinThreshold before we can use stamina again */
	UPROPERTY(BlueprintReadOnly, Category = "Counselor|Stamina")
	bool bStaminaDepleted;
	
	/** Base stamina depletion rate while moving */
	UPROPERTY(EditDefaultsOnly, Category = "Counselor|Stamina")
	float StaminaDepleteRate;

	/** Base stamina refill rate when idle */
	UPROPERTY(EditDefaultsOnly, Category = "Counselor|Stamina")
	FStatModifiedValue StaminaRefillRate;

	/** Amount to scale stamina refill when idle */
	UPROPERTY(EditDefaultsOnly, Category = "Counselor|Stamina")
	float StaminaIdleModifier;

	/** Amount to scale stamina refill when hiding */
	UPROPERTY(EditDefaultsOnly, Category = "Counselor|Stamina")
	float StaminaHidingModifier;

	/** Amount to scale stamina depletion when walking */
	UPROPERTY(EditDefaultsOnly, Category = "Counselor|Stamina")
	float StaminaWalkingModifier;

	/** Amount to scale stamina depletion when running */
	UPROPERTY(EditDefaultsOnly, Category = "Counselor|Stamina")
	float StaminaRunningModifier;

	/** Amount to scale stamina depletion when sprinting */
	UPROPERTY(EditDefaultsOnly, Category = "Counselor|Stamina")
	float StaminaSprintingModifier;

	/** Amount to scale stamina depletion when swimming */
	UPROPERTY(EditDefaultsOnly, Category = "Counselor|Stamina")
	float StaminaSwimmingModifier;

	/** Amount to scale stamina depletion when crouched */
	UPROPERTY(EditDefaultsOnly, Category = "Counselor|Stamina")
	float StaminaCrouchingModifier;

	/** Cost to stamina to perform a dodge (any direction) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Stamina")
	FStatModifiedValue StaminaStatMod;

	/** Refill rate (per second) while grabbed */
	FStatModifiedValue GrabbedStaminaRefillRate;

	/** The amount to scale the base refill rate with, the curve is indexed into using the current fear amount. */
	UPROPERTY(EditDefaultsOnly, Category = "Counselor|Stamina")
	UCurveFloat* StaminaFearRefillModifier;

	FORCEINLINE float GetStamina() const { return CurrentStamina; }
	FORCEINLINE float GetMaxStamina() const { return StaminaMax; }

	/** If true, we have enough stamina for this action and are not currently depleated */
	UFUNCTION(BlueprintCallable, Category = "Counselor|Stamina")
	FORCEINLINE_DEBUGGABLE bool CanUseStamina(float StaminaToUse = 0.f) const
	{
		if (bStaminaDepleted)
			return false;

		// Allow a little leeway to allow us to actually bottom out
		return (CurrentStamina - StaminaToUse) > -0.5f;
	}

	/** Calls CanUseStamina, if passed uses the stamina and also returns true, otherwise returns false */
	UFUNCTION(BlueprintCallable, Category = "Counselor|Stamina")
	bool TryUseStamina(float StaminaToUse);

	UFUNCTION(BlueprintCallable, Category = "Counselor|Stamina")
	void ReturnStamina(float StaminaToGain);

	/** Called when a counselor uses a chunk of stamina (1 point or larger) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Counselor|Stamina")
	void OnStaminaChunkUsed(float StaminaAmount) const;

	/** Called when a counselor gains a chunk of stamina (1 point or larger) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Counselor|Stamina")
	void OnStaminaChunkGained(float StaminaAmount) const;

protected:
	void UpdateStamina(float DeltaTime);

private:
	float GetStaminaModifier() const;

	// ~Stamina
	////////////////////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////////////////////
	// Stat Utilities
public:

	UPROPERTY(EditDefaultsOnly, Category = "Counselor|Stats", meta = (UIMin = 1, UIMax = 10))
	uint8 Stats[(uint8)ECounselorStats::MAX];

	/**
	 * Gets the stat alpha [0->1] for the stat value [1->10]
	 * @param Stat - Stat to get the delta from
	 * @return Returns the float alpha for the given stat, 0.5 (average) if no stat was found
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Counselor|Stats")
	FORCEINLINE_DEBUGGABLE float GetStatAlpha(const ECounselorStats& Stat) const
	{
		return ((float)Stats[(uint8)Stat] - 1.f) / 9.f;
	}

	/**
	 * Converts a base value into the stat modified value based on the range
	 * @param Stat - Stat that affects our base value
	 * @param BaseValue - Default value (same as if our stat was 6)
	 * @param StatModifier - Percent value (0-100) to build a range off of (range will be BaseValue * [-StatModifier, +StatModifier])
	 * @return Returns the modification to apply to the base value (just add it)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Counselor|Stats")
	FORCEINLINE_DEBUGGABLE float CalculateStatModification(const ECounselorStats& Stat, const float BaseValue, const float StatModifier) const
	{
		const float Adjustment = StatModifier * 0.01f;
		return BaseValue * FMath::Lerp(-Adjustment, Adjustment, GetStatAlpha(Stat));
	}

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Counselor|Stats")
	float GetStatModifiedValue(FStatModifiedValue& StatModifiedValue) const
	{
		return StatModifiedValue.Get(this);
	}

	virtual float GetStatInteractMod() const override { return MinigameSpeedMod.Get(this); }
	virtual float GetPerkInteractMod() const override;

	virtual void Native_NotifyKillerOfMinigameFail(USoundCue* FailSound);

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void SERVER_NotifyKillerOfMinigameFail(USoundCue* FailSound);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Counselor|Stats")
	FStatModifiedValue MinigameSpeedMod;
	// ~Stat Utilities
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	// Water Effects
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Water")
	UParticleSystem* RippleParticle;
protected:
	UPROPERTY()
	UParticleSystemComponent* RippleComponent;
	// ~Water Effects
	////////////////////////////////////////////////////////////////////////////////

public:
	/** Update current position on the exit spline. */
	void FollowExitSpline(float DeltaTime);

	/** Is the character following an exit spline */
	bool IsEscaping() const { return ExitSpline != nullptr; }

	/** Set the player to escaped, invulnerable, and give them the exit spline to follow for the exit cinematic */
	void SetEscaped(USplineComponent* ExitPath, float EscapeDelay = 0.f);

	/** Delegate for when this counselor has escaped */
	UPROPERTY(BlueprintAssignable, Category="Escape")
	FOnCounselorEscaped OnEscaped;

	UPROPERTY(EditDefaultsOnly, Category = "Escape")
	float HunterEscortRadius;

	UPROPERTY(EditDefaultsOnly, Category = "Escape")
	float ActiveSacrificeRadius;

	FTimerHandle TimerHandle_Escape;

	void EnterHidingSpot(ASCHidingSpot* HidingSpot);
	void ExitHidingSpot(ASCHidingSpot* HidingSpot);
	void FinishedExitingHiding();
	ASCHidingSpot* GetCurrentHidingSpot() const { return CurrentHidingSpot; }

	UPROPERTY(EditDefaultsOnly, Category = "Hiding", meta = (ClampMin = "0.2", ClampMax = "1.0", UIMin = "0.2", UIMax = "1.0"))
	float HidingSensitivityMod;

	// X = Right stick X, Y = Right stick Y, Z = mouse
	FVector CachedSensitivity;

	/**
	 * Test if character is in light with a minimum intensity
	 * @return - true if character is affected by any light with a minumum intensity 
	 */
	bool TestLights(); // FIXME: What a very bad name.

	// The minimum intensity of the light affecting the character including falloff.
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	float MinLightStrength;

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void OnPickupWalkieTalkie();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	int32 MaxSmallItems;

	void UpdateInventoryWidget();

	/** If true, we have at least MaxSmallItems in our inventory and cannot hold any more */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsSmallInventoryFull() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TArray<ASCItem*> GetSmallItems() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	ASCItem* GetCurrentSmallItem() const;

	UFUNCTION()
	void RemoveSmallItemOfType(TSubclassOf<ASCItem> ItemClass);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	ASCItem* HasSmallRepairPart(TSubclassOf<ASCRepairPart> PartClass) const;

	bool UseSmallRepairPart(ASCRepairPart* Part);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TArray<ASCItem*> GetSpecialItems() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	ASCItem* HasSpecialItem(TSubclassOf<ASCSpecialItem> ItemClass) const;

	UFUNCTION(Server, Reliable, WithValidation)
	void RemoveSpecialItem(ASCSpecialItem* Item, bool bDestroyItem);

	virtual bool AddOrSwapPickingItem() override;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	virtual void OnSwapItemOnUse();

	virtual void OnItemPickedUp(ASCItem* item) override;

	bool bCachedFromInput;
	bool bShouldDestroyItem;

	UPROPERTY()
	ASCItem* ItemToDestroy;

	// Default item to always spawn this character with (used for hero characters)
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSoftClassPtr<ASCItem> DefaultLargeItem;

	virtual void UseCurrentSmallItem(bool bFromInput = true);

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void SERVER_UseCurrentSmallItem(bool bFromInput);

	UFUNCTION(NetMulticast, Reliable)
	virtual	void MULTICAST_UseCurrentSmallItem(ASCItem* Item, bool bFromInput);

	void OnUseItem();
	void OnForceDestroyItem(ASCItem* Item);
	void OnDestroyItem();

	/** Input Handler for Drop Item */
	UFUNCTION()
	void OnDropItemPressed();

	/** Input Handler for Drop Item */
	UFUNCTION()
	void OnDropItemReleased();

	/** Sets bDroppingSmallItem on the Server */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_SetDroppingSmallItem(const bool bDropping);

	/** Simulates dropping small item */
	UFUNCTION()
	void OnRep_DroppingSmallItem();

	/** Drops Current Small Item */
	void DropCurrentSmallItem();

	FTimerHandle TimerHandle_DropSmallItem;

	void UpdateDropItemTimer(float DeltaSeconds);

	/** Sets bDroppingLargeItem on the Server */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_SetDroppingLargeItem(const bool bDropping);

	/** Simulates dropping large item */
	UFUNCTION()
	void OnRep_DroppingLargeItem();

	/** Override to handle switching bDroppingLargeItem back to false */
	virtual void DropEquippedItem(const FVector& DropLocationOverride = FVector::ZeroVector, bool bOverrideDroppingCharacter = false) override;

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void SERVER_UseCurrentLargeItem(bool bFromInput = true, FVector CameraLocation = FVector::ZeroVector, FRotator CameraForward = FRotator::ZeroRotator);

	UFUNCTION()
	virtual void DestroyEquippedItem();

	virtual bool CanAttack() const override;

	UFUNCTION(BlueprintPure, Category = "Vehicle")
	FORCEINLINE bool IsInVehicle() const { return CurrentSeat != nullptr; }

	virtual void SetSprinting(bool bSprinting) override;

	void SetSprintingWithButtonState(bool Sprinting, bool ButtonState);

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_SetSprintingWithButtonState(bool Sprinting, bool ButtonState);

	virtual void OnForwardPC(float Val) override;
	virtual void OnRightPC(float Val) override;
	virtual void OnForwardController(float Val) override;
	virtual void OnRightController(float Val) override;
	virtual void OnDodge();

	virtual void OnLargeItemPressed();
	virtual void OnLargeItemReleased();

	/** Set wether or not the large item use button is being held */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_SetLargeItemPressHeld(bool Held);

	/** Get if Large item button is being held down */
	UFUNCTION(BLueprintPure, Category = "Items")
	FORCEINLINE bool IsLargeItemPressHeld() const { return bLargeItemPressHeld; }

	virtual void OnSmallItemPressed();

	void OnUseSmallItemOne();
	void OnUseSmallItemTwo();
	void OnUseSmallItemThree();

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_OnUseSmallItem(uint8 ItemIndex);

	virtual void OnAimPressed();
	virtual void OnAimReleased();

	/** True if the character is dodging in either direction */
	virtual bool IsDodging() const override;
	FORCEINLINE bool IsInHiding() const { return bInHiding; };

	FOnIsHiding OnIsHidingChanged;

	UFUNCTION(BlueprintPure, Category = "Counselor")
	bool GetIsFemale() const override { return bIsFemale; }

	UFUNCTION(BlueprintPure, Category = "Counselor")
	bool IsInWheelchair() const final { return bInWheelchair; }
	
	/** Detach the wheelchair skeletal mesh from this counselor */
	void DetachWheelchair();

	virtual bool IsCounselor() const override { return true; }

	virtual void SetCombatStance(const bool bEnable) override;

	virtual void OnRep_CurrentStance() override;

private:
	FVector2D CurrentInputVector;

	bool bSprintPressed;

	bool bUsingSmallItem;

	/** Is Large item button being held down */
	UPROPERTY(Replicated)
	bool bLargeItemPressHeld;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Class")
	ECounselorClassType CounselorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor")
	bool bIsFemale;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor")
	bool bInWheelchair;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Movement")
	float DodgingSpeedMod;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Movement")
	float StumblingSpeedMod;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Movement")
	float WaterSpeedMod;

	/** Dpeth (in cm) the water must be before we transition from walking to swimming */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Movement")
	float MinWaterDepthForSwimming;

	/** The modifier for the counselors overall speed */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Movement")
	FStatModifiedValue SpeedStatMod;

	// Spline to follow when escaping, set from the escape volume.
	UPROPERTY()
	USplineComponent* ExitSpline;
	float SplineDistanceTraveled;

public:
	/** If true, we're driving. Don't make me pull this car/carboat over! */
	FORCEINLINE bool IsDriver() const { return bIsDriver; }

protected:
	// Are we driving, or just riding?
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle")
	bool bIsDriver;

	UPROPERTY()
	bool bHasEscaped;

	UPROPERTY()
	UMaterialInterface* ShirtMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Crouch")
	float CrouchCapsuleRadius;

	UPROPERTY()
	float OriginalCapsuleRadius;

public:
	UPROPERTY()
	UInputComponent* PassengerInputComponent;

	// The character's pitch while aiming.
	UPROPERTY(Replicated, Transient, BlueprintReadOnly)
	FFloat_NetQuantize AimPitch;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool ShouldShowInventory() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetSmallItemIndex() const { return SmallItemIndex; }

	/** @return the first index in the small invenetory of the specified item type. -1 if not found. */
	int32 GetIndexOfSmallItem(TSubclassOf<ASCItem> ItemClass) const;

	/** @return Pointer to this character's small inventory component */
	FORCEINLINE const USCInventoryComponent* GetSmallInventory() const { return SmallItemInventory; }

protected:
	UPROPERTY()
	UInputComponent* AimInputComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	USCInventoryComponent* SpecialItemInventory;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	USCInventoryComponent* SmallItemInventory;

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_SmallItemIndex, BlueprintReadOnly, Category = "Inventory")
	int32 SmallItemIndex;

	UFUNCTION()
	void OnRep_SmallItemIndex();

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Hiding")
	bool bInHiding;

	void ToggleMap();

	UPROPERTY(EditDefaultsOnly, Category = "Flashlight")
	USoundCue* FlashlightOnSound;

	UPROPERTY(EditDefaultsOnly, Category = "Flashlight")
	USoundCue* FlashlightOffSound;

public:
	UFUNCTION(BlueprintCallable, Category = "Counselor|Class")
	ECounselorClassType GetCounselorClassType() const { return CounselorClass; }

	/** Update stamina outside the update, such as in the stun minigame. */
	UFUNCTION()
	void ForceModifyStamina(float InStaminaChange);
	
	/** Stamina modification needs to be server authoritative. */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_ForceModifyStamina(float InStaminaChange);

	bool IsSprintPressed() const;

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// First Aid Spray settings
	/** The distance another Counselor must be within to heal them with first aid */
	UPROPERTY(EditDefaultsOnly, Category = "First Aid")
	float FirstAidDistance;

	/** The angle another Counselor must be within to heal them with first aid */
	UPROPERTY(EditDefaultsOnly, Category = "First Aid")
	float FirstAidAngle;
	//////////////////////////////////////////////////////////////////////////////////////////////////

private:
	void OnToggleSprint();
	void OnToggleSprintStop();
	void OnStartSprint();
	void OnStopSprint();

	void OnRunToggle();
	void OnCrouchToggle();

	void NextSmallItem();
	
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_NextSmallItem();

	void PrevSmallItem();

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_PrevSmallItem();

	UPROPERTY()
	UInputComponent* HidingInputComponent;

	UPROPERTY()
	ASCHidingSpot* CurrentHidingSpot;

	UFUNCTION()
	void OnExitHidingPressed();

	bool bHasAimingCapability;

	UPROPERTY()
	USkeletalMeshComponent* AttachedTo;

	/** Cache the list of light components in the scene */
	UPROPERTY()
	TArray<ULightComponent*> AllLights;

public:
	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_EnterVehicle(ASCDriveableVehicle* Vehicle, USCVehicleSeatComponent* Seat, USCSpecialMoveComponent* SpecialMove);
	
	UFUNCTION()
	void OnEnterVehicleDestinationReached(ASCCharacter* Interactor);

	UFUNCTION()
	void OnEnterVehicleComplete(ASCCharacter* Interactor);

	UFUNCTION()
	void OnEnterVehicleAborted(ASCCharacter* Interactor);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_ExitVehicle(bool bExitBlocked);

	UFUNCTION()
	void OnExitVehicleComplete();

	void GrabbedOutOfVehicle(USCContextKillComponent* KillComp);

	UFUNCTION()
	void OnGrabbedOutOfVehicleComplete(ASCCharacter* Interactor, ASCCharacter* Victim);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_EjectFromVehicle(bool bFromCrash);

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle")
	USCVehicleSeatComponent* PendingSeat;

	FORCEINLINE USCVehicleSeatComponent* GetCurrentSeat() const { return CurrentSeat; }

	void SetCurrentSeat(USCVehicleSeatComponent* Seat);

	UFUNCTION(BlueprintPure, Category = "Vehicle")
	FORCEINLINE ASCDriveableVehicle* GetVehicle() const { return CurrentSeat ? CurrentSeat->ParentVehicle : nullptr; }

protected:
	UPROPERTY()
	USCVehicleSeatComponent* CurrentSeat;

	/** Track where we started when we attempt to get in the dumb car */
	FVector EnterVehicleStartLocation;

public:
	UPROPERTY()
	bool bLeavingSeat;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Vehicle|Camera")
	float MaxVehicleCameraYawAngle;

	UPROPERTY(EditDefaultsOnly, Category = "Vehicle|Camera")
	float MaxVehicleCameraPitchAngle;

	/** Scales the sensitivity of camera rotation when in a vehicle */
	UPROPERTY(EditDefaultsOnly, Category = "Vehicle|Camera", meta = (ClampMin = "0.1", ClampMax = "1.0", UIMin = "0.1", UIMax = "1.0"))
	FVector2D CameraRotateScale;

private:
	float LocalVehicleYaw;
	float LocalVehiclePitch;

public:
	void OnGrabbed(ASCKillerCharacter* Killer);

	UFUNCTION(BlueprintImplementableEvent, Category = "Grabbing")
	void BP_OnGrabbed(ASCKillerCharacter* Killer);

	UPROPERTY(BlueprintAssignable, Category="Grabbing")
	FOnGrabbed OnCounselorGrabbed;

	/**
	 * Drop the counselor
	 * @param IsDead - Will the counselor be dead when dropped?
	 * @param bIgnoreTrace - Only used in anim BP so that at the end of a context kill the counselor is not moved around collision.
	*/
	UFUNCTION(BlueprintCallable, Category = "Grabbing")
	void OnDropped(bool IsDead, bool bIgnoreTrace = false);

	UFUNCTION(BlueprintImplementableEvent, Category = "Grabbing")
	void BP_OnDropped();

	UPROPERTY(BlueprintAssignable, Category="Grabbing")
	FOnDropped OnCounselorDropped;

	/** 
	 * Attach this Counselor to a Killer Character
	 * @param Killer - Killer to attach to
	 * @param bAttachToHand - Attached to Hand or Target Joint
	 */
	void AttachToKiller(bool bAttachToHand);

	UFUNCTION(BlueprintCallable, Category = "Grab")
	void UpdateGrabTransform();

	/**
	 * Begin the grab stun minigame
	 * @param StunDamageType - The SCStunDamageType to get the stun data from
	 */
	void BeginGrabMinigame();

	// Stun damage to apply when grabbed
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grab")
	TSubclassOf<UDamageType> GrabStunDamageType;

	/** Amount of time to keep the counselor attached to Jason during the pocket knife breakout */
	UPROPERTY(EditDefaultsOnly, Category = "Grab")
	float PocketKnifeDetachDelay;

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_PlayCounselorStunnedJasonVO();

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_PlayCounselorKnockedJasonsMaskOffVO();

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_PlayWoundedVO();

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_PlayHitImpactVO();

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_PlayDiedVO();

	/** Is this character being grabbed? */
	UPROPERTY(BlueprintReadWrite)
	bool bIsGrabbed;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_InKillerHand, Transient)
	bool bInKillerHand;

	UPROPERTY(Replicated)
	ASCKillerCharacter* GrabbedBy;

	virtual void EndStun() override;
	void MULTICAST_EndStun_Implementation() override;

	FTimerHandle TimerHandle_BreakOut;
	bool bIsKillerOnMap;
	float MinKillerViewAngle;
	float OutOfSightTime;

protected:
	UFUNCTION()
	void OnRep_InKillerHand();

	// the buoyancy value that was set on the character movement component before the counselor was grabbed
	UPROPERTY()
	float SavedBuoyancy;

	UPROPERTY()
	FTimerHandle TimerHandle_ResetBuoyancy;

	////////////////////////////////////////////////////////////////////////////////
	// Getting Chased
protected:
	/** Angle (in degrees) offset from behind the player a killer must be to start a chase */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Chase")
	float StartChaseKillerAngle;

	/** Angle (in degrees) offset from behind the player a killer must be to end a chase (should be larger than Start) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Chase")
	float EndChaseKillerAngle;

	/** Distance (in cm) behind the player a killer must be to start a chase */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Chase")
	float StartChaseDistance;

	/** Distance (in cm) behind the player a killer must be to end a chase (should be larger than Start) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Chase")
	float EndChaseDistance;

	/** Time (in seconds) for chase to linger before being disabled */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Chase")
	float ChaseTimeout;

	/** If true, killer must be moving towards the player for a chase to be happening */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Chase")
	bool ChasingKillerMustBeMoving;

	/** If true, we have a killer hot on our heels! */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Counselor|Chase")
	bool bIsBeingChased;

	/** 2D Distance (in cm) the chasing killer is from the player */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Counselor|Chase")
	float ChasingKillerDistance;

	/** Used to track the timeout for the chase */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Counselor|Chase")
	float LastChased_Timestamp;

	/** Used to track the length of a chase */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Counselor|Chase")
	float ChaseStart_Timestamp;

	/** Handles updating of all chase variables, called from Tick */
	void UpdateChase();

public:
	/** Called when a chase starts up for the first time */
	UFUNCTION(BlueprintImplementableEvent, Category = "Counselor|Chase")
	void OnChaseStart();

	/** Called when a chase ends */
	UFUNCTION(BlueprintImplementableEvent, Category = "Counselor|Chase")
	void OnChaseEnd(float ChaseTime);

	/** Returns true if this killer meets all the critera of a chasing killer */
	bool IsBeingChasedBy(const ASCCharacter* ChasingCharacter) const;

	/** Returns true if this player is being chased */
	FORCEINLINE bool IsBeingChased() const { return bIsBeingChased; }

	/** 2D Distance (in cm) the chasing player is behind us, only valid if IsBeingChased is true (otherwise will be 0) */
	FORCEINLINE float ChaseDistance() const { return ChasingKillerDistance; }

	// ~Getting Chased
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	// Fear
public:

	void UpdateFearEvents();
	/**
	 * Returns the current amount of fear this counselor has.
	 */
	UFUNCTION(BlueprintPure, Category = "Fear")
	FORCEINLINE float GetFear() const { return FearManager->GetFear(); }

	/** Set this counselors current fear level */
	void SetFear(float FearLevel) { FearManager->LockFearTo(FearLevel, false); }

	/**
	 * Returns the current amount of fear this counselor has in 0 to 1 space.
	 */
	UFUNCTION(BlueprintPure, Category = "Fear")
	FORCEINLINE float GetNormalizedFear() const { return GetFear() * 0.01f; }

	FORCEINLINE void PushFearEvent(TSubclassOf<USCFearEventData> FearEvent) { FearManager->PushFearEvent(FearEvent); }
	FORCEINLINE void PopFearEvent(TSubclassOf<USCFearEventData> FearEvent) { FearManager->PopFearEvent(FearEvent); }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear|Events")
	TSubclassOf<USCFearEventData> DarkFearEvent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear|Events")
	TSubclassOf<USCFearEventData> GrabbedFearEvent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear|Events")
	TSubclassOf<USCFearEventData> WoundedFearEvent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear|Events")
	TSubclassOf<USCFearEventData> IndoorFearEvent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear|Events")
	TSubclassOf<USCFearEventData> HiddenFearEvent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear|Events")
	TSubclassOf<USCFearEventData> InWaterFearEvent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear|Events")
	TSubclassOf<USCFearEventData> JasonSpottedFearEvent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear|Events")
	TSubclassOf<USCFearEventData> CorpseSpottedFearEvent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear|Events")
	TSubclassOf<USCFearEventData> NearFriendlyFearEvent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear|Events")
	TSubclassOf<USCFearEventData> JasonNearFearEvent;

protected:
	UPROPERTY(Transient, Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "Fear")
	USCFearComponent* FearManager;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear")
	UTexture* FearColorGrade;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fear")
	UMaterial* FearScreenEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Fear")
	UCurveFloat* VisualFearCurve;

	UPROPERTY(EditDefaultsOnly, Category = "Fear")
	UCurveFloat* HUDOpacityFearCurve;

	UPROPERTY(EditDefaultsOnly, Category = "Fear")
	UCurveFloat* FearVignetteCurve;

	UPROPERTY(EditDefaultsOnly, Category = "Fear")
	UCurveFloat* FearDistortionCurve;

	UPROPERTY(EditDefaultsOnly, Category = "Fear")
	float JasonNearMaxRange;

	UPROPERTY(EditDefaultsOnly, Category = "Fear")
	float JasonNearMinRange;

	UPROPERTY(EditDefaultsOnly, Category = "Fear")
	UAnimMontage* CorpseSpottedJumpMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Fear")
	float MaxCorpseSpottedJumpDistance;

private:
	UPROPERTY()
	UMaterialInstanceDynamic* FearScreenEffectInst;

	bool bInLight;
	int NumCorpseSpotted;

	// ~Fear
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	// Combat
public:

	/** Cost to stamina to perform a dodge (any direction) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Combat")
	FStatModifiedValue DodgeStaminaCost;

	/** Cost to stamina to attempt a block */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Combat")
	FStatModifiedValue BlockStaminaCost;

	/** Adrenalin payoff for hitting Jason */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Combat")
	FStatModifiedValue HitKillerStaminaBoost;

	/** Adrenalin payoff for breaking free from Jason's grab */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Combat")
	FStatModifiedValue BreakFreeStaminaBoost;

	/** The modifier to multiply the weapon damage by */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Combat")
	FStatModifiedValue StrengthAttackMod;

	/** The amount to shift the random chance to stun */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Combat")
	FStatModifiedValue StunChanceMod;

	/** The modifier for weapon durablilty */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Combat")
	FStatModifiedValue WeaponDurabilityMod;

	/** The modifier for damage when blocking an attack */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Combat")
	FStatModifiedValue BlockingDamageMod;

	/** The modifier for damage when stunned */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Combat")
	FStatModifiedValue StunnedDamageMod;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Combat")
	float HeavyAttackModifier;

	virtual void OnCharacterInView(ASCCharacter* VisibleCharacter) override;
	virtual void OnCharacterOutOfView(ASCCharacter* VisibleCharacter) override;

	UPROPERTY(EditDefaultsOnly, Category = "Counselor|Combat")
	float WoundedStunMod;

	UPROPERTY(EditDefaultsOnly, Category = "Counselor|Combat")
	float RageGrabStunMod;

protected:
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool CanDodge() const;

	void DodgeLeft();
	void DodgeRight();
	void DodgeBack();

	// Dodge implementation
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_Dodge(EDodgeDirection DodgeDirection);

	UFUNCTION(NetMultiCast, Reliable)
	void MULTICAST_OnDodge(EDodgeDirection DodgeDirection);

	virtual void ProcessWeaponHit(const FHitResult& Hit) override;

	virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
	virtual void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) override;

public:
	float GetWeaponDurabilityModifier() const;

	// ~Combat
	////////////////////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////////////////////
	// Abilities
public:
	/** Use this counselor's active ability */
	void UseAbility();

	/** Returns true if we're actively using the sweater ability */
	bool IsUsingSweaterAbility() const;

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_UseAbility();

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_UseAbility();

	/** Adds the specified ability to this counselor's active abilities */
	void ActivateAbility(USCCounselorActiveAbility* Ability);

	/** Gets the total modifier value of the specified ability type */
	float GetAbilityModifier(EAbilityType Type) const;

	/** Checks to see if this counselor has an active ability of the specified type */
	bool IsAbilityActive(EAbilityType Type) const;

	/** Clears any ability of the specified type */
	void RemoveAbility(EAbilityType Type);

	/** Active ability class for this counselor */
	UPROPERTY(EditDefaultsOnly, Category = "Abilities|Active")
	TSubclassOf<USCCounselorActiveAbility> ActiveAbilityClass;

	// Cheat to allow unlimited uses of the sweater (dev builds only)
	UPROPERTY(Replicated)
	bool bUnlimitedSweaterStun;

protected:
	/** Keeps track of active abilities and removes them if they have worn off */
	void UpdateActiveAbilities();

	UPROPERTY(BlueprintReadOnly, Category = "Abilities|Active")
	USCCounselorActiveAbility* ActiveAbility;

	/** Active ability for Pamela's sweater */
	UPROPERTY(BlueprintReadOnly, Category = "Abilities|Active")
	USCCounselorActiveAbility* SweaterAbility;

	UPROPERTY()
	TMap<int64, USCCounselorActiveAbility*> CurrentActiveAbilities;

	UPROPERTY()
	FTimerHandle TimerHandle_SweaterTimer;

	// ~Abilities
	////////////////////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////////////////////
	// Noise Making
public:
	UFUNCTION(BlueprintCallable, Category = "Noise")
	float GetNoiseLevel() const;

	FORCEINLINE float GetMaxNoiseMod() const { return SprintingNoiseMod * 2.f; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "NoiseModifiers")
	float CrouchingNoiseMod;
	UPROPERTY(EditDefaultsOnly, Category = "NoiseModifiers")
	float WalkingNoiseMod;
	UPROPERTY(EditDefaultsOnly, Category = "NoiseModifiers")
	float RunningNoiseMod;
	UPROPERTY(EditDefaultsOnly, Category = "NoiseModifiers")
	float SprintingNoiseMod;
	UPROPERTY(EditDefaultsOnly, Category = "NoiseModifiers")
	float SwimmingNoiseMod;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NoiseModifiers")
	FStatModifiedValue NoiseStatMod;

	// ~Noise Making
	////////////////////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////////////////////
	// Stumbling
public:
	// Animation montage to play when stumbling and sprinting
	UPROPERTY(EditDefaultsOnly, Category = "Counselor|Stumbling")
	UAnimMontage* SprintStumbleAnim;

	// Animation montage to play when stumbling and running
	UPROPERTY(EditDefaultsOnly, Category = "Counselor|Stumbling")
	UAnimMontage* RunStumbleAnim;

	// Animation montage to play when stumbling and walking
	UPROPERTY(EditDefaultsOnly, Category = "Counselor|Stumbling")
	UAnimMontage* WalkStumbleAnim;

	// Animation montage to play when stumbling from running out of stamina
	UPROPERTY(EditDefaultsOnly, Category = "Counselor|Stumbling")
	UAnimMontage* OutOfStaminaStumbleAnim;

	/**
	 * Use to determine how often to check if we should stumble.
	 * Time is used as the fear level [0->1] and the resulting value is the time (in seconds) to wait between stumble checks.
	 * If both Stamina and Fear frequency are set, uses the mean average of the two (sum / 2). If neither is set, checks once a second.
	 * Stumbles will not stack, if you are stumbling when a chance to roll comes up, the roll timer will reset
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Counselor|Stumbling")
	UCurveFloat* StumbleFearFrequecy;

	/**
	 * The fear chance is used to adjust the chance of stumbling on a given roll.
	 * Time is used as the fear level [0->1] and the resulting value is percent chance [0->100] to stumble for this roll.
	 * If we are checking every second and have a 10% chance of stumbling, we will (on average) stumble once every 10 seconds.
	 * Changing this to a 5% will shift the stumble rate to once every 20 seconds (rolling twice a second will have the same effect)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Counselor|Stumbling")
	UCurveFloat* StumbleFearChance;

	/**
	 * Use to determine how often to check if we should stumble.
	 * Time is used as the inverse stamina level [0->1] and the resulting value is the time (in seconds) to wait between stumble checks.
	 * Inverse stamina (1 - Stamina level) is used to support cross use with fear curves
	 * If both Stamina and Fear frequency are set, uses the mean average of the two (sum / 2). If neither is set, checks once a second.
	 * Stumbles will not stack, if you are stumbling when a chance to roll comes up, the roll timer will reset
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Counselor|Stumbling")
	UCurveFloat* StumbleStaminaFrequency;

	/**
	 * The stamina chance is used to adjust the chance of stumbling on a given roll.
	 * Time is used as the inverse stamina level [0->1] and the resulting value is percent chance [0->100] to stumble for this roll.
	 * Inverse stamina (1 - Stamina level) is used to support cross use with fear curves
	 * If we are checking every second and have a 10% chance of stumbling, we will (on average) stumble once every 10 seconds.
	 * Changing this to a 5% will shift the stumble rate to once every 20 seconds (rolling twice a second will have the same effect)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Counselor|Stumbling")
	UCurveFloat* StumbleStaminaChance;

	// Adjusts the stumble chance based on stats, should be a delta value (1.0 for no change, < 1 for less frequent, > 1 for more frequent)
	UPROPERTY(EditDefaultsOnly, Category = "Counselor|Stumbling")
	FStatModifiedValue StumbleStatMod;

	// This will be used as an additive chance to stumble per roll when stumbled
	UPROPERTY(EditDefaultsOnly, Category = "Counselor|Stumbling")
	float StumbleWoundedChance;

	// Time (in seconds) to delay between stumbles
	UPROPERTY(EditDefaultsOnly, Category = "Counselor|Stumbling")
	float StumbleCooldown;

	// If true, the counselor will automatically trip when depleting stamina, regardless of fear
	UPROPERTY(EditDefaultsOnly, Category = "Counselor|Stumbling")
	bool bForceStumbleOnStaminaDepletion;

	// If true, we are playing the stumble animation, automatically turned off when the montage ends
	UPROPERTY(BlueprintReadOnly, Replicated, ReplicatedUsing="OnRep_SetStumbling", Category = "Counselor|Stumbling")
	bool bIsStumbling;
	
	/** Used to play montage on clients when stumbling is true */
	UFUNCTION()
	virtual void OnRep_SetStumbling(bool bWasStumbling);

	/** Calulates the frequency of stumble rolls based on stamina and fear, if both curves are set uses the mean average (sum / 2), if neither is set uses fallback of once per second. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Counselor|Stumbling")
	float EvaluateStumbleFrequency() const;

	/** Caluclates the chance of stumbling based on stamina and fear, if both curves are set uses additive, if neither are set returns 0 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Counselor|Stumbling")
	float EvaluateStumbleChance() const;

	/** Utility function to find out if we should even try stumbling */
	bool CanStumble() const;

	/** Runs checks to see if we should stumble and updates timers (run on server) */
	void UpdateStumbling_SERVER(float DeltaSeconds);

	/** Calls SetStumbling on the server */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetStumbling(bool bStumbling, bool bFromStaminaDepletion = false);

	/** Turns stumbling on or off */
	void SetStumbling(bool bStumbling, bool bFromStaminaDepletion = false);

	/** Called when bIsStumbling is set to true */
	UFUNCTION(BlueprintImplementableEvent, Category = "Counselor|Stumbling")
	void OnStartStumble();

	/** Called when bIsStumbling is set to false */
	UFUNCTION(BlueprintImplementableEvent, Category = "Counselor|Stumbling")
	void OnStopStumble();

private:
	/** Handle for turning stumbling back off when the stumble montage (see StumbleAnim) ends */
	FTimerHandle TimerHandle_Stumble;

	/** Last time we performed a stumble roll, resets EVERY time a new chance comes up (regardless of if we succeed or not, or even if we roll or not) */
	float LastStumbleRollTimestamp;

	/** Last time we successfully stumbled (not always from a roll) */
	float LastStumbleTimestamp;

	// ~Stumbling
	////////////////////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////////////////////
	// Console Commands
public:

	UFUNCTION(exec, BlueprintCallable, Category = "Console")
	void LockStaminaTo(float InStamina, bool Lock = true);
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_LockStaminaTo(float NewStamina, bool Lock);
	UPROPERTY(Replicated)
	bool bLockStamina;

	// ~Console Commands
	////////////////////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////////////////////
	// Ragdoll dragging
public:

	UPROPERTY(EditDefaultsOnly, Category = "Default")
	TArray<FName> GrabbableBones;

	// ~Ragdoll dragging
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	// Hunter interaction

	// Is this a hunter counselor?
	UPROPERTY(EditDefaultsOnly, Category="Hunter")
	bool bIsHunter;

	UPROPERTY(BlueprintReadOnly, Category = "Hunter")
	bool bIsAiming;

	/** Return the result of the sc.infiniteAmmo cvar */
	UFUNCTION()
	bool GetInfiniteAmmo();

	// Text to display when the character spawns in
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hunter")
	FText SpawnText;

	// ~Hunter interaction
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	// Score Events
public:
	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	TSubclassOf<USCScoringEvent> HitJasonScoreEvent;

	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	TSubclassOf<USCScoringEvent> StunJasonScoreEvent;

	// ~Score Events
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	// Badges
public:
	UPROPERTY(EditDefaultsOnly, Category = "Scoring|Badges")
	TSubclassOf<USCStatBadge> MeleeStunnedJasonBadge;

	UPROPERTY(EditDefaultsOnly, Category = "Scoring|Badges")
	TSubclassOf<USCStatBadge> PocketKnifeEscapeBadge;

	UPROPERTY(EditDefaultsOnly, Category = "Scoring|Badges")
	TSubclassOf<USCStatBadge> EscapeGrabNoKnifeBadge;

	UPROPERTY(EditDefaultsOnly, Category = "Scoring|Badges")
	TSubclassOf<USCStatBadge> SweaterStunnedJasonBadge;

	UPROPERTY(EditDefaultsOnly, Category = "Scoring|Badges")
	TSubclassOf<USCStatBadge> EnterJasonsShackBadge;

	// ~Badges
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	// Drop Item handling
private:
	bool CanDropItem() const;

	bool bDropItemPressed;
	float DropItemTimer;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	float DropItemHoldTime;

	UPROPERTY(Replicated, ReplicatedUsing=OnRep_DroppingSmallItem, Transient)
	bool bDroppingSmallItem;

	UPROPERTY(Replicated, ReplicatedUsing=OnRep_DroppingLargeItem, Transient)
	bool bDroppingLargeItem;

public:
	UFUNCTION()
	void DropItemsOnShore();

	// If there is an escape delay for Counselors, drop their items after the delay has elapsed.
	UFUNCTION()
	void OnDelayedDropItem();

	// ~Drop Item handling
	////////////////////////////////////////////////////////////////////////////////
	
	////////////////////////////////////////////////////////////////////////////////
	// For Music and VO Systems
	
	/** Adds friendlies in view into an array. */
	void FriendlyInView(ASCCounselorCharacter* VisibleCharacter);

	/** Removes friendlies in view or out of range from the array. */
	void FriendlyOutOfView(ASCCounselorCharacter* VisibleCharacter);

	/** Sets the pointer to the killer if its in the characters sight. */
	void KillerInView(ASCKillerCharacter* VisibleKiller);

	/** Sets the pointer to the killer if its in the characters sight. */
	void KillerOutOfView();

	FORCEINLINE ASCKillerCharacter* GetKillerInView() const { return KillerInViewPtr; }

protected:
	// Camera shake to play when a dead body is spotted
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UCameraShake> DeadBodyShake;

private:
	/** Checks for dead friendlies in range. */
	void CheckForDeadFriendliesInRange();

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_DeadFriendlySpotted();

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_DeadFriendlySpotted();

	void DeadFriendlySpotted();

	/** Cool when the see dead friendlies cooldown timer is done. */
	void DeadFriendliesCooldownDone();

	/** Array of Friendly Counselors currently in view. */
	UPROPERTY(Transient)
	TArray<ASCCounselorCharacter*> FriendliesInView;

	/** Array of Friendly DEAD Counselors already seen by this character. */
	UPROPERTY(Transient)
	TArray<ASCCounselorCharacter*> AlreadySeenDeadCounselors;

	bool SeenPamelasHead;

	UPROPERTY()
	ASCKillerCharacter* KillerInViewPtr;

	float DeadFriendliesCooldownTime;

	bool bSeeDeadFriendliesCooldown;

	FTimerHandle TimerHandle_SeeDeadFriendliesCooldown;
	// ~For Music and VO Systems
	////////////////////////////////////////////////////////////////////////////////


public:
	void SetSenseEffects(bool Enable);

	UPROPERTY(EditDefaultsOnly, Category = "Sense")
	UParticleSystem* SenseEffectParticle;

protected:
	UPROPERTY()
	UParticleSystemComponent* SenseEffectComponent;

private:
	/////////////////////////////////////////////////////////////////////////////
	// Pamela VO
	bool InShackWarningToJasonPlayed;

	UFUNCTION()
	void OnShackWarningToJasonCooldownDone();

	FTimerHandle TimerHandle_VOCooldown;

	// ~Pamela VO
	/////////////////////////////////////////////////////////////////////////////


	/////////////////////////////////////////////////////////////////////////////
	// Breath
public:
	/** Input bind for when you press the hold breath button. */
	UFUNCTION()
	void OnHoldBreath();

	/** Input bind for when you release the hold breath button. */
	UFUNCTION()
	void OnReleaseBreath();

	/** Server RPC to enable/disable breathing. */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_SetHoldBreath(bool IsHoldingBreath);

	/** The total amount of breath this counselor has. */
	UPROPERTY(EditDefaultsOnly, Category = "Breath")
	float MaxBreath;

	/** How much breath the counselor must have regained before they can hold their breath again. */
	UPROPERTY(EditDefaultsOnly, Category = "Breath")
	float MinHoldBreathThreshold;

	/** How much time must go by after releasing the hold breath button before you can hold it again. */
	UPROPERTY(EditDefaultsOnly, Category = "Breath")
	float MinHoldBreathTimer;

	/** The stat modifier used to modify the decrease of breath. */
	UPROPERTY(EditDefaultsOnly, Category = "Breath")
	FStatModifiedValue BreathHoldModifier;

	/** The stat modifier used to modify the recharge of breath. */
	UPROPERTY(EditDefaultsOnly, Category = "Breath")
	FStatModifiedValue BreathRechargeModifier;

	/** The amount of sound level to subtract while holding your breath in hiding */
	UPROPERTY(EditDefaultsOnly, Category = "Breath")
	FStatModifiedValue BreathNoiseMod;

	/** @return: The amount of breath left in normalized 0-1 space. */
	UFUNCTION(BlueprintPure, Category = "Breath")
	float GetBreathRatio() const { return CurrentBreath / MaxBreath;}

	UFUNCTION(BlueprintPure, Category = "Breath")
	bool CanHoldBreath() const;

	/** Only called from the server, used to update breath. */
	UFUNCTION()
	void UpdateBreath(float DeltaSeconds);

	/** Checks to see if this counselor is holding their breath. */
	bool IsHoldingBreath() { return bIsHoldingBreath; }

protected:
	/** Used to tell if the player is holding down the breath button. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_IsHoldingBreath, Transient, Category = "Breath")
	bool bIsHoldingBreath;

	/** The current amount of breath this counselor has. */
	UPROPERTY(BlueprintReadOnly, Replicated, Transient, Category = "Breath")
	float CurrentBreath;

	/** The function that is called when the breath button position changes. */
	UFUNCTION()
	void OnRep_IsHoldingBreath();

	UPROPERTY(Replicated, Transient)
	bool bCanHoldBreath;

	UPROPERTY()
	float LastBreathTimer;

	UPROPERTY()
	bool bPressedBreath;

	// ~Breath
	/////////////////////////////////////////////////////////////////////////////

public:
	UMaterialInstanceDynamic* GetTeleportScreenEffect() const { return TeleportScreenEffectInst; }
	void SetTeleportScreenEffect(UMaterialInstanceDynamic* TeleportScrrenEffect) { TeleportScreenEffectInst = TeleportScrrenEffect; }
protected:
	UPROPERTY()
	UMaterialInstanceDynamic* TeleportScreenEffectInst;

	/////////////////////////////////////////////////////////////////////////////
	// Perks
public:

	// Checks to see if this counselor is using the specified perk
	UFUNCTION(BlueprintPure, Category = "Perks")
	bool IsUsingPerk(TSubclassOf<USCPerkData> PerkInQuestion) const;

	/** Gets the number of counselors around this counselor within the specified radius */
	int32 GetNumCounselorsAroundMe(const float Radius = 2000.0f) const;

	/** Notifies when we have entered a radio perk modifier volume */
	FORCEINLINE void EnteredRadioPerkModifierVolume(ASCRadio* Radio) { PerkModifyingRadios.AddUnique(Radio); }

	/** Notifies when we have left a radio perk modifier volume */
	FORCEINLINE void LeftRadioPerkModifierVolume(ASCRadio* Radio) { PerkModifyingRadios.Remove(Radio); }

	/** Checks to see if we are in range of a radio with perk modifiers */
	bool IsInRangeOfRadioPlaying();

	// Specific perk classes we need to check for
	// Weapon perks
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Perks")
	TSubclassOf<USCPerkData> FriendshipPerk;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Perks")
	TSubclassOf<USCPerkData> PotentRangerPerk;

	// Fear perks
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Perks")
	TSubclassOf<USCPerkData> TeamworkPerk;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Perks")
	TSubclassOf<USCPerkData> LoneWolfPerk;

	// Driving perks
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor|Perks")
	TSubclassOf<USCPerkData> SpeedDemonPerk;

protected:
	/** Applies any pertinent perks when receiving a player state or player state settings are updated */
	void ApplyPerks();

private:
	/** List of radios we are in range of their perk modifier volume */
	UPROPERTY()
	TArray<ASCRadio*> PerkModifyingRadios;

	/** Debug command to add an active perk to this character */
	void AddActivePerk(const TArray<FString>& Args);

#if !UE_BUILD_SHIPPING

	/** Helper function to register all perk commands */
	void RegisterPerkCommands();

	/** Helper function to unregister all perk commands */
	void UnregisterPerkCommands();

	/** Map of all possible perks */
	TMap<FString, USCPerkData*> PerkClassMapping;

#endif

	// ~Perks
	/////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////
	// Clothing
public:
	// Mesh for the counselor's clothing
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USkeletalMeshComponent* CounselorOutfitComponent;

	// Get the outfit this counselor is wearing
	FORCEINLINE const FSCCounselorOutfit& GetOutfit() const { return CurrentOutfit; }

	/** Uses the default outfit when none is passed along with the player state */
	UFUNCTION(BlueprintCallable, Category = "Clothing")
	void RevertToDefaultOutfit();

	/** Used to set and load the current outfit */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetCurrentOutfit(const FSCCounselorOutfit& NewOutfit, bool bFromMenu = false);

	/** Returns the default clothing class, used to generate an outfit from */
	UFUNCTION(BlueprintPure, Category = "Clothing")
	TSubclassOf<USCClothingData> GetDefaultOutfitClass() const { return DefaultCounselorOutfit; }

	/** Does this counselor have Pamela's sweater on */
	UFUNCTION(BlueprintPure, Category = "Counselor")
	FORCEINLINE bool HasPamelasSweater() const { return bHasPamelasSweater; }

	/** 
	 * Put Pamela's sweater on 
	 * @param Sweater - the sweater we are putting on
	 */
	void PutOnPamelasSweater(ASCPamelaSweater* Sweater);

	/** Take Pamela's sweater off */
	void TakeOffPamelasSweater();

protected:
	// If no outfit is selected by the player state, use this mesh as a fallback
	UPROPERTY(EditDefaultsOnly, Category = "Clothing")
	TSubclassOf<USCClothingData> DefaultCounselorOutfit;

	// Combines a skin texture with an alpha mask texture to generate a base skin texture for the skin material
	UPROPERTY(EditDefaultsOnly, Category = "Clothing")
	TAssetPtr<UMaterialInstance> AlphaMaskApplicationMaterial;

	// Name of the skin texture material parameter to pass the combined texture into the full skin material
	UPROPERTY(EditDefaultsOnly, Category = "Clothing")
	FName AlphaMaskMaterialParameter;

	// Name of the skin texture material parameter to pass the combined texture into the full skin material
	UPROPERTY(EditDefaultsOnly, Category = "Clothing")
	FName SkinTextureMaterialParameter;

	// Material index into the character mesh for the skin (generally 0)
	UPROPERTY(EditDefaultsOnly, Category = "Clothing")
	int32 SkinMaterialIndex;

	// The skin texture generated from the AlphaMaskApplicationMaterial and passed into our skin material
	UPROPERTY(Transient, BlueprintReadOnly)
	UTextureRenderTarget2D* DynamicSkinTexture;

	// Outfit passed along from the player state
	UPROPERTY(EditAnywhere, Replicated, Category = "Clothing")
	FSCCounselorOutfit CurrentOutfit;

	// Normal outfit for when we're not wearing the sweater (ladies only!), need a reference here so it doesn't get cleaned up
	UPROPERTY(Transient)
	USkeletalMesh* StandardOutfit;

	// Sweater outfit for when we're wearing the sweater (ladies only!), need a reference here so it doesn't get cleaned up
	UPROPERTY(Transient)
	USkeletalMesh* SweaterOutfit;

	// Used to make sure the sweater shows up on all clients regardless of connection time
	UPROPERTY(Replicated, ReplicatedUsing=OnRep_WearingSweater, Transient)
	bool bWearingSweater;

	// Handle for the wardrobe callback that's currently handling out clothing loading
	int32 CurrentOutfitLoadingHandle;

	// True when the outfit is fully loaded (we can show our counselor now!)
	bool bAppliedOutfit;

	// True while loading the outfit, prevents recursive calls into DeferredLoadOutfit from the AsyncManager
	bool bLoadingOutfit;

	// Are we wearing the sweater?
	UPROPERTY(Transient)
	bool bHasPamelasSweater;

	// If true, outfit was set from the menu and the character should use their menu pose
	UPROPERTY(Transient)
	bool bUseMenuPose;

	// Timer handle for using the default outfit if none is replicated
	UPROPERTY()
	FTimerHandle TimerHandle_RevertToDefaultOutfit;

	/** Called from the client to ensure the sweater is seen by all clients */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_SetWearingSweater(const bool bIsWearingSweater);

	/** Rep callback for bWearingSweater */
	UFUNCTION()
	void OnRep_WearingSweater();

	/** Called for each material that needs to stream in for the counselor outfit */
	UFUNCTION()
	void DeferredLoadOutfit();

	/** Editor only function to grab a new random outfit for this character */
	UFUNCTION(exec)
	void RandomOutfit();

	/** Should be called anytime the */
	void SetCounselorOutfitMesh(USkeletalMesh* NewSkeletalMesh);

	/** Delegate for whenever the player state settings get updated (clothing, perks, etc.) */
	UFUNCTION()
	void PlayerStateSettingsUpdated(AILLPlayerState* UpdatedPlayerState);

public:
	/** Destroys any outfit assets set by the current outfit to clean up memory */
	void CleanupOutfit();

	// ~Clothing
	/////////////////////////////////////////////////////////////////////////////

public:
	/////////////////////////////////////////////////////////////////////////////
	// Power Cut

	/** Plays a Sound Effect and VO when the power is cut. */
	void PowerCutSFXVO();

	/** Function that is deferred to play the power cut VO. */
	UFUNCTION()
	void PlayPowerCutVO();

	UPROPERTY(EditDefaultsOnly, Category = "Power Cut")
	USoundCue* PowerCutSFX;

private:
	FTimerHandle TimerHandle_DelayVO;

	// ~Power Cut
	/////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////
	// Rear view Cam
public:
	UFUNCTION()
	void OnRearCamPressed();

	UFUNCTION()
	void OnRearCamReleased();

	UFUNCTION(BlueprintImplementableEvent, Category = "RearCam")
	void OnUseRearCam(bool ShouldUse);
	// ~Rear Cam
	/////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////
	// Some bullshit to fix some other bullshit doing some bullshit every frame and injected into the middle of this shitty file
public:
	/** Function used to hide/unhide large items in counselor hands. Should be called from the server. */
	UFUNCTION(BlueprintCallable, Category = "Item")
	void HideLargeItem(bool HideItem) { bHideLargeItemOverride = HideItem;}

protected:
	UPROPERTY(Replicated, Transient)
	bool bHideLargeItemOverride;
	// ~Bullshit
	/////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////
	// Emotes
public:
	// List of emotes to use if none have been saved to the player profile
	UPROPERTY(EditDefaultsOnly, Category = "Emotes")
	TArray<TSubclassOf<USCEmoteData>> DefaultEmotes;

protected:
	// If true, the currently playing emote prevents movement until finished (non-looping)
	bool bEmoteBlocksMovement;

	// Is our emote a looping emote?
	bool bLoopingEmote;

	// If true, we're playing an emote
	bool bEmotePlaying;

	// Montage for our currently playing emote
	UPROPERTY(Transient)
	UAnimMontage* CurrentEmote;

	// Time (in seconds) to wait before showing the emote select when holding the emote button on console
	UPROPERTY(EditDefaultsOnly, Category = "Emotes")
	float EmoteHoldDelayController;

	// Time (in seconds) to wait before showing the emote select when holding the emote button on PC
	UPROPERTY(EditDefaultsOnly, Category = "Emotes")
	float EmoteHoldDelayKeyboard;

	// Timer used to automatically cancel an emote once the montage has finished (non-looping)
	FTimerHandle TimerHandle_Emote;

	// Timer to block movement so we don't instantly cancel looping emotes
	FTimerHandle TimerHandle_EmoteBlockInput;

	// Timer to handle the delay from holding the emote button to showing the emote wheel (allows abilities to use the same button)
	FTimerHandle TimerHandle_ShowEmoteSelect;

	/** Starts the ShowEmoteSelect timer which will call OnShowEmoteSelect */
	UFUNCTION()
	void OnRequestShowEmoteSelect();

	/** Shows emote select wheel through the in game HUD */
	UFUNCTION()
	void OnShowEmoteSelect();

	/** Hides the emote select wheel from the in game HUD */
	UFUNCTION()
	void OnHideEmoteSelect();

	/** Input component to block other input while emote select is up */
	UPROPERTY()
	UInputComponent* EmoteInputComponent;

public:
	/** Makes sure playing the emote make sesne and then calls MULTICAST_PlayEmote */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_PlayEmote(TSubclassOf<USCEmoteData> EmoteClass);

	/** Plays the emote animation on all clients */
	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_PlayEmote(UAnimMontage* EmoteMontage, bool bLooping);

	/** Calls MULTICAST_CancelEmote */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_CancelEmote();

	/** Stops the current emote from playing and resets emote variables */
	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_CancelEmote();

	/** Make sure we can't exploit emotes to get out of bad situations we deserve because of our poor choices earlier that we now regret, and only finally now realize that we just want to dance... like a jerk */
	bool CanPlayEmote() const;

	// ~Emotes
	/////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////
	// AI
public:
	/** Delegate for when this counselor goes into offline bots AI logic */
	UPROPERTY(BlueprintAssignable, Category="AI")
	FOnOfflineAIMode OnOfflineAIMode;

	UPROPERTY(VisibleAnywhere, Category = "Map")
	bool bIsMapEnabled;

	/** @return true if this bot is using offline bots AI logic */
	UFUNCTION(BlueprintPure, Category="AI")
	bool IsUsingOfflineLogic() const;

	/** Is our character playing any conversation VO */
	bool IsPlayingConversationVO() const;

	// ~AI
	/////////////////////////////////////////////////////////////////////////////
};
