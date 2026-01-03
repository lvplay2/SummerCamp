// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "AI/Navigation/NavLinkProxy.h"

#include "SCAnimDriverComponent.h"
#include "SCBaseDamageType.h"
#include "SCContextComponent.h"
#include "SCContextKillActor.h"

#include "SCWindow.generated.h"

class ASCCounselorCharacter;
class UNavModifierComponent;
class USCInteractComponent;
class USCSpecialMoveComponent;
class USCSpacerCapsuleComponent;
class USCStatBadge;

enum class EILLInteractMethod : uint8;
enum class EILLHoldInteractionState : uint8;

/**
 * @class USCWindowDiveDamageType
 */
UCLASS()
class USCWindowDiveDamageType
: public USCBaseDamageType
{
	GENERATED_BODY()

public:
	USCWindowDiveDamageType(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	{
		bShouldPlayHitReaction = false;
		bPlayDeathAnim = false;
	}
};

/**
 * @class ASCWindow
 */
UCLASS()
class SUMMERCAMP_API ASCWindow
: public ANavLinkProxy
{
	GENERATED_BODY()

public:
	ASCWindow(const FObjectInitializer& ObjectInitializer);

	// BEGIN AActor interface
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser) override;
	// END AActor interface
	
	// Animation driver for checking if the player is falling a long distance or not
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	USCAnimDriverComponent* FallDriver;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UStaticMeshComponent* FrameMesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UStaticMeshComponent* MoveablePaneMesh;

	// Camera to look through while interacting indoors
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* IndoorCamera;

	// Camera to look through while interacting outdoors
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* OutdoorCamera;

	// Interior Context Kill
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "ContextAction")
	USCContextKillComponent* ContextKillComponent;

	// Exterior Context Kill
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "ContextAction")
	USCContextKillComponent* OuterContextKillComponent;

	// Window Interaction Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	USCInteractComponent* InteractComponent;

	// Animation Driver for window movement
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCAnimDriverComponent* MovementDriver;
	
	// Animation driver for hand IK
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCAnimDriverComponent* IKDriver;

	// Animation driver for window destruction
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCAnimDriverComponent* BreakDriver;

	// The scene component to drive the left hand to via IK
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USceneComponent* LeftHandIKLocation;

	// The scene component to drive the right hand to via IK
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USceneComponent* RightHandIKLocation;

	// Spacer component for the inside of the window
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSpacerCapsuleComponent* InsideBlockerCapsule;

	// Spacer component for the outside of the window
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSpacerCapsuleComponent* OutsideBlockerCapsule;

	// Navigation modifier to specify the NavArea around this window
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AI")
	UNavModifierComponent* NavigationModifier;

	/** Test if a special can be completed and move any counselors out of the landing area. */
	UFUNCTION()
	bool UpdateBlockers(ASCCharacter* Interactor);

	/** Deactivate the blocker spacers */
	UFUNCTION()
	void DeactivateBlockers(FAnimNotifyData NotifyData);

	UFUNCTION()
	int32 CanInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation);

	UFUNCTION()
	bool ShouldHighlight(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation);

	/**
	 * Triggered when interacting with this actor's press interact component
	 * @param Interacting Actor
	 */
	UFUNCTION()
	void OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod);

	/** Called when the hold interaction is either release or finished */
	UFUNCTION()
	void OnHoldStateChanged(AActor* Interactor, EILLHoldInteractionState NewState);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool IsInteractorInside(AActor* Interactor);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_NativeOnWindowDestroyed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Default")
	void OnWindowDestroyed(bool bPlayEffect = true);

	/** Is this window open */
	UFUNCTION(BlueprintPure, Category="Default")
	FORCEINLINE bool IsOpen() const { return bIsOpen; }

	/** Is this window destroyed */
	UFUNCTION(BlueprintPure, Category="Default")
	FORCEINLINE bool IsDestroyed() const { return bIsDestroyed; }

