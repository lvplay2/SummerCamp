// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "SCContextKillComponent.h"
#include "SCAnimDriverComponent.h"
#include "SCHidingSpot.generated.h"

enum class EILLInteractMethod : uint8;

class AILLCharacter;
class ASCCharacter;
class ASCCounselorCharacter;
class ASCWeapon;

class USCAnimDriverComponent;
class USCContextKillComponent;
class USCInteractComponent;
class USCSpecialMoveComponent;
class USCStatBadge;
class USCStatusIconComponent;

/**
* @enum ESCHidingSpotType
*/
UENUM(BlueprintType)
enum class ESCHidingSpotType : uint8
{
	Armoire,
	Barn,
	Bed,
	Outhouse,
	Tent
};

/**
 * @struct FWeaponSpecificAnim
 */
USTRUCT()
struct FWeaponSpecificAnim
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSubclassOf<ASCWeapon> WeaponClass;

	UPROPERTY(EditAnywhere)
	UAnimSequence* AnimSequence;
};

/**
 * @class ASCHidingSpot
 */
UCLASS()
class SUMMERCAMP_API ASCHidingSpot
: public AActor
{
	GENERATED_BODY()

public:
	ASCHidingSpot(const FObjectInitializer& ObjectInitializer);

	// BEGIN AActor interface
	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// END AActor interface

	/** Called from special move destination reached to allow the hiding spot to animate while entering/exiting */
	UFUNCTION()
	void EnableTick(ASCCharacter* Interactor);

	/**
	 * Decides if the counselor can get in the hiding spot
	 * @param Interacting Character
	 * @param Character location
	 * @param Character Rotation
	 * @return True if can interact, otherwise false
	 */
	UFUNCTION()
	int32 CanCounselorInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation);

	/**
	 * Triggered when interacting with this actor's press interact component
	 * @param Interacting Actor
	 */
	UFUNCTION()
	void OnCounselorInteract(AActor* Interactor, EILLInteractMethod InteractMethod);

	/**
	 * Decides if the killer can searcch the hiding spot
	 * @param Interacting Character
	 * @param Character location
	 * @param Character Rotation
	 * @return True if can interact, otherwise false
	 */
	UFUNCTION()
	int32 CanKillerInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation);

	/**
	 * Triggered when interacting with this actor's press interact component
	 * @param Interacting Actor
	 */
	UFUNCTION()
	void OnKillerInteract(AActor* Interactor, EILLInteractMethod InteractMethod);

	/** Kicks whoever is inside out (without animation) and cleans up any loose data */
	void CancelHiding();

	/** Retrieve the counselor hiding in this hiding spot if any */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	ASCCounselorCharacter* GetHidingCounselor() const { return CounselorInHiding; }

	/** Force interactors control rotation to match passed rotation */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void ForceControlRotation(ASCPlayerController* LocalController, FRotator InRotation);

	/** Retrieve a weapon specific kill animation for the hiding spot */
	UFUNCTION(BlueprintPure, Category = "Context Action")
	UAnimSequence* GetWeaponKillAnim(ASCWeapon* KillerWeapon);

	/** Retrieve a weapon specific search animation for the hiding spot */
	UFUNCTION(BlueprintPure, Category = "Context Action")
	UAnimSequence* GetWeaponSearchAnim(ASCWeapon* KillerWeapon);

	/**
	 * Retrieve the item drop location for this hiding spot
	 * @param Index - the item drop location to get, 0 - 2 return small item locations, any other value returns the large item drop location
	 */
	UFUNCTION()
	FVector GetItemDropLocation(int32 Index = -1, float ZOffset = 0.f);

	UFUNCTION()
	FORCEINLINE USCInteractComponent* GetInteractComponent() const { return InteractComponent; }

	/** Get the base rotation of the hidespot camera */
	FORCEINLINE FRotator GetBaseCameraRotation() const { return HidingCameraBaseTransform.Rotator(); }

	/** Get the location where spectators should watch the kill */
	FORCEINLINE UArrowComponent* GetSpectatorViewingLocation() const { return SpectatorViewingLocation; }

	/**
	 * BP function to set sense materials
	 * @param bEnable - Turn the effects on or off?
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Effects")
	void SetSenseEffects(bool bEnable);

	UFUNCTION(BlueprintPure, Category = "Default")
	ESCHidingSpotType GetHidingSpotType() const { return HidingSpotType; }

	/** @return true if this hiding spot is destroyed */
	FORCEINLINE bool IsDestroyed() const { return bDestroyed; }

	/** @return true if someone is hiding in this hiding spot */
	FORCEINLINE bool IsSomeoneHiding() const { return CounselorInHiding != nullptr; }

	/** @return the world position for the EnterSpecialMoveComponent */
	FVector GetEntranceLocation() const;

