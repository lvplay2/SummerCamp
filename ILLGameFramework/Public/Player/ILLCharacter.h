// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLDynamicCameraComponent.h"
#include "Camera/CameraTypes.h"
#include "GameFramework/Character.h"
#include "ILLCharacter.generated.h"

class AILLCharacter;
class UILLInteractableComponent;
enum class EILLInteractMethod : uint8;

/** Callback delegate for when a special move completes. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnILLCharacterLandedDelegate, const FHitResult&, Hit);

/**
 * @struct FILLCharacterMontage
 */
USTRUCT()
struct ILLGAMEFRAMEWORK_API FILLCharacterMontage
{
	GENERATED_BODY()

	// Montage to be played while viewed from first person
	UPROPERTY(EditDefaultsOnly)
	UAnimMontage* FirstPerson;

	// Montage to be played while viewed from third person
	UPROPERTY(EditDefaultsOnly)
	UAnimMontage* ThirdPerson;
};

/**
 * @struct FILLSkeletonIndependentMontage
 */
USTRUCT()
struct ILLGAMEFRAMEWORK_API FILLSkeletonIndependentMontage
{
	GENERATED_BODY()

	// Class these montages can be played on (can be NULL if bExactMatch is false)
	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<ACharacter> Class;

	// Montages to play for given class
	UPROPERTY(EditDefaultsOnly)
	TArray<FILLCharacterMontage> Montages;

	/** Checks if the classes match up with the given character, calls to CanPlayOnSkeleton if the class is not an exact match */
	bool CanPlayOnCharacter(const AILLCharacter* Character, const bool bExactMatchOnly = false, const int32 Index = 0) const;

	/**
	 * Safety check that this montage will actually play on the skeleton of the character passed in
	 * Outside sources should generally use CanPlayOnCharacter (implicitly calls to this)
	 */
	bool CanPlayOnSkeleton(const AILLCharacter* Character, const int32 Index = 0) const;
};

/**
 * @struct Blend values for changing between cameras
 */
USTRUCT()
struct ILLGAMEFRAMEWORK_API FILLCameraBlendInfo
{
	GENERATED_BODY()

	FILLCameraBlendInfo() : StartTimestamp(0.f), BlendLength(0.f), BlendDelta(0.f), BlendWeight(0.f), BlendWeightNormalized(0.f) { }
	FILLCameraBlendInfo(UILLDynamicCameraComponent* Camera, float Start, float Length, float Delta = 0.f) : StartTimestamp(Start), BlendLength(Length), BlendDelta(Delta), BlendWeight(Delta), BlendWeightNormalized(0.f), DynamicCamera(Camera) { memset(&CameraInfoCache, 0, sizeof(FMinimalViewInfo));  }

	// Time in seconds at which blend was requested
	float StartTimestamp;

	// Time in seconds to take to blend into this camera
	float BlendLength;

	// Internal blend delta for the current frame (managed by ILLCharacter)
	float BlendDelta;

	// BlendDelta as modified by higher stack items
	float BlendWeight;

	// BlendWeight normalized by whole camera stack (managed by ILLCharacter)
	float BlendWeightNormalized;

	// Last frame's camera info, cached for blending from dead objects
	FMinimalViewInfo CameraInfoCache;

	// Pointer to the camera that we're blending from, can be NULL
	UPROPERTY()
	UILLDynamicCameraComponent* DynamicCamera;
};

/**
 * @class AILLCharacter
 */