protected:
	// Is this window currently open
	UPROPERTY(ReplicatedUsing = OnRep_IsOpen, BlueprintReadOnly, Category = "Interaction")
	bool bIsOpen;

	// Is this window currently destroyed
	UPROPERTY(ReplicatedUsing = OnRep_IsDestroyed, BlueprintReadOnly, Category = "Interaction")
	bool bIsDestroyed;

	// Should counselors be wounded for diving through broken or closed windows
	UPROPERTY(EditDefaultsOnly, Category = "Interaction|Damage")
	bool bWoundCounselorForDiving;

	// Should a counselor take damage for vaulting through a broken window
	UPROPERTY(EditDefaultsOnly, Category = "Interaction|Damage")
	bool bTakeVaultDamage;

	// Minimum fall height to damage a counselor when exiting through a window
	UPROPERTY(EditDefaultsOnly, Category = "Interaction|Damage")
	float WoundHeight;

	// Minimum height to notify a counselor they will be falling after exiting the window
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float FallHeight;

	// Amount of damage counselors will take after Diving through a broken or closed window if bWoundCounselorsForDiving = true
	UPROPERTY(EditDefaultsOnly, Category = "Interaction|Damage")
	float DiveDamage;

	// Amount of damage counselors will take after vaulting through a broken window if bTakeVaultDamage = true
	UPROPERTY(EditDefaultsOnly, Category = "Interaction|Damage")
	float VaultDamage;

	// Amount of damage to take after falling out a window taht is higher than WoundHeight
	UPROPERTY(EditDefaultsOnly, Category = "Interaction|Damage")
	float HeightDamage;

	// Location of the window pane in the closed position
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	FVector ClosedLocation;

	// Location of the window pane in the open position
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	FVector OpenLocation;

	/** Set the window open state, can only be called on the server */
	void SetOpen(const bool bOpen);

	UFUNCTION()
	void OnRep_IsOpen();

	UFUNCTION()
	void OnRep_IsDestroyed();

	FORCEINLINE bool CanOpen() const { return !bIsOpen && !bIsDestroyed; }
	FORCEINLINE bool CanClose() const { return bIsOpen && !bIsDestroyed; }

	bool CanVault(const AActor* Interactor) const;
	bool CanDive(const AActor* Interactor) const;
	bool CanThrow(const AActor* Interactor) const;

	// Time in seconds it will take the camera to interpolate back to the player
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float CameraReturnLerpTime;

	// Name of the window movement curve in the animation asset
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default")
	FName MovementCurveName;

	// Anim notify event for animating the window movement
	UFUNCTION(BlueprintNativeEvent, Category = "AnimDriverEvent")
	void AnimateWindow(FAnimNotifyData NotifyData);

	// Anim notify event for updating the counselors hand IK
	UFUNCTION(BlueprintNativeEvent, Category = "AnimDriverEvent")
	void UpdateWindowIK(FAnimNotifyData NotifyData);

	// Anim notify for destroying the window when it's dove through
	UFUNCTION(BlueprintNativeEvent, Category = "AnimDriverEvent")
	void BreakWindow(FAnimNotifyData NotifyData);

	// Anim notify event for testing if the character should switch to their falling state
	UFUNCTION(BlueprintNativeEvent, Category = "AnimDriverEvent")
	void FallingCheck(FAnimNotifyData NotifyData);

	bool bIsFalling;
	float FallingStart_Timestamp;

	// The minimum amount of time to stay in the falling state
	UPROPERTY(EditDefaultsOnly)
	float FallingMinTime;

	// Current interactor reference used for IK and open and vault functionality.
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	ASCCounselorCharacter* InteractingCounselor;

	// Will the counselor take damage if they attempt to exit this window
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	bool bWoundOnExit;

	// Will the counselor fall if they attempt to exit this window
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	bool bFallOnExit;

	// -------------Kill --------------------------------
protected:
	/** Called when the kill fails to reach its destination */
	UFUNCTION()
	void KillAborted(ASCCharacter* Interactor, ASCCharacter* Victim);

	// -------------Destroy Window --------------------------------
protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	USCSpecialMoveComponent* DestroyWindowInside_SpecialMoveHandler;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	USCSpecialMoveComponent* DestroyWindowOutside_SpecialMoveHandler;

	UPROPERTY()
	USCAnimDriverComponent* DestructionDriver;

	UFUNCTION()
	void DestroyWindowStarted(ASCCharacter* Interactor);

	UFUNCTION()
	void DestroyWindowComplete(ASCCharacter* Interactor);

	UFUNCTION()
	void DestroyWindowAborted(ASCCharacter* Interactor);

	UFUNCTION()
	void DestroyWindow(FAnimNotifyData NotifyData);

	// -------------Open Window --------------------------------
protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	USCSpecialMoveComponent* CounselorOpenFromInsideComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	USCSpecialMoveComponent* CounselorOpenFromOutsideComponent;

	UFUNCTION()
	void NativeOpenDestinationReached(ASCCharacter* Interactor);

	UFUNCTION(BlueprintImplementableEvent, Category = "OpenInteraction")
	void OpenDestinationReached();

	/** Called when the open special move completes */
	UFUNCTION()
	void NativeOpenCompleted(ASCCharacter* Interactor);

	/** Called when the open window is aborted */
	UFUNCTION()
	void NativeOpenAborted(ASCCharacter* Interactor);

	UFUNCTION(BlueprintImplementableEvent, Category = "OpenInteraction")
	void OpenCompleted();

	// -------------Close Window --------------------------------
protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	USCSpecialMoveComponent* CounselorCloseFromInsideComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	USCSpecialMoveComponent* CounselorCloseFromOutsideComponent;

	UFUNCTION()
	void NativeCloseDestinationReached(ASCCharacter* Interactor);

	UFUNCTION(BlueprintImplementableEvent, Category = "CloseInteraction")
	void CloseDestinationReached();

	/** Called when the close special move completes */
	UFUNCTION()
	void NativeCloseCompleted(ASCCharacter* Interactor);

	/** Called when the close window is aborted */
	UFUNCTION()
	void NativeCloseAborted(ASCCharacter* Interactor);

	UFUNCTION(BlueprintImplementableEvent, Category = "CloseInteraction")
	void CloseCompleted();

	// -------------Vault Through Window --------------------------------
protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	USCSpecialMoveComponent* CounselorVaultFromInsideComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	USCSpecialMoveComponent* CounselorVaultFromOutsideComponent;

	UFUNCTION()
	void NativeVaultDestinationReached(ASCCharacter* Interactor);

	UFUNCTION(BlueprintImplementableEvent, Category = "VaultInteraction")
	void VaultDestinationReached();

	UFUNCTION()
	void NativeVaultStarted(ASCCharacter* Interactor);

	/** Called when the Enter special move completes */
	UFUNCTION()
	void NativeVaultCompleted(ASCCharacter* Interactor);

	/** Called when some dick hits us while we're climbing through the window */
	UFUNCTION()
	void NativeVaultAborted(ASCCharacter* Interactor);

	UFUNCTION(BlueprintImplementableEvent, Category = "VaultInteraction")
	void VaultCompleted();

	// -------------Dive Through Window --------------------------------
protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	USCSpecialMoveComponent* CounselorDiveFromInsideComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	USCSpecialMoveComponent* CounselorDiveFromOutsideComponent;

	UFUNCTION()
	void NativeDiveDestinationReached(ASCCharacter* Interactor);

	UFUNCTION(BlueprintImplementableEvent, Category = "DiveInteraction")
	void DiveDestinationReached();

	UFUNCTION()
	void NativeDiveStarted(ASCCharacter* Interactor);

	/** Called when the Enter special move completes */
	UFUNCTION()
	void NativeDiveCompleted(ASCCharacter* Interactor);

	/** Called when some dick hits us while we're diving through the window */
	UFUNCTION()
	void NativeDiveAborted(ASCCharacter* Interactor);

	UFUNCTION(BlueprintImplementableEvent, Category = "DiveInteraction")
	void DiveCompleted();

	/** Resets window AND interactor data related to diving interaction. */
	void ResetDiveInteraction(ASCCharacter* const Interactor);

	/** Calculates and deals damage to player */
	void DealDamageToInteractor(ASCCharacter* const Interactor);

	// Sound effect to play when opening the window
	UPROPERTY(EditDefaultsOnly, Category = "Sound Effects")
	USoundCue* WindowOpenSound;

	// Sound effect to play when closing the window
	UPROPERTY(EditDefaultsOnly, Category = "Sound Effects")
	USoundCue* WindowCloseSound;

	// Used for determining whether or not the player should take damage during an interaction abort.
	bool bBrokenByDive;

private:
	float GetMontagePlayRate(ASCCounselorCharacter* Counselor) const;

	// ------------- Badges --------------------------------
public:
	UPROPERTY(EditDefaultsOnly, Category = "Scoring|Badges")
	TSubclassOf<USCStatBadge> ClosedWindowJumpBadge;

	UPROPERTY(EditDefaultsOnly, Category = "Scoring|Badges")
	TSubclassOf<USCStatBadge> SecondStoryWindowJumpBadge;
};