protected:
	// Position to put the counselor when they get into the hiding spot
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USceneComponent* CounselorHidingSpotLocation;

	// If in hiding this is the location we will drop the counselors Large item so it doesn't get lost in the hiding spot
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* ItemDropLocationLarge;

	// If in hiding this is the location we will drop the counselors first item so it doesn't get lost in the hiding spot
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* ItemDropLocationSmall01;

	// If in hiding this is the location we will drop the counselors second item so it doesn't get lost in the hiding spot
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* ItemDropLocationSmall02;

	// If in hiding this is the location we will drop the counselors third item so it doesn't get lost in the hiding spot
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* ItemDropLocationSmall03;

	// If we have mirrored the item drop locations
	bool bHasUpdatedItemDropLocations;

	// Maximum value for vignette while fully in hiding
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	float VignetteIntensityMax;

	// Should the character ignore the hiding spot's collision while in hiding?
	UPROPERTY(EditDefaultsOnly, Category = " Default")
	bool bIgnoreCollisionWhileHiding;

	// The hiding spot mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default")
	USkeletalMeshComponent* Mesh;

	// Default hiding spot context kill
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ContextAction")
	USCContextKillComponent* ContextKillComponent;

	/** Callback when context kill destination is reached */
	UFUNCTION()
	void ContextKillDestinationReached(ASCCharacter* Interactor, ASCCharacter* Victim);

	/** Callback when context kill is finished */
	UFUNCTION()
	void ContextKillCompleted(ASCCharacter* Interactor, ASCCharacter* Victim);

	// Counselor interaction component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	USCInteractComponent* InteractComponent;

	// Killer interaction component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	USCInteractComponent* KillerInteractComponent;

	// Killer search interaction componet
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction|Search")
	USCSpecialMoveComponent* SearchComponent;

	// Listen for anim notifies for destruction (search and context kill)
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Context Action")
	USCAnimDriverComponent* DestructionFXDriver;

	// The point the hiding camera will pivot around when changing view rotation
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Context Action|Camera")
	USceneComponent* HidingCameraPivot;

	// The distance away from the pivot the camera should be in cm
	UPROPERTY(EditDefaultsOnly, Category = "Context Action|Camera")
	float HidingCamDistFromPivot;

	// Max Pitch(X) and Yaw(Y) offset from the default hiding camera rotation in degrees
	UPROPERTY(EditDefaultsOnly, Category = "Context Action|Camera")
	FVector2D MaxHidingRotationOffset;

	// Camera location for while in hiding
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* HidingCamera;

	// Location where spectators should watch context kills
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	UArrowComponent* SpectatorViewingLocation;

	// Control rotation facing when exit is complete
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	UArrowComponent* OnExitControlRotation;

	// Force camera control rotation after exiting hiding?
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	bool bForceExitCameraControlRotation;

	// Cached hiding camera transform
	UPROPERTY(BlueprintReadOnly, Category = "Camera")
	FTransform HidingCameraTransform;

	// Cached hiding camera trasform with mirrored relative yaw and xPos
	UPROPERTY(BlueprintReadOnly, Category = "Camera")
	FTransform HidingCameraTransformMirrored;

	UPROPERTY(BlueprintReadWrite, Category = "Camera")
	FTransform HidingCameraBaseTransform;

	UPROPERTY(BlueprintReadWrite, Category = "Camera")
	bool bEnteredFromMirrorSide;

	// Allow the user to rotate the camera while in hiding
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	bool bAllowCameraRotation;

	// Is hiding spot destroyed after a kill happens with it?
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	bool bDestructable;

	UPROPERTY(EditDefaultsOnly, Category = "Default")
	bool bDestroyOnSearch;

	UPROPERTY(ReplicatedUsing = OnRep_Destroyed, Transient, BlueprintReadOnly, Category = "Default")
	bool bDestroyed;

	UPROPERTY()
	bool bBeginDestruction;

	UFUNCTION()
	void OnRep_Destroyed();

	bool bIsHighlightEnabled;

	// List of weapon types and the hiding spot anims to play if a context kill is performed with one
	UPROPERTY(EditDefaultsOnly, Category = "Interaction|Kill")
	TArray<FWeaponSpecificAnim> WeaponSpecificAnims;

	// List of weapon types and the hiding spot anims to play if a search is performed with one
	UPROPERTY(EditDefaultsOnly, Category = "Interaction|Search")
	TArray<FWeaponSpecificAnim> WeaponSpecificSearchAnims;

	/** Override if multiple special moves are added */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	USCSpecialMoveComponent* GetCorrectSpecialMoveComponent(AActor* Interactor);

	/** Override if multiple search specials are added */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	USCSpecialMoveComponent* GetCorrectSearchMoveComponent(AActor* Interactor);

	/** Override if multiple context kills are added */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	USCContextKillComponent* GetCorrectContextKillComponent(AActor* Interactor);

	UFUNCTION()
	void NativeDestroyHidingSpot(FAnimNotifyData NotifyData);

	UFUNCTION(BlueprintImplementableEvent, Category = "Context Action")
	void DestroyHidingSpot();

	UFUNCTION()
	void HideSpotSearchStart(ASCCharacter* Interactor);

	UFUNCTION()
	void HideSpotSearchCompleted(ASCCharacter* Interactor);

	UFUNCTION()
	void HideSpotSearchAborted(ASCCharacter* Interactor);

	UPROPERTY(EditDefaultsOnly)
	ESCHidingSpotType HidingSpotType;

	// Blueprint event to test any blockers attached to the hiding spot. False = unobstructed
	UFUNCTION(BlueprintNativeEvent, Category = "Blockers")
	bool TestBlockers(ASCCounselorCharacter* Counselor);

	// Blueprint event to reset any blockers attached to the hiding spot.
	UFUNCTION(BlueprintNativeEvent, Category = "Blockers")
	void ResetBlockers();
	
