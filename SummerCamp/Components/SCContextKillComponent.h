// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCContextComponent.h"

#include "SCSplineCamera.h"
#include "SCContextKillComponent.generated.h"

class AILLCharacter;
class ASCCharacter;
class ASCKillerCharacter;

class USCContextKillDamageType;
class USCInteractComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FContextKillDelegate, ASCCharacter*, Interactor, ASCCharacter*, Victim);
DECLARE_DYNAMIC_DELEGATE_RetVal(USceneComponent*, FContextCameraFocusDelegate);

/**
 * @class USCContextKillComponent
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SUMMERCAMP_API USCContextKillComponent
: public USCContextComponent
{
	GENERATED_BODY()

public:
	USCContextKillComponent(const FObjectInitializer& ObjectInitializer);

	// BEGIN UActorComponent interface
#if WITH_EDITOR
	virtual void CheckForErrors() override;
#endif
	virtual void BeginPlay() override;
	// END UActorComponent interface

	/**
	 * Activate context kill with specified killer and victim
	 * @param InKiller - The character performing the kill
	 * @param InVictim - The poor sap gettin' got
	 */
	UFUNCTION(Server, Reliable, WithValidation, Category = "Default")
	void SERVER_ActivateContextKill(ASCCharacter* InKiller, ASCCharacter* InVictim);

	/** Start the context kill from here on all clients to avoid starting before a special is created on a client */
	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_ActivateContextKill(ASCCharacter* InKiller, ASCCharacter* InVictim);

	/** 
	 * Activate context kill with specified killer and his grabbed counselor
	 * @param InKiller - The character performing the kill
	 */
	UFUNCTION(BlueprintCallable, Category = "Default")
	void ActivateGrabKill(ASCKillerCharacter* InKiller);

	/** 
	 * Callback when character reaches their special move destination.
	 * This function only processes data on the server and will call
	 * SyncBeginAction when the actors are in position.
	 */
	UFUNCTION()
	virtual void BeginAction() override;

	/** Start montage on all clients. */
	UFUNCTION()
	virtual void SyncBeginAction();

	/** Called on context kill finished */
	UFUNCTION()
	virtual void EndAction() override;

	/** Sets the Killer and Special Move for this synced animation */
	void SetKillerAndSpecialMove(ASCCharacter* InKiller, USCSpecialMove_SoloMontage* InSpecialMove);

	/** Sets the Victim and Special Move for this synced animation */
	void SetVictimAndSpecialMove(ASCCharacter* InVictim, USCSpecialMove_SoloMontage* InSpecialMove);

	/** Activate context camera and play the animation */
	UFUNCTION()
	void ActivateKillCam();

	/**
	 * Get the transform location for the kill based on set EContextPositionRequirements.
	 * @param Location - Location of the kill passed by reference to be modified.
	 * @param Rotation - Rotation of the kill passed by reference to be modified.
	 * @param bForKiller - Retrieve the Transform for the killer (true) or victim (false)
	 */
	UFUNCTION()
	bool GetKillTransform(FVector& Location, FRotator& Rotation, const bool bForKiller);

	/**
	 * Retrieve the reference to our victim's special move
	 * @return - USCSpecialMove_SoloMontage reference to Victim's special move.
	 */
	TSubclassOf<USCSpecialMove_SoloMontage> GetVictimSpecialMoveClass(const ASCCharacter* InWeaponHolder = nullptr) const;

	/**
	 * Get Killer's special move class type
	 * @return - Special move for the killer
	 */
	TSubclassOf<USCSpecialMove_SoloMontage> GetKillerSpecialMoveClass(const ASCCharacter* InWeaponHolder = nullptr) const;

	/** 
	 * Can the given character interact with this component.
	 * @param Character - AILLCharacter reference requesting interaction.
	 * @param ViewLocation - The position of the character or camera.
	 * @param ViewRotation - The rotation of the character or camera.
	 */
	UFUNCTION()
	virtual int32 CanInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation);

	/**
	 * Retrieve the reference to our victim
	 * @Return - ASCCounselorCharacter reference to actor being killed
	 */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	ASCCharacter* GetCurrentVictim() const { return Victim; };

	/**
	 * Returns an SCCharacter reference for either the killer or victim if they are locally controlled
	 * Will return null if neither is locally controlled.
	 */
	UFUNCTION(BlueprintPure, Category = "ContextAction")
	ASCCharacter* GetLocalInteractor() const;

	// Does this kill require that the killer has a counselor grabbed?
	UPROPERTY(EditDefaultsOnly, Category = "ContextAction")
	bool bIsGrabKill;

	// Hide the killer weapon while performing the kill?
	UPROPERTY(EditDefaultsOnly, Category = "ContextAction")
	bool bHideWeapon;

	// Callback when the kill is activated.
	UPROPERTY(BlueprintAssignable, Category = "ContextAction")
	FContextKillDelegate KillStarted;

	// Callback when the actors are in position.
	UPROPERTY(BlueprintAssignable, Category = "ContextAction")
	FContextKillDelegate KillDestinationReached;

	// Called just after the kill is finished and cameras are reset and before the interactor references are reset.
	UPROPERTY(BlueprintAssignable, Category = "ContextAction")
	FContextKillDelegate KillComplete;

	// Called if players are unable to reach a kill location
	UPROPERTY(BlueprintAssignable, Category = "ContextAction")
	FContextKillDelegate KillAbort;

	/**
	 * Find a valid Spline camera for this Context kill
	 * (One valid camera is chosen at random and returned)
	 */
	UFUNCTION(BlueprintPure, Category = "Camera")
	USCSplineCamera* GetSplineCamera() const;

	/** Set all cameras attached to this component to inactive state. */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void DisableChildCameras();

	/** Return if the context kill is currently playing */
	UFUNCTION(BlueprintPure, Category = "ContextAction")
	bool IsKillActive() const { return bIsKillActive; }

	FContextCameraFocusDelegate GetContextCameraFocus;

	/** Return if the context kill component is enabled or not */
	UFUNCTION()
	FORCEINLINE bool IsEnabled() const { return bIsEnabled; }

