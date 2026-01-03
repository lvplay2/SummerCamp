// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "StatModifiedValue.h"

#include "Animation/AnimInstance.h"
#include "WheeledVehicle.h"
#include "Navigation/PathFollowingComponent.h"

#include "SCDriveableVehicle.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSCVehicleSlamEvent);

enum class EILLHoldInteractionState : uint8;
enum class EILLInteractMethod : uint8;

class AILLCharacter;
class ASCCounselorCharacter;
class ASCEscapeVolume;
class ASCKillerCharacter;
class ASCRepairPart;

class UBehaviorTree;

class USCBaseDamageType;
class USCCameraSplineComponent;
class USCContextKillComponent;
class USCInteractComponent;
class USCNavModifierComponent_Vehicle;
class USCRepairComponent;
class USCScoringEvent;
class USCSpecialMoveComponent;
class USCSplineCamera;
class USCStatBadge;
class USCVehicleSeatComponent;
class USCVehicleStarterComponent;

class USplineComponent;

/**
 * @enum EDriveableVehicleType
 */
UENUM()
enum class EDriveableVehicleType : uint8
{
	Car = 1,
	Boat
};

/**
 * @class ASCDriveableVehicle
 */
UCLASS()
class SUMMERCAMP_API ASCDriveableVehicle
: public AWheeledVehicle
{
	GENERATED_UCLASS_BODY()

public:
	// BEGIN AActor Interface
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetSimpleCollisionCylinder(float& CollisionRadius, float& CollisionHalfHeight) const override;
	virtual void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;
	// END AActor Interface

	// BEGIN APawn Interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void AddControllerYawInput(float Val) override;
	virtual void AddControllerPitchInput(float Val) override;
	virtual void UpdateNavigationRelevance() override;
	// END APawn Interface

	UFUNCTION(NetMulticast, Reliable)
	void MULTI_SetPhysicsState(float PhysicsBlend);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle")
	EDriveableVehicleType VehicleType;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Vehicle")
	USCVehicleStarterComponent* StartVehicleComponent;

	/** This interact component is for the boat */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Vehicle")
	USCInteractComponent* JasonInteractComponent;

	/** This interact component is for the car */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Vehicle")
	USCInteractComponent* JasonCarInteractComponent;

	// Kill volume for characters finding a way to get on top of the vehicle
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UBoxComponent* CheaterKillVolume;

	/** Attempts to start the car */
	UFUNCTION(BlueprintCallable, Category = "Vehicle")
	void StartVehicle(ASCCounselorCharacter* Counselor);

	/** Gets the number of people in this vehicle */
	UFUNCTION(BlueprintPure, Category = "Vehicle")
	int32 GetNumberOfPeopleInVehicle() const;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCNavModifierComponent_Vehicle* NavigationModifier;
	
	///////////////////////////////////////////////////////////////////////////////////
	// Car Audio
public:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	UAudioComponent* HornAudioComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	UAudioComponent* EngineAudioComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	UAudioComponent* RadioAudioComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	UAudioComponent* ImpactAudioComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	USoundCue* EngineStartCue;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	USoundCue* EngineCue;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	USoundCue* EngineDieCue;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	USoundCue* JasonEngineCue;

	UPROPERTY(EditDefaultsOnly, Category = "Audio|Impacts")
	USoundCue* WaterImpactCue;

	UPROPERTY(EditDefaultsOnly, Category = "Audio|Impacts")
	USoundCue* LightImpactCue;

	UPROPERTY(EditDefaultsOnly, Category = "Audio|Impacts")
	USoundCue* HeavyImpactCue;

	UPROPERTY()
	TArray<UCapsuleComponent*> ExitCapsules;

	///////////////////////////////////////////////////////////////////////////////////
	// Repair info
public:
	UFUNCTION()
	void Repair(AActor* Interactor, EILLInteractMethod InteractMethod);

	UFUNCTION()
	void OnInteractStartVehicle(AActor* Interactor, EILLInteractMethod InteractMethod);

	UFUNCTION()
	void OnStartVehicleHoldChanged(AActor* Interactor, EILLHoldInteractionState NewState);

	UFUNCTION()
	int32 CanInteractWithStartVehicle(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation);

	UFUNCTION()
	void OnJasonInteract(AActor* Interactor, EILLInteractMethod InteractMethod);

	UFUNCTION()
	int32 CanJasonDestroyCar(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation);

	UFUNCTION()
	void OnJasonDestroyCar(AActor* Interactor, EILLInteractMethod InteractMethod);

protected:
	UFUNCTION()
	void OnJasonInteractSpecialMoveComplete(ASCCharacter* Interactor);

public:
	UFUNCTION()
	int32 CanJasonInteract(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation);

	/** Setup the dynamic material in blueprint */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Material")
	void SetupDynamicMaterial();

	// Number of required parts to repair vehicle
	int32 NumRequiredParts;
	
	// Number of parts currently installed on the vehicle
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Vehicle")
	int32 NumParts;

	// Property to track repair state of the vehicle
	UPROPERTY(Replicated, ReplicatedUsing = OnRep_IsRepaired, BlueprintReadOnly, Category = "Default")
	bool bIsRepaired;

	/** Sets the repaired state of the vehicle */
	void SetRepaired(const bool bRepaired, const bool bForced = false);

	/** Calls BP function to turn on light materials, particle effects, etc */
	UFUNCTION()
	void OnRep_IsRepaired();

	/** BP Event to handle repairing the vehicle */
	UFUNCTION(BlueprintImplementableEvent, Category = "Default")
	void OnRepaired();

protected:
	// Was this vehicle force repaired?
	bool bWasForceReapired;

public:
	/** @return true if this vehicle was force repaired */
	FORCEINLINE bool WasForceRepaired() const { return bWasForceReapired; }

	/** Force the vehicle to be repaired, so it can be started */
	UFUNCTION(BlueprintCallable, Category="Vehicle")
	void ForceRepair();

	UFUNCTION(BlueprintPure, Category = "Vehicle")
	bool IsPartRepaired(FString PartName);

	/** @return true if the specified part is needed */
	bool IsRepairPartNeeded(TSubclassOf<ASCRepairPart> Part, USCRepairComponent*& AssociatedRepairComponent) const;

	/** @return the number of repairs still needed */
	int32 GetNumRepairsStillNeeded() const { return NumRequiredParts - NumParts; }

	// Property to track the started state of the vehicle
	UPROPERTY(Replicated, ReplicatedUsing = OnRep_IsStarted, BlueprintReadOnly, Category = "Vehicle")
	bool bIsStarted;

	// Property to help objective track the initial start
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Vehicle")
	bool bFirstStartComplete;

	/** Sets the started state of the vehicle */
	void SetStarted(const bool bStarted);

	UFUNCTION()
	void OnRep_IsStarted();

	/** BP Event to handle starting the vehicle */
	UFUNCTION(BlueprintImplementableEvent, Category = "Vehicle")
	void OnStarted();

	/** BP Event to handle the vehicle stalling out */
	UFUNCTION(BlueprintImplementableEvent, Category = "Vehicle")
	void OnStalled();

	/** BP Event to handle vehicle escaping */
	UFUNCTION(BlueprintImplementableEvent, Category = "Vehicle")
	void OnEscaped();

	void SetEscaped(ASCEscapeVolume* EscapeVolume, USplineComponent* ExitPath, float EscapeDelay);
	
	UFUNCTION()
	void EscapeFinished();

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_EscapeFinished();

	// Is this vehicle escaping
	bool bEscaping;

	// Has this vehicle escaped
	bool bEscaped;

	/** Force the vehicle to start even without any parts repaired */
	UFUNCTION(BlueprintCallable, Category="Vehicle")
	void ForceStart();

	/** Sets all seat interactables to enabled */
	void UnlockSeats();

	void CalculateStartTime();

protected:
	UPROPERTY(Transient)
	FTimerHandle TimerHandle_OnEscaped;

	// Minimum base time to restart the car
	UPROPERTY(EditDefaultsOnly, Category = "Vehicle|Restart")
	float MinRestartTime;

	// Maximum base time to restart the car
	UPROPERTY(EditDefaultsOnly, Category = "Vehicle|Restart")
	float MaxRestartTime;

	// Mod applied to restart time based on driver, total time will be: Rand(MinRestartTime, MaxRestartTime) * RestartTimeMod
	UPROPERTY(EditDefaultsOnly, Category = "Vehicle|Restart")
	FStatModifiedValue RestartTimeMod;

	// Assigned when the car stalls based on min/max values, then adjusted by RestartTimeMod anytime a player enters the driver seat (server only)
	UPROPERTY()
	float BaseRestartTime;

	// Actual restart time, replicated out and applied to the StartVehicleComponent anytime a player enters the driver seat
	UPROPERTY(Replicated, ReplicatedUsing = OnRep_RestartTime)
	float RestartTime;

	/** Applies Restart time to StartVehicleComponent's hold time */
	UFUNCTION()
	void OnRep_RestartTime();

	// List of Counselors PlayerStates that have repaired this vehicle
	UPROPERTY()
	TArray<APlayerState*> VehicleRepairers;

	///////////////////////////////////////////////////////////////////////////////////

	UPROPERTY()
	USplineComponent* ExitSpline;

	int32 CurrentSplineIndex;

	void UpdateAIMovement(float DeltaSeconds);

public:
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle")
	TArray<USCVehicleSeatComponent*> Seats;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Vehicle")
	AActor* Driver;

	///////////////////////////////////////////////////////////////////////////////////
	// Driving
public:
	/** Input handler for forward */
	void OnForward(float Val);

	/** Input handler for reverse */
	void OnReverse(float Val);

	/** Combines forward/reverse values into a combined Throttle Value */
	void SetThrottle(float val);

	/** Set throttle on server and replicate that value */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_SetThrottle(float val);

	/** Calls the BP event for particle effects on all clients and handles Vehicle Movement on Local Controller */
	UFUNCTION()
	void OnRep_Throttle();

	/** BP Event to handle particle effects and stuff */
	UFUNCTION(BlueprintImplementableEvent, Category = "Vehicle")
	void OnThrottle(float val);

	/** Input handler for steering */
	void SetSteering(float val);

	/** Set steering value on server and replicate that value */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_SetSteering(float val);

	/** Calls the BP event for steering */
	UFUNCTION()
	void OnRep_Steering();

	/** BP Eevent to handle steering audio logic */
	UFUNCTION(BlueprintImplementableEvent, Category = "Vehicle")
	void OnSteering(float val);

	/** Input handler for Handbrake Engaging */
	void OnHandbrakeDown();

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_OnHandbrakeDown();
	
	/** Input handler for Handbrake Disengaging */
	void OnHandbrakeUp();

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_OnHandbrakeUp();

	UFUNCTION()
	void OnRep_Handbrake();

	UFUNCTION(BlueprintImplementableEvent, Category = "Vehicle")
	void OnHandbrake(bool bOn);

	UFUNCTION()
	void OnDriverExitVehicle();

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_OnDriverExitVehicle();

protected:
	/** Unmodified max engine RPM */
	float DefaultMaxEngineRPM;

	///////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////
	// Killer Collision
public:
	// Collision volume for when driving into the killer
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UBoxComponent* KillerFrontCollisionVolume;

	// Collision volume for when backing into the killer
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UBoxComponent* KillerRearCollisionVolume;

	// Spline for the slam camera to run on
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCCameraSplineComponent* SlamSpline;

	// Camera to use when slamming the car
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSplineCamera* SlamSplineCamera;

	// If a car is going faster than this speed (in m/s) when a killer enters the KillerCollisionVolume, the killer will perform the vehicle slam to stop it
	UPROPERTY(EditDefaultsOnly, Category = "Vehicle|Slam")
	float KillerSlamSpeed;

	/** Returns true if we're being slammed */
	UFUNCTION(BlueprintPure, Category = "Vehicle|Slam")
	FORCEINLINE bool IsBeingSlammed() const { return bIsBeingSlammed; }

	/** @return true if this vehicle is disabled */
	FORCEINLINE bool IsVehicleDisabled() const { return bIsDisabled; }

protected:
	// Animation to play on the killer when they slam they vehicle
	UPROPERTY(EditDefaultsOnly, Category = "Vehicle|Slam")
	UAnimMontage* KillerSlamVehicleMontage;

	// Animation to play on the vehicle when the killer slams it
	UPROPERTY(EditDefaultsOnly, Category = "Vehicle|Slam")
	UAnimMontage* VehicleSlammedMontage;

	// Called when the killer slams this vehicle
	UPROPERTY(BlueprintAssignable, Category = "Vehicle|Slam")
	FSCVehicleSlamEvent OnVehicleSlammed;

	// What angle of impact do we handle for crashing the car
	UPROPERTY(EditDefaultsOnly, Category = "Vehicle|Impact")
	float MaxImpactAngle;

	// How fast the car needs to be moving for a light impact to happen
	UPROPERTY(EditDefaultsOnly, Category = "Vehicle|Impact")
	float MinLightImpactSpeed;

	// How fast the car needs to be moving for a heavy impact and stall to happen
	UPROPERTY(EditDefaultsOnly, Category = "Vehicle|Impact")
	float MinHeavyImpactSpeed;

	// How fast the car needs to be moving for a counselor to die on collision
	UPROPERTY(EditDefaultsOnly, Category = "Vehicle|Impact")
	float MinKillCounselorSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Vehicle|Animation")
	UAnimMontage* StartVehicleMontage;

	// Damage type to use when hitting counseors
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TSubclassOf<USCBaseDamageType> KillCounselorDamageType;

	UPROPERTY(Replicated, Transient, BlueprintReadOnly, Category = "Vehicle|Impact")
	bool bIsDisabled;

private:
	/** Called when the mesh gets an overlap event */
	UFUNCTION()
	void OnMeshOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Called when the mesh stops overlapping */
	UFUNCTION()
	void OnMeshOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** Called when an actor hit is registered */
	UFUNCTION()
	void OnVehicleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/** Called when a character gets on top of the car */
	UFUNCTION()
	void OnCheaterVolumeOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	/** Called when a killer gets in front of the car */
	UFUNCTION()
	void OnKillerFrontCollisionVolumeOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	/** Called when a killer gets behind the car */
	UFUNCTION()
	void OnKillerFrontCollisionVolumeOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** Called when a killer gets in front of the car */
	UFUNCTION()
	void OnKillerRearCollisionVolumeOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	/** Called when a killer gets behind the car */
	UFUNCTION()
	void OnKillerRearCollisionVolumeOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// List of counselors in front of the car (only on the server)
	UPROPERTY()
	TArray<ASCCounselorCharacter*> CounselorsInFrontOfCar;

	// List of counselors behind the car (only on the server)
	UPROPERTY()
	TArray<ASCCounselorCharacter*> CounselorsBehindCar;

	// List of killers in front of the car (only on the server)
	UPROPERTY()
	TArray<ASCKillerCharacter*> KillersInFrontOfCar;

	// List of killers behind the car (only on the server)
	UPROPERTY()
	TArray<ASCKillerCharacter*> KillersBehindCar;

	// If true, the server has a killer in front of the car
	UPROPERTY(Replicated)
	bool bIsFrontBlockedByKiller;

	// If true, the server has a killer behind the car
	UPROPERTY(Replicated)
	bool bIsRearBlockedByKiller;

	/** Stops movement and plays slam animations */
	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_KillerSlamCar(ASCKillerCharacter* Killer);

	// Tracking for car slam
	bool bIsBeingSlammed;

	/** Let's Slam that Car! */
	void StartSlamJam(ASCKillerCharacter* Killer);

	FTimerHandle TimerHandle_SlamStall;
	FTimerHandle TimerHandle_LocalSlamFinish;
	FTimerHandle TimerHandle_SlamFinish;

	///////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////
	// Minimap
public:
	UFUNCTION()
	void ToggleDriverMap();

	UPROPERTY(BlueprintReadOnly, Category = "Driver")
	bool bShowDriverMap;
	///////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////
	// VOIP
	UFUNCTION()
	void StartTalking();

	UFUNCTION()
	void StopTalking();
	///////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////
	// Hood Animation
public:
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* OpenHoodAnim;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* CloseHoodAnim;

	UFUNCTION(BlueprintCallable, Category = "Animation")
	void OpenHood();

	UFUNCTION(BlueprintCallable, Category = "Animation")
	void CloseHood();
	///////////////////////////////////////////////////////////////////////////////////

	void EnableVehicleStarting();

	void EjectCounselor(ASCCounselorCharacter* Counselor, bool bFromCrash);

	/** Returns the world location of an empty capsule component around the vehicle
	  * @return Valid position around the car, Zero Vector if all capsules are blocked */
	FVector GetEmptyCapsuleLocation(const AActor* CounselorToIgnore = nullptr);

	UFUNCTION(BlueprintImplementableEvent, Category = "Camera")
	UCameraComponent* GetFirstPersonCamera();

	UFUNCTION(BlueprintImplementableEvent, Category = "Camera")
	UCameraComponent* GetReverseCamera();

protected:
	/** Input handler for the look behind camera */
	void OnLookBehindPressed();

	/** Input handler for the look behind camera */
	void OnLookBehindReleased();

	/** Updates ActiveCamera to match the current driver inputs */
	void UpdateActiveCamera();

	// True while the look behind button is held down
	bool bLookBehindHeld;

private:
	/** Current active driving camera (forward or reverse?) */
	UPROPERTY()
	UCameraComponent* ActiveCamera;

protected:
	// How submerged do we want the car before ejecting counselors?
	UPROPERTY(EditDefaultsOnly, Category = "Water")
	float MinWaterDepthForSinking;

	// How fast is too fast? m/s
	UPROPERTY(EditDefaultsOnly, Category = "Vehicle")
	float MinUseCarDoorSpeed;

public:
	FORCEINLINE float GetMinUseCarDoorSpeed() const { return MinUseCarDoorSpeed; }

	/** @return true if doors can be interacted with */
	bool CanEnterExitVehicle() const;

protected:
	UFUNCTION(Client, Reliable)
	void CLIENT_SetCameraToCounselor(ASCCounselorCharacter* Counselor);

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle")
	bool bJustStarted;

public:
	UFUNCTION(BlueprintCallable, Category = "Vehicle")
	void PlayAnimMontage(UAnimMontage* Montage);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_PlayAnimMontage(UAnimMontage* Montage);

	UFUNCTION(BlueprintCallable, Category = "Vehicle")
	void StopAnimMontage(UAnimMontage* Montage);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_StopAnimMontage(UAnimMontage* Montage);

protected:
	UFUNCTION()
	void StartHorn();

	UFUNCTION()
	void StopHorn();

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_PlayHorn(bool bPlay);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_PlayHorn(bool bPlay);

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float MaxCameraYawAngle;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float MaxCameraPitchAngle;

	UFUNCTION(BlueprintCallable, Category = "CharacterVO")
	void PlayVO(const FName VOName, ASCCounselorCharacter* Counselor);

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_PlayVO(const FName VOName, ASCCounselorCharacter* Counselor);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_PlayVO(const FName VOName, ASCCounselorCharacter* Counselor);

	void PlayImpactSound(USoundCue* ImpactSound, const FVector& ImpactLocation);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_PlayImpactSound(USoundCue* ImpactSound, const FVector& ImpactLocation);

	/** Function to call from BP to set all vehicle passengers' view to a specific camera */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetPassengerCameras(USCSplineCamera* Camera);

	/** Function to call from BP to return cameras to the passengers */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void ReturnPassengerCameras();

	UPROPERTY(EditDefaultsOnly, Category = "Flipping")
	float FlipAngleThreshold;

private:
	float ForwardVal;
	float ReverseVal;

	UPROPERTY(ReplicatedUsing=OnRep_Throttle)
	float ThrottleVal;

	UPROPERTY(ReplicatedUsing=OnRep_Steering)
	float SteeringVal;

	UPROPERTY(ReplicatedUsing=OnRep_Handbrake)
	uint32 bHandbrake : 1;

	// Rotation rate (degrees/second scaled by input modifier (defaults to 2)) for the camera to handle delta time correctly. Pitch/Yaw/Roll
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	FVector CameraRotateRate;
	
	// Scales the sensitivity of camera rotation when in a vehicle
	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "0.1", ClampMax = "1.0", UIMin = "0.1", UIMax = "1.0"))
	FVector2D CameraRotateScale;

	UPROPERTY(EditDefaultsOnly, Category = "CheaterKill", meta = (ClampMin = "0", ClampMax = "360", UIMin = "0", UIMax = "360"))
	float MaxAngleToKillCheaters;

	float LocalYaw;
	float LocalPitch;