// -------------Enter Hiding Spot --------------------------------

	// Special Move for entering the hiding spot
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	USCSpecialMoveComponent* EnterSpecialMove;

	/** Called when the Enter special move starts */
	UFUNCTION()
	void EnterStarted(ASCCharacter* Interactor);

	/** Called when the Enter special move completes */
	UFUNCTION()
	void EnterCompleted(ASCCharacter* Interactor);

	/** Blueprint Event fired off when entering the hiding spot is completed */
	UFUNCTION(BlueprintImplementableEvent, Category = "Hiding Spot")
	void OnEnterCompleted();

	/** Called when we abort Entering */
	UFUNCTION()
	void EnterAborted(ASCCharacter* Interactor);


// -------------Exit Hiding Spot --------------------------------

	/** Notify all clients the character is exiting the hiding spot */
	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_ExitStarted(ASCCharacter* Interactor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void BlueprintExitStarted();

	/** Called when the exit move could not/did not play and we need a valid location to place the counselor */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	USCSpacerCapsuleComponent* GetExitSpacer(ASCCharacter* Interactor) const;

	/** Called when the exit animation is done playing and we don't need to animate the hiding spot anymore */
	UFUNCTION()
	void ExitCompleted();

	/** Called slightly before the exit animation is done to smoothly return the camera to the exiting player */
	UFUNCTION()
	void ReturnCameraToCounselorOnExit(ASCCounselorCharacter* Counselor);

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "UI")
	USCStatusIconComponent* OccupiedIcon;

	void SetVignetteIntensity(float InIntensity);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_SetVignetteIntensity(float InIntensity);

private:
	bool bEntering;
	bool bExiting;

protected:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_CounselorInHiding, Category = "Hiding Spot")
	ASCCounselorCharacter* CounselorInHiding;

	/** Called when CounselorInHiding gets repped */
	UFUNCTION()
	void OnRep_CounselorInHiding();

	void SetCounselorInHiding(ASCCounselorCharacter* InCounselor);

	// ------------- Badges --------------------------------
public:
	// Badge for using a hiding spot
	UPROPERTY(EditDefaultsOnly, Category = "Scoring|Badges")
	TSubclassOf<USCStatBadge> UseHideSpotBadge;

private:
	///////////////////////////////////////////////////////////
	// Timers
	FTimerHandle TimerHandle_Exit;
	FTimerHandle TimerHandle_ReturnCamera;
};