protected:
	// Is this context kill enabled in the world
	UPROPERTY(EditAnywhere, Category = "ContextAction")
	bool bIsEnabled;

	// If true, performs a ray trace against WorldStatic to set our desired destination to the ground
	UPROPERTY(EditDefaultsOnly, Category = "ContextAction|Snapping")
	bool bSnapToGround;

	// Distance (in cm, +/-) to search against WorldStatic for ground to snap to (bSnapToGround must be true)
	UPROPERTY(EditDefaultsOnly, Category = "ContextAction|Snapping")
	float SnapToGroundDistance;

	/** Gets the desired location for this component, taking snap to ground into consideration */
	FVector GetDesiredLocation() const;

	// If this kill is marked as always relevant and bPlayForEveryone everyone in the world will see this kill.
	UPROPERTY(EditAnywhere, Category = "ContextAction")
	bool bPlayForEveryone;

	// Should all other camera on the actor be disabled when the kill takes camera control
	UPROPERTY(EditDefaultsOnly, Category = "ContextAction")
	bool bDisableAllActorCams;

	// Should the counselor be dropped before performing the kill?
	UPROPERTY(EditDefaultsOnly, Category = "ContextAction")
	bool bDropCounselor;

	// Should the counselor be dropped after performing the "kill"?
	UPROPERTY(EditDefaultsOnly, Category = "ContextAction")
	bool bDropCounselorAtEnd;

	// Does the kill require the victim to be attached to the killer?
	UPROPERTY(EditDefaultsOnly, Category = "ContextAction")
	bool AttachVictimToKiller;

	// If true, the character will remain attached to the hand at the end of the action
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ContextAction")
	bool bAttachToHand;

	// Does this context kill require specific special moves for different weapon types?
	UPROPERTY(EditDefaultsOnly, Category = "ContextAction")
	bool bPlayWeaponSpecificKills;

	// List of weapon specific Victim special moves.
	UPROPERTY(EditDefaultsOnly, Category = "ContextAction", meta=(EditCondition="bPlayWeaponSpecificKills"))
	TArray<FWeaponSpecificSpecial> VictimWeaponKillClasses;

	// List of weapon specific Killer special moves.
	UPROPERTY(EditDefaultsOnly, Category = "ContextAction", meta=(EditCondition="bPlayWeaponSpecificKills"))
	TArray<FWeaponSpecificSpecial> KillerWeaponKillClasses;

	// Does the victim require specific location or rotation before performing the kill?
	UPROPERTY(EditDefaultsOnly, Category = "ContextAction")
	EContextPositionReqs VictimPositionRequirement;

	// USCSpecialMove_SoloMontage reference to our victim's special move.
	UPROPERTY(EditDefaultsOnly, Category = "ContextAction")
	TSubclassOf<USCSpecialMove_SoloMontage> VictimSpecialClass;

	// If there are any weapons in this list the kill will only be interactable if the interactor is carrying one of said weapons.
	UPROPERTY(EditDefaultsOnly, Category = "ContextAction")
	TArray<TSubclassOf<ASCWeapon>> WeaponRequirements;

	// Info for this context kill (kill name, scoreboard display, etc...)
	UPROPERTY(EditDefaultsOnly, Category = "ContextAction")
	TSubclassOf<USCContextKillDamageType> KillInfo;

	// Should we use Specific collision settings for the killer and victim while this kill is active?
	UPROPERTY(EditDefaultsOnly, Category = "ContextCollision")
	bool bUseSpecificCollisionSettings;

	// The collision settings to apply to the killer and victim once the kill begins
	UPROPERTY(EditDefaultsOnly, Category = "ContextCollision", meta=(EditCondition="bUseSpecificCollisionSettings"))
	FCollisionResponseContainer InKillCollisionResponses;

	// Should the killer and victim ignore gravity for the duration of the kill?
	UPROPERTY(EditDefaultsOnly, Category = "ContextCollision")
	bool bIgnoreGravity;
	
	// Stored killer collision settings to revert on kill end.
	FCollisionResponseContainer StoredKillerCollision;
	// Stored victim collision settings to rever on kill end.
	FCollisionResponseContainer StoredVictimCollision;

	// Called if the special move is aborted
	UFUNCTION()
	void KillAborted();