public:
	UFUNCTION(BlueprintPure, Category = "Steering")
	FORCEINLINE float GetSteeringValue() const { return SteeringVal; }

	void ResetControlRotation();

	UPROPERTY(Transient)
	TArray<ASCCounselorCharacter*> CounselorsInteractingWithVehicle;

	/** Boat only variable, hnnnnggg */
	UPROPERTY(Replicated, Transient)
	bool bJasonFlippingBoat;

private:
	UPROPERTY()
	APhysicsVolume* CurrentWaterVolume;

	/////////////////////////////////////////////////////////////////////////////
	// Score Events
public:
	// Score Event for when a part is repaired
	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	TSubclassOf<USCScoringEvent> PartRepairedScoreEvent;

	// Score Event for when the vehicle is fully repaired
	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	TSubclassOf<USCScoringEvent> VehicleRepairedScoreEvent;

	/////////////////////////////////////////////////////////////////////////////
	// Badges
public:
	// Jason slammed the car
	UPROPERTY(EditDefaultsOnly, Category = "Scoring|Badges")
	TSubclassOf<USCStatBadge> CarSlamBadge;

	// Escaped in a car
	UPROPERTY(EditDefaultsOnly, Category = "Scoring|Badges")
	TSubclassOf<USCStatBadge> CarEscapeBadge;

	// Escaped in a boat
	UPROPERTY(EditDefaultsOnly, Category = "Scoring|Badges")
	TSubclassOf<USCStatBadge> BoatEscapeBadge;

	UPROPERTY(EditDefaultsOnly, Category = "Scoring|Badges")
	TSubclassOf<USCStatBadge> BoatFlipBadge;

