// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLCharacterAnimInstance.h"
#include "SCCharacter.h"
#include "SCTypes.h"
#include "SCAnimInstance.generated.h"

class ASCCharacter;

/**
 * @enum EMovementState
 */
UENUM(BlueprintType)
enum class EMovementState : uint8
{
	// Standing still, not trying to move
	Idle,
	// Turn in place, used prior to start for Jason
	Turn,
	// Move has been requested, playing animation transition into move
	Start,
	// Moving, no transitions
	Move,
	// Move is no longer requested, playing animation transition into stop
	Stop,

	// NOT IMPLEMENTED: Move direction has changed enough to require a transition from move to move
	Juke,

	MAX UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMovementSpeed : uint8
{
	// Standing still
	Idle,
	// Slow walk
	Walk,
	// Light run (sort of a jog)
	Run,
	// Full out sprint
	Sprint,

	MAX
};

/**
* @struct FMontageArray
* Work around for nested containers and static arrays of containers
*/
USTRUCT(BlueprintType)
struct FMontageArray
{
	GENERATED_USTRUCT_BODY()

	FMontageArray()
	{

	}

	UPROPERTY(EditDefaultsOnly)
	TArray<UAnimMontage*> Montages;
};

/**
 * @class USCAnimInstance
 */
UCLASS()
class SUMMERCAMP_API USCAnimInstance
: public UILLCharacterAnimInstance
{
	GENERATED_BODY()

public:
	USCAnimInstance(const FObjectInitializer& ObjectInitializer);

	// Begin UAnimInstance interface
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeUninitializeAnimation() override;
	// End UAnimInstance interface

	// Upcast of our owner to make accessing data easier (may be NULL because Persona is a JERK)
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Default")
	ASCCharacter* Player;

	/** Get the move speed state as a float */
	virtual float GetMoveSpeedState(const float CurrentSpeed) const;

	/** Get the move speed state as a float */
	UFUNCTION(BlueprintPure, Category = "Movement")
	virtual float GetMoveSpeedState() const;

	/** Gets the movement speed mod to use as a playback speed modifier */
	virtual float GetMovementPlaybackRate() const;

	/** Get our desired move speed when starting (allows characters to add/remove states from the base functionality) */
	virtual EMovementSpeed GetDesiredStartMoveState() const;

	/** Get our desired move speed when stopping (allows characters to add/remove states from the best functionality) */
	virtual EMovementSpeed GetDesiredStopMoveState(const float CurrentSpeed, float& SpeedDifference) const;


	UPROPERTY(Transient, BlueprintReadOnly, Category = "Stance")
	ESCCharacterStance CurrentStance;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Movement")
	float ForwardVelocity;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Movement")
	float RightVelocity;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Movement")
	float Direction;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Movement")
	float Speed;

	// If true, Speed is > 0
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Movement")
	bool bIsMoving;

	// Movement playback speed based on player movement mod
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Movement")
	float MovementPlaybackRate;

	// If true, the character is currently in the air
	UPROPERTY(Transient, EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	bool bIsFalling;

	// Time (in seconds) the player has been falling (0 if bIsFalling is false)
	UPROPERTY(Transient, EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float InAirTime;

	// Speed when we enter the stop state
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Movement")
	float StopSpeed;

	// True when the character is playing a montage that has root motion in it, does not mean we are using it, is not always false when no montage is playing
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Movement")
	bool bIsMontageUsingRootMotion;

	// If true, we're currently in a special move
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Movement")
	bool bIsInSpecialMove;

	// Requested move input, local to the character mesh
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Movement")
	FVector2D RequestedMoveInput;

	// Requested move direction, local to the character mesh. Will not jump the -180/180 boundry while in a start state
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Movement")
	float RequestedMoveDirection;

	// Requested move direction for the start, local to the character mesh. Only set when transitioning into start or turn state
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Movement")
	float RequestedStartDirection;

	// Gametime we started starting at
	float StartStarted_Timestamp;

	// Our active move state this frame, call TransitionMovementState to update
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Movement")
	EMovementState CurrentMoveState;

	// Our move state last frame, only modified by code
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Movement")
	EMovementState PreviousMoveState;

	// The speed the player was requesting when a start begins
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Movement")
	EMovementSpeed DesiredStartTransitionSpeed;

	// The speed the player was requesting when a start begins
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Movement")
	float DesiredStartTransitionSpeedAsFloat;

	// The speed the player was requesting when a stop begins
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Movement")
	EMovementSpeed DesiredStopTransitionSpeed;

	// The speed the player was requesting when a stop begins
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Movement")
	float DesiredStopTransitionSpeedAsFloat;

	/**
	 * Called to move from one state to another, handles enter/exit specifics for each state
	 * @param NewState - The state we're transitioning to
	 * @return If the transition is allowed
	 */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	bool TransitionMovementState(const EMovementState NewState);

	void SetUsingStartStopTransitions(const bool bUseTransitions);

	// Angle in degrees to allow start animation to play without doing a turn in place first. 180+ for no turn in place
	UPROPERTY(Transient, EditDefaultsOnly, Category = "Movement")
	float IgnoreTurnTransitionLimit;

	// Used to calculate lean alpha by dividing the current speed over this value, so at the MaxLeanSpeed the lean will be fully blended in
	UPROPERTY(Transient, EditDefaultsOnly, Category = "Movement")
	float MaxLeanSpeed;

	// Amount of lean to apply to the character based on speed and stance
	UPROPERTY(Transient, EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float LeanAlpha;

	// Lean direction in world space so we can filter it over time
	float LeanDirectionWorld;

	// Constantly blends towards RequestedMoveDirection in [-180,180] range
	UPROPERTY(Transient, EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float LeanDirectionFiltered;

	// Speed to interp lean direction at
	UPROPERTY(Transient, EditDefaultsOnly, Category = "Movement")
	float LeanSmoothSpeed;

	// EMovementSpeed for the character converted to a float
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Movement")
	float MoveSpeedState;

	// Combines MoveSpeedState with DesiredStartTransitionSpeed and DesiredStopTransitionSpeed
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Movement")
	float DesiredMoveSpeedState;

	// Rotation of the actor at the start of the turn state
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Movement")
	FRotator StartTurnRotation;

	// If true, we want to move to the left and should use the appropriate blendspace for it */
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Movement")
	bool bStartingToTheLeft;

	// If true, we're closer to a left foot sync node than a right foot sync node
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Movement")
	bool bOnLeftFoot;

	// Value of bOnLeftFoot when we enter the stop state
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Movement")
	bool bStopOnLeftFoot;

	// If true, character acceleration will be adjusted to match the playing animation
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	bool bUseStartStopTransitions;

	// If true, we are currently using start/stop transitions. Used to smartly turn starts/stops on and off
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Movement")
	bool bUsingStartStopTransitions;

	// If true, will update the mesh root offset and apply it to the character when we stop moving
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	bool bUseRootOffset;

	// Distance to check for a wall when stopping, if one is found will cap the stop speed at BlockMaxStopSpeed, if 0 will be ignored
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float BlockedStopDistance;

	// Maximum speed to stop at when a wall is in front of the player
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	EMovementSpeed BlockedMaxStopSpeed;

	// Rotation (just Yaw) that should be applied to the root while moving
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Movement")
	FRotator MeshRootOffset;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Camera")
	FVector CameraLocation;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Camera")
	FRotator CameraRotation;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "Camera")
	FRotator TargetLookAtRotation;

	// TODO: Move this to SCKillerAnimInstance
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	UAnimMontage* GrabMontage;

	// TODO: Move this to SCCounselorAnimInstance
	UPROPERTY(EditDefaultsOnly, Category = "Swimming|Kill")
	UAnimMontage* DrownFromSwimmingMontage;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat")
	virtual UAnimSequenceBase* GetCurrentItemIdlePose() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat")
	virtual UAnimSequenceBase* GetCurrentWeaponCombatPose() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TArray<UAnimMontage*> BlockMontages;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	FMontageArray HitMontages[(int32)ESCCharacterHitDirection::MAX];

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TArray<UAnimMontage*> BlockingHitMontages;

	// TODO: Add support for more stances in a more flexible manner

	// Animation to play when standing and killed from the front
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	UAnimMontage* FrontDeathMontage;

	// Animation to play when standing and killed from behind
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	UAnimMontage* RearDeathMontage;

	// Animation to play when crouched and killed from behind
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	UAnimMontage* FrontCrouchDeathMontage;

	// Animation to play when crouched and killed from behind
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	UAnimMontage* RearCrouchDeathMontage;

	// Float value for CurrentStance != Combat
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Combat")
	float OutOfCombat;

	/**
	 * Picks a random death animation to play
	 * @return If true, we found a death animation to play
	 */
	bool PlayDeathAnimation(const FVector DamageDirection);

	FORCEINLINE bool IsAttacking() const { return bIsAttacking; }
	FORCEINLINE bool IsHeavyAttack() const { return bIsHeavyAttack; }
	FORCEINLINE bool IsRecoiling() const { return bIsRecoiling; }
	FORCEINLINE bool IsWeaponStuck() const { return bIsWeaponStuck; }

	// TODO: Move this to SCKillerAnimInstance
	UFUNCTION(BlueprintCallable, Category = "Grab")
	void PlayGrabAnim();

	// TODO: Move this to SCKillerAnimInstance
	UFUNCTION(BlueprintCallable, Category = "Grab")
	void StopGrabAnim();

	// TODO: Move this to SCKillerAnimInstance
	UFUNCTION(BlueprintCallable, Category = "Grab")
	bool IsGrabAnimPlaying() const;

	/** When called will turn off all sorts of start/stop nonsense in order to let us locomote like a real pawn for once */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void AllowRootMotion();

	/**
	 * Performs a raytrace to find out what sort of material we're walking on based on the position of the bone (foot?) 
	 * passed in, calls to GetFootstepSound to find out what sound to actually play.
	 */
	UFUNCTION(BlueprintCallable, Category = "Footsteps")
	virtual void HandleFootstep(const FName Bone, const bool bJustLanded = false);

	/** Passes in the material we stepped on in hopes of getting a sound to play back */
	UFUNCTION(BlueprintImplementableEvent, Category = "Footsteps")
	TAssetPtr<USoundBase> GetFootstepSound(const EPhysicalSurface SurfaceType, const bool bJustLanded);

	/** Play Character stun animation */
	UFUNCTION(BlueprintCallable, Category = "Stun")
	void PlayStunAnimation(UAnimMontage* Montage);

	/** End the playing stun animation by setting "EndStun" as next section */
	UFUNCTION(BlueprintCallable, Category = "Stun")
	void EndStunAnimation();

	// The currently playing stun montage
	UPROPERTY(BlueprintReadOnly, Category = "Stun")
	UAnimMontage* StunMontage;

	// Should we end the stun at the next branch?
	UPROPERTY(BlueprintReadOnly, Category = "Stun")
	bool bEndStun;

	FORCEINLINE bool IsStunned() const { return StunMontage != nullptr; }

	/** Callback for when a montage is played, used to force root motion on */
	UFUNCTION()
	void OnMontagePlay(UAnimMontage* Montage);

	/** Play the drown animation when the killer activates underwater kill */
	UFUNCTION()
	float PlayDrown();

	/** Set this charater mesh to ragdoll */
	UFUNCTION(BlueprintCallable, Category = "Ragdoll")
	void SetRagdollPhysics(FName BoneName = NAME_None, bool Simulate = true, float BlendWeight = 1.f, bool SkipCustomPhys = false);

	/** Turns ragdoll on and pauses animation updating */
	UFUNCTION(BlueprintCallable, Category = "Ragdoll")
	void SetFullRagdoll(const bool bEnabled);

	void PlayAttackAnim(bool HeavyAttack = false);
	void PlayRecoilAnim();
	void PlayWeaponImpactAnim();

	int32 ChooseBlockAnim() const;
	void PlayBlockAnim(int32 Index = 0);
	float GetBlockAnimLength(int32 Index);
	void EndBlockAnim(int32 Index = 0);

	int32 ChooseHitAnim(ESCCharacterHitDirection HitDirection, bool bBlocking = false) const;
	void PlayHitAnim(ESCCharacterHitDirection HitDirection, int32 Index = 0, bool bBlocking = false);

protected:
	/** Callback for when PendingFootstepSound has streamed in to play it. */
	void DeferredPlayFootstepSound();

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	uint32 bIsAttacking : 1;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	uint32 bIsHeavyAttack : 1;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	uint32 bIsRecoiling : 1;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	uint32 bIsWeaponStuck : 1;

	TAssetPtr<USoundBase> PendingFootstepSound;
	FVector PendingFootstepLocation;

	// Loudness passed to the AI system for making a noise when walking
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep")
	float WalkFootstepLoudness;

	// Loudness passed to the AI system for making a noise when running
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep")
	float RunFootstepLoudness;

	// Loudness passed to the AI system for making a noise when sprinting
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep")
	float SprintFootstepLoudness;

	// Max acceleration to apply, set from Player->CharacterMovement on Init
	float MaxAcceleration;

	// Max deceleration to apply, set from Player->CharacterMovement on Init
	float BrakingDecelerationWalking;

private:
	FTimerHandle Attack_TimerHandle;
	FTimerHandle Recoil_TimerHandle;
	FTimerHandle Impact_TimerHandle;

	UPROPERTY()
	FAttackMontage CachedAttackData;

	// If true, we've finished rotating for our directional start animation
	bool bDoneRotatingForStart;

	void EnterCombatStance();

	void LeaveCombatStance();

	///////////////////////////////////////////////////
	// State Machine Transition Cache
protected:
	// Result of CurrentMoveState == EMovementState::Idle
	UPROPERTY(Transient, BlueprintReadOnly, Category = "State Machine")
	bool bInIdleState;

	// Result of CurrentMoveState == EMovementState::Move
	UPROPERTY(Transient, BlueprintReadOnly, Category = "State Machine")
	bool bInMoveState;

	// Result of CurrentMoveState == EMovementState::Stop
	UPROPERTY(Transient, BlueprintReadOnly, Category = "State Machine")
	bool bInStopState;

	// Set to true when EnterCombatStance is called, false when LeaveCombatStance is called
	UPROPERTY(Transient, BlueprintReadOnly, Category = "State Machine")
	bool bInCombatStance;

	// Result of (CurrentMoveState == EMovementState::Start) && !bIsMontageUsingRootMotion;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "State Machine")
	bool bStartStop_IdleToStart;

	// Result of (CurrentMoveState == EMovementState::Stop) && (AccelerationCurve < StartToIdleRatio)
	UPROPERTY(Transient, BlueprintReadOnly, Category = "State Machine")
	bool bStartStop_StartToIdle;

	// Used to determine if we should play a stop or blend straight to idle when in the start state (based on acceleration curve)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "State Machine")
	float StartToIdleRatio;

	// Dupe of bStartStop_IdleToStart
	UPROPERTY(Transient, BlueprintReadOnly, Category = "State Machine")
	bool bStartStop_StopToStart;

	// Result of (CurrentMoveState == EMovementState::Stop) && (AccelerationCurve >= StartToIdleRatio)
	UPROPERTY(Transient, BlueprintReadOnly, Category = "State Machine")
	bool bStartStop_StartToStop;
};