UCLASS(Abstract)
class ILLGAMEFRAMEWORK_API AILLCharacter
: public ACharacter
{
	GENERATED_UCLASS_BODY()

public:
	// Begin UObject interface
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// End UObject interface

	// Begin AActor interface
	virtual void PostInitializeComponents() override;
	virtual void Destroyed() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void CalcCamera(float DeltaTime, struct FMinimalViewInfo& OutResult) override;
	// End AActor interface

	// Begin APawn interface
	virtual void RecalculateBaseEyeHeight() override;
	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void AddControllerYawInput(float Val) override;
	virtual void AddControllerPitchInput(float Val) override;
	virtual float PlayAnimMontage(class UAnimMontage* AnimMontage, float InPlayRate = 1.f, FName StartSectionName = NAME_None) override;
	virtual void StopAnimMontage(class UAnimMontage* AnimMontage) override;
	virtual void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;
	// End APawn interface

	// Begin ACharacter interface
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void Landed(const FHitResult& Hit) override;
	// End ACharacter interface

	/**
	 * Plays anim montage on both first and third models when possible
	 * @param AnimMontage Montage struct supporting different montages for first or third
	 * @param InPlayRate Rate to play the montages at
	 * @param StartSectionName Name of the section to start the montages at
	 * @return The length of the longer montage (first or third)
	 */
	float PlayAnimMontage(FILLCharacterMontage* AnimMontage, float InPlayRate = 1.f, FName StartSectionName = NAME_None);

	/** @return true if the specified montage is active and playing, or if no montage is specified, return true if ANY montage is active and playing */
	bool IsAnimMontagePlaying(UAnimMontage* AnimMontage) const;

	// Landed event broadcast
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnILLCharacterLandedDelegate OnCharacterLanded;

	/** Called from OnRep_Controller and PossessedBy for simulated Controller handling. */
	virtual void OnReceivedController();

	/** Called from OnRep_PlayerState PossessedBy for simulated PlayerState handling. */
	virtual void OnReceivedPlayerState();
	
	/** Handle any cleanup for player before leaving. */
	virtual void OnPlayerLeave() {}

	//////////////////////////////////////////////////
	// Input
public:
	/** Checks with the player input component to see if we're using a gamepad, always false if not locally controlled */
	virtual bool IsUsingGamepad() const;

	//////////////////////////////////////////////////
	// Meshes
public:
	/** Third person or first person mesh, depending on what IsFirstPerson() returns. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mesh")
	USkeletalMeshComponent* GetPawnMesh() const;

	/** @return First person mesh. */
	FORCEINLINE USkeletalMeshComponent* GetFirstPersonMesh() const { return Mesh1P; }

	/** @return Third person mesh. Alias for GetMesh. */
	FORCEINLINE USkeletalMeshComponent* GetThirdPersonMesh() const { return GetMesh(); }
	
	/**
	 * Change the skeletal mesh of the third-person skeletal mesh component.
	 * @param SkelMesh1P First person mesh.
	 * @param SkelMesh3P Third person mesh.
	 * @param bUpdateVisibility Call to UpdateCharacterMeshVisibility afterwards? Typically false when you want to call that yourself.
	 */
	UFUNCTION(BlueprintCallable, Category="Mesh")
	virtual void SetCharacterMeshes(USkeletalMesh* SkelMesh1P, USkeletalMesh* SkelMesh3P, const bool bUpdateVisibility = true);

	/** Change the anim instance. */
	void SetAnimInstanceClass(UClass* AnimInstance);

	/**
	 * Update the character and inventory mesh visibility states.
	 * Intended as a broadcast function for updating the visibility state of this character and anything attached to it.
	 */
	virtual void UpdateCharacterMeshVisibility();

	/** @returns true if the character should be shown. */
	virtual bool ShouldShowCharacter() const { return true; }

	/** First person mesh component name */
	static const FName Mesh1PComponentName;

protected:
	// Skip the call to UpdateCharacterMeshVisibility in PostInitializeComponents? Set this to true if you intend on doing it yourself later anyways
	bool bSkipPostInitializeUpdateVisiblity;

	// Enable the third person mesh(es) to cast a hidden shadow when IsFirstPerson?
	bool bThirdPersonShadowInFirst;

private:
	// First person mesh
	UPROPERTY(VisibleDefaultsOnly, Category="Mesh")
	USkeletalMeshComponent* Mesh1P;

	//////////////////////////////////////////////////
	// Animations
public:
	/** Extended version of PlayAnimMontage to handle 1P/3P separation. */
	virtual float PlayAnimMontageEx(class UAnimMontage* AnimMontage1P, class UAnimMontage* AnimMontage3P, float InPlayRate = 1.f, FName StartSectionName = NAME_None);

	virtual float PlayAnimMontageEx(FILLCharacterMontage Montage, float InPlayRate = 1.f, FName StartSectionName = NAME_None);

	/** Extended version of StopAnimMontage to handle 1P/3P separation. */
	virtual void StopAnimMontageEx(class UAnimMontage* AnimMontage1P, class UAnimMontage* AnimMontage3P);

	virtual void StopAnimMontageEx(FILLCharacterMontage Montage);

	/** Stop playing all montages. */
	virtual void StopAllAnimMontages();

	//////////////////////////////////////////////////
	// Camera
public:
	/** Toggle third person action. */
	virtual void OnToggleThirdPerson();

	/**
	 * Plays a camera shake when this character is locally controlled and for anyone who has this character as their view target. Use this in simulated code!
	 *
	 * @param Shake Camera shake animation to play.
	 * @param Scale Scalar defining how "intense" to play the anim.
	 * @param PlaySpace Which coordinate system to play the shake in (used for CameraAnims within the shake).
	 * @param UserPlaySpaceRot Matrix used when PlaySpace = CAPS_UserDefined.
	 * @param bPlayForLocalViewers Play the camera shake for any local player controllers that have this character as their view target? (Spectators)
	 */
	UFUNCTION(BlueprintCallable, Category="Camera")
	void PlayCameraShake(TSubclassOf<class UCameraShake> Shake, const float Scale, const ECameraAnimPlaySpace::Type PlaySpace, const FRotator UserPlaySpaceRot, const bool bPlayForLocalViewers = true);

	/** Stops all existing camera shake isntances of Shake class. Use this in simulated code! */
	UFUNCTION(BlueprintCallable, Category="Camera")
	virtual void StopCameraShakeInstances(TSubclassOf<class UCameraShake> Shake);

	/** @return Cached camera information. */
	const FMinimalViewInfo& GetCachedCameraInfo() const;

	/** @return true if this character should be drawn from the first person perspective. NOTE: For remote characters this does *not* mean they are in a first person CAMERA mode (though that can drive this), just that the first person mesh(es) should be active. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mesh")
	virtual bool IsFirstPerson() const;

	/** @return true if this character is the ViewTarget for a local player. */
	bool IsLocalPlayerViewTarget() const;

	/** <GB> [Server+Local] change third person state (server has to know that player is using third person) */
	void SetThirdPerson(bool bNewThirdPerson);

	/**
	 * Hard snaps to new camera (clears full camera stack)
	 * If None is passed in, will revert to default AActor camera handling
	 * @param NewTargetCamera - New camera to use for the player (should be attached to the character)
	 */
	UFUNCTION(BlueprintCallable, Category="Camera")
	void SetActiveCamera(UILLDynamicCameraComponent* NewTargetCamera);

	/**
	 * Blends to new camera over time
	 * @param NewTargetCamera - New camera to use for the player (should be attached to the character)
	 * @param BlendTime - Time in seconds to perform ease in/out blend
	 */
	UFUNCTION(BlueprintCallable, Category="Camera")
	void SetActiveCameraWithBlend(UILLDynamicCameraComponent* NewTargetCamera, float BlendTime = 0.5f);

	void SetAspectRatioOverride(const bool bEnabled, const bool bConstrainRatio, const float AspectRatio = -1.f)
	{
		bOverrideAspectRatio = bEnabled;
		bConstrainAspectRatioOverride = bConstrainRatio;
		AspectRatioOverride = AspectRatio;
	}

protected:
	// Camera stack we're blending between
	TArray<FILLCameraBlendInfo> ActiveCameraStack;

	// Used to override the aspect ratio setting if bOverrideAspectRatio is true
	float AspectRatioOverride;

	// Current eye height, smoothed
	float DesiredEyeHeight;

	// Rate at which BaseEyeHeight interpolates towards DesiredEyeHeight, assign this every time you want to interpolate
	UPROPERTY(Transient)
	float EyeHeightLerpSpeed;

	// Rate at which the BaseEyeHeight interpolates for crouching
	UPROPERTY(EditDefaultsOnly, Category="Camera")
	float CrouchEyeHeightLerpSpeed;

	// Rate at which the BaseEyeHeight interpolates for un-crouching
	UPROPERTY(EditDefaultsOnly, Category="Camera")
	float UnCrouchEyeHeightLerpSpeed;

private:
	// Updated every frame from the controller calling CalcCamera
	FMinimalViewInfo CachedCameraInfo;

	// If true will override the player camera aspect ratio, regardless of source
	uint32 bOverrideAspectRatio : 1;

	// Used to override the constrain aspect ratio setting if bOverrideAspectRatio is true
	uint32 bConstrainAspectRatioOverride : 1;

	// Are we in third person?
	uint8 bInThirdPerson : 1;

	//////////////////////////////////////////////////
	// Interactables
public:
	/** @return true if the interactable manager should even look for something. */
	virtual bool CanInteractAtAll() const;

	/**
	 * Handle request to interact with an interactable component
	 * @param Interactable the interactable we want to interact with.
	 * @param InteractMethod the interaction method we're trying to invoke on the interactable
	 */
	virtual void OnInteract(UILLInteractableComponent* Interactable, EILLInteractMethod InteractMethod) {}

	/**
	 * Server has authorized our interaction, do anything necessary that isn't handled by our OnInteract broadcast in the interactable itself
	 * @param Interactable the interactable we want to interact with.
	 * @param InteractMethod the interaction method we're trying to invoke on the interactable
	 */
	virtual void OnInteractAccepted(UILLInteractableComponent* Interactable, EILLInteractMethod InteractMethod) {}

	//////////////////////////////////////////////////
	// Special moves
public:
	/** @return true if the current SpecialMove can be interrupted by SpecialMove. */
	virtual bool CanAbortCurrentMoveFor(TSubclassOf<class UILLSpecialMoveBase> SpecialMove, AActor* TargetActor = nullptr) const;

	/** @return true if SpecialMove can be performed right now. */
	virtual bool CanStartSpecialMove(TSubclassOf<class UILLSpecialMoveBase> SpecialMove, AActor* TargetActor = nullptr) const;

	/** @return true if this character is in a special move. */
	UFUNCTION(BlueprintPure, Category = "SpecialMoves")
	virtual bool IsInSpecialMove() const { return (ActiveSpecialMove != nullptr); }

	/** Performs SpecialMove if CanStartSpecialMove returns true. */
	virtual void StartSpecialMove(TSubclassOf<class UILLSpecialMoveBase> SpecialMove, AActor* TargetActor = nullptr, const bool bViaReplication = false);

	/** @return the list of special moves we're allowed to do with this character */
	const TArray<TSubclassOf<UILLSpecialMoveBase>>& GetAllowedSpecialMoves() const { return AllowedSpecialMoves; }

	/** Simulated outward notification for when a special move starts. */
	virtual void OnSpecialMoveStarted();

	/** Callback delegate for when our ActiveSpecialMove completes. */
	virtual void OnSpecialMoveCompleted();

protected:
	// Registry of allowed special moves
	UPROPERTY(EditDefaultsOnly, Category="SpecialMoves")
	TArray<TSubclassOf<UILLSpecialMoveBase>> AllowedSpecialMoves;

	// Active special move
	UPROPERTY()
	class UILLSpecialMoveBase* ActiveSpecialMove;

	/** Tells the server that the client wants to start a special move, which then tells everyone. */
	UFUNCTION(Reliable, Server, WithValidation)
	virtual void ServerStartSpecialMove(TSubclassOf<class UILLSpecialMoveBase> SpecialMove, AActor* TargetActor);

	/** Notifies clients to start this special move. */
	UFUNCTION(Reliable, NetMulticast)
	virtual void ClientsStartSpecialMove(TSubclassOf<class UILLSpecialMoveBase> SpecialMove, AActor* TargetActor);

	//////////////////////////////////////////////////
	// Fidgets
public:
	/** Returns true if a fidget is currently allowed */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="Animation")
	bool CanFidget() const;

	UFUNCTION(BlueprintCallable, Category="Animation", meta=(AdvancedDisplay="ExtraTime,bCancelActiveFidget"))
	void ResetFidgetTimer(float ExtraTime = 0.f, bool bCancelActiveFidget = true);