private:
	/////////////////////////////////////////////////////////////////////////////
	// Radio Music stuff
	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TArray<TAssetPtr<USoundCue>> Songs;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TAssetPtr<USoundCue> StreamerModeSong;

	int32 CurrentSong;
	FTimerHandle TimerHandle_NextTrack;

	// Last frame's speed for use in OnVehicleHit
	float CachedSpeed;

protected:
	UFUNCTION()
	void PlaySong();

	void StartMusic(const TAssetPtr<USoundCue>& SongToPlay, float StartTime);
	void StopMusic();
	void NextTrack();

public:
	/** Checks to see if we are in Streamer mode. */
	bool IsStreamerMode();

	void UpdateStreamerMode(bool bStreamerMode);

private:
	float DisablePhysicsOnBeginTimer;
	bool bDisablePhysicsOnBegin;

	////////////////////////////////////////////////////////////////////////////
	// Throttle Input Timers
	float ValidThrottleTime;

	UPROPERTY(EditDefaultsOnly, Category = "Vehicle")
	float MinValidThrottleTime;

public:
	/** Track how long the vehicle doesn't move while holding down throttle input. Used in Jason interaction with vehicle seats. */
	FORCEINLINE bool GetValidThrottle() const { return ValidThrottleTime < MinValidThrottleTime; }

	////////////////////////////////////////////////////////////////////////////
	// AI

	// BehaviorTree to use for cars
	UPROPERTY(EditAnywhere, Category = "AI")
	UBehaviorTree* BehaviorTree;

	/** @return the position in world space where we should start our forward ray traces from. */
	FORCEINLINE FVector GetFrontRayTraceStartPosition() const { return FrontRayTraceStart->GetComponentLocation(); }

	/** @return the position in world space where we should start our rear ray traces from. */
	FORCEINLINE FVector GetRearRayTraceStartPosition() const { return RearRayTraceStart->GetComponentLocation(); }

protected:
	// Starting location on the front of the vehicle for AI to preform ray traces to check for possible collisions
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="AI")
	USceneComponent* FrontRayTraceStart;

	// Starting location on the rear of the vehicle for AI to preform ray traces to check for possible collisions
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AI")
	USceneComponent* RearRayTraceStart;
};
