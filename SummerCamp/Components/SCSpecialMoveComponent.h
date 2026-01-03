// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Components/SceneComponent.h"

#include "SCSpecialMoveComponent.generated.h"

class USCSpecialMove_SoloMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSpecialMoveEvent, class ASCCharacter*, Interactor);

/**
 * @class USCSpecialMoveComponent
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SUMMERCAMP_API USCSpecialMoveComponent
: public USceneComponent
{
	GENERATED_UCLASS_BODY()

public:
	// Begin UActorComponent interface
#if WITH_EDITOR
	virtual void CheckForErrors() override;
#endif
	// End UActorComponent interface

	// Begin USceneComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// End USceneComponent interface

	/** 
	 * Activate the special move associated with this component 
	 * @param Interactor - The character to play the special move on.
	 * @param MontagePlayRate - the rate at which the montage will play
	 */
	UFUNCTION(BlueprintCallable, Category = "Special")
	void ActivateSpecial(ASCCharacter* Interactor, const float MontagePlayRate = 1.f);

	UFUNCTION()
	void BeginSpecial(ASCCharacter* Interactor, const float MontagePlayRate);

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_BeginSpecial(ASCCharacter* Interactor, const float MontagePlayRate);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_BeginSpecial(ASCCharacter* Interactor, const float MontagePlayRate);

	UFUNCTION()
	void NativeDestinationReached();

	// Called when the interactor has reached the destination but before the action begins
	UPROPERTY(BlueprintAssignable, Category = "Special")
	FSpecialMoveEvent DestinationReached;

	// Called when the special move is started
	UPROPERTY(BlueprintAssignable, Category = "Special")
	FSpecialMoveEvent SpecialStarted;

	// Called when the special move is created
	UPROPERTY(BlueprintAssignable, Category = "Special")
	FSpecialMoveEvent SpecialCreated;

	/** Returns true if any special move is currently playing. */
	FORCEINLINE	bool IsSpecialMoveActive() const { return CurrentSpecialMove != nullptr; }

	/** Native function to support callback from the special move itself and pass it through to our actors */
	UFUNCTION()
	void NativeSpecialComplete();

	// Called when the special move is finished
	UPROPERTY(BlueprintAssignable, Category = "Special")
	FSpecialMoveEvent SpecialComplete;

	/** Native function to support callback from the special move itself and pass it through to our actors */
	UFUNCTION()
	void NativeSpecialAborted();

	// Called when the special move is finished
	UPROPERTY(BlueprintAssignable, Category = "Special")
	FSpecialMoveEvent SpecialAborted;

	/** Native function to support callback from the special move itself and pass it through to our actors */
	UFUNCTION()
	void NativeSpecialSpedUp();

	// Called when the special move is sped up
	UPROPERTY(BlueprintAssignable, Category = "Special")
	FSpecialMoveEvent SpecialSpedUp;

	UFUNCTION(BlueprintPure, Category = "Special")
	ASCCharacter* GetInteractor() const { return CurrentInteractor; }

	UPROPERTY(EditAnywhere, Category = "Special")
	bool bClearDelegatesOnFinish;

	UPROPERTY(EditAnywhere, Category = "Special")
	bool bTakeCameraControl;

	/** Gets the length (in seconds) of the montage we want to play on the character, returns 0 if montage cannot be played */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Special")
	float GetMontageLength(const ASCCharacter* Interactor) const;

	/** Activate a special interaction camera */
	UFUNCTION()
	void PlayInteractionCamera(ASCCharacter* Interactor);

	/** Stop any special interaction cameras that are playing */
	UFUNCTION()
	void StopInteractionCamera(ASCCharacter* Interactor);

	/** Returns the specific special move we want to play on this interactor */
	TSubclassOf<USCSpecialMove_SoloMontage> GetDesiredSpecialMove(const ASCCharacter* Interactor) const;

protected:
	// Default special move
	UPROPERTY(EditDefaultsOnly, Category = "Special")
	TSubclassOf<USCSpecialMove_SoloMontage> SpecialMove;

	// If true, will try to find a match in WeaponSpecificMoves, if that fails or this is false, uses SpecialMove as the default
	UPROPERTY(EditDefaultsOnly, Category = "Special")
	bool bUseWeaponSpecificMoves;

	// List of moves to use based on the weapon the SCCharacter is holding
	UPROPERTY(EditDefaultsOnly, Category = "Special", meta=(EditCondition="bUseWeaponSpecificMoves"))
	TArray<FWeaponSpecificSpecial> WeaponSpecificMoves;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default")
	bool bEnabled;

	UPROPERTY(Transient)
	ASCCharacter* CurrentInteractor;

	UPROPERTY(Transient)
	USCSpecialMove_SoloMontage* CurrentSpecialMove;

	// The Special interaction camera that is currently playing
	UPROPERTY(Transient)
	class USCSplineCamera* PlayingCamera;

	// Cached list of SplineCameras parented to this component
	UPROPERTY(Transient)
	TArray<USCSplineCamera*> AttachedSplineCameras;

	// Time to take when blending from player controller to spline cam.
	UPROPERTY(EditDefaultsOnly, Category = "Special")
	float BlendInTime;

	// Time to take after special complete to return the camera to player controller
	UPROPERTY(EditDefaultsOnly, Category = "Special")
	float BlendBackTime;

	// Do we want to update the player control rotation to match our spline cam rotation before returning?
	UPROPERTY(EditDefaultsOnly, Category = "Special")
	bool bForceNewCameraRotation;

	// If true, performs a ray trace against WorldStatic to set our desired destination to the ground
	UPROPERTY(EditDefaultsOnly, Category = "Special", meta=(PinHiddenByDefault))
	bool bSnapToGround;

	// Distance (in cm, +/-) to search against WorldStatic for ground to snap to (bSnapToGround must be true)
	UPROPERTY(EditDefaultsOnly, Category = "Special", meta=(UIMin = "1.0", ClampMin = "1.0", EditCondition="bSnapToGround"))
	float SnapToGroundDistance;

	// Set when bSnapToGround is true and this special move starts (world space)
	FVector SnapLocation;

	bool ClientInPosition;
	bool ServerInPosition;

public:
	/** Gets the desired location for this component, taking snap to ground into consideration */
	FVector GetDesiredLocation(const AActor* Interactor) const;
};