protected:
	/** List of fidget animations to play for this character */
	UPROPERTY(EditDefaultsOnly, Category="Animation")
	TArray<FILLCharacterMontage> FidgetAnimations;

	/** Time (in seconds) between trying to play a fidget */
	UPROPERTY(EditDefaultsOnly, Category="Animation")
	float TimeBetweenFidgets;

	/**
	 * Amount of time (+/- in seconds) to adjust the fidget time 
	 * @note With TimeBetweenFidgets at 20 and FidgetTimeRange at 5, the fidget may play between 15 and 25 seconds
	 */
	UPROPERTY(EditDefaultsOnly, Category="Animation")
	float FidgetTimeRange;

	/** When true the fidgets will only pull from the current item (if present), when false fidgets will pull from both character and item */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation")
	uint32 bFidgetPreferItems : 1;

private:
	// Timer for playing fidgets
	FTimerHandle TimerHandle_Fidget;

	// Array index of the next fidget to play, set by calling ResetFidgetTimer (-1 for unset)
	int NextFidget;

	// If true the NextFidget refers to the current item's fidget array, false is our own array
	uint32 bFidgetFromItem : 1;

	// Fidget we're currently playing (used to cancel)
	UPROPERTY()
	FILLCharacterMontage ActiveFidget;

	/** @return List of fidgets. */
	const TArray<FILLCharacterMontage>* GetFidgetList() const;

	/** Callback to play a fidget */
	void TryFidget();
};