private:
	// Victim character reference
	UPROPERTY(Transient)
	ASCCharacter* Victim;

	// Victim activated special move reference
	UPROPERTY(Transient)
	USCSpecialMove_SoloMontage* VictimSpecialMove;

	// How many characters have reached their intended location (killer and victim must both be in position before performing)
	int8 InteractorsInPosition;

	UPROPERTY(Replicated, Transient)
	bool bIsKillActive;

	FVector SnapLocation;

	// Editor Simulation ------------------------------------------------------------
protected:
	UFUNCTION(BlueprintCallable, Category = "Simulation")
	void BeginSimulation();

	UFUNCTION(BlueprintCallable, Category = "Simulation")
	void EndSimulation();

	UFUNCTION()
	void Simulate();

	UPROPERTY(EditAnywhere, Transient, BlueprintReadOnly, Category = "Simulation")
	bool bSimulateInEditor;

	FTimerHandle TimerHandle_Simulate;

	UPROPERTY(EditDefaultsOnly, Category = "Simulation")
	USkeletalMesh* KillerMeshClass;

	UPROPERTY(Transient)
	USkeletalMeshComponent* KillerMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Simulation")
	USkeletalMesh* VictimMeshClass;

	UPROPERTY(Transient)
	USkeletalMeshComponent* VictimMesh;
};
