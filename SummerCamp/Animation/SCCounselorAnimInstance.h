// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Animation/SCAnimInstance.h"
#include "SCCounselorCharacter.h"
#include "SCGun.h"
#include "SCCounselorAnimInstance.generated.h"

class ASCItem;

/**
 * @enum ESCLandingWeight
 */
UENUM(BlueprintType)
enum class ESCLandingWeight : uint8
{
	Light,
	Medium,
	Heavy,

	MAX UMETA(Hidden)
};

/**
 * @class USCCounselorAnimInstance
 */
UCLASS()
class SUMMERCAMP_API USCCounselorAnimInstance
: public USCAnimInstance
{
	GENERATED_BODY()

public:
	USCCounselorAnimInstance(const FObjectInitializer& ObjectInitializer);

	// Begin UAnimInstance interface
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	// End UAnimInstance interface

	// Begin SCAnimInstance interface
	virtual float GetMovementPlaybackRate() const override;
	// End SCAnimInstance interface

	// Max value for fear to still use the look at rotation
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	float MaxLookAtFear;

	// Upcast of our owner to make accessing data easier (may be NULL because Persona is a JERK)
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Default")
	ASCCounselorCharacter* Counselor;

	// Are we a boy or a girl?
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Default")
	bool bIsFemale;

	// Are we in a wheelchair?
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Default")
	bool bInWheelchair;

	// Are we driving or just along for the ride?
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Default")
	bool bIsDriver;

	// Blend value for the look at rotation
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Default")
	float TargetLookRotationAlpha;

	// Current fear value, range of [0->100]
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Default")
	float FearValue;

	// 1.f if we're blocking, 0.f if we're not
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Combat")
	float BlockWeight;

	// Don't use start/stop transitions if we're in a special move (and not grabbed) or playing a montage with root motion
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Combat")
	bool bShouldIgnoreStartStops;

	// If true, the killer has us!
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Combat")
	bool bIsGrabbed;

	// Trying to break free of a grab
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Combat")
	bool bIsStruggling;

	// If true, the player health is below the wounded threshold defined in their pawn
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Combat")
	bool bIsWounded;

	// If true, the player will play the wounded pose
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Combat")
	bool bShouldPlayWounded;

	// Recommended position for our left hand
	UPROPERTY(Transient, BlueprintReadOnly, Category = "IKData")
	FVector LeftHandIKLocation;

	// Recommended rotation for our left hand
	UPROPERTY(Transient, BlueprintReadOnly, Category = "IKData")
	FRotator LeftHandIKRotation;

	// Alpha blend for left hand IK, disabled when performing an action
	UPROPERTY(Transient, BlueprintReadOnly, Category = "IKData")
	float LeftHandIKAlpha;

	// Recommended position for our right hand
	UPROPERTY(Transient, BlueprintReadOnly, Category = "IKData")
	FVector RightHandIKLocation;

	// Recommended rotation for our right hand
	UPROPERTY(Transient, BlueprintReadOnly, Category = "IKData")
	FRotator RightHandIKRotation;

	// Alpha blend for right hand IK, disabled when holding a large item or performing an action
	UPROPERTY(Transient, BlueprintReadOnly, Category = "IKData")
	float RightHandIKAlpha;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	UAnimMontage* DodgeLeftMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	UAnimMontage* DodgeRightMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	UAnimMontage* DodgeBackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	UAnimMontage* PickupSmallHighMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	UAnimMontage* PickupSmallMiddleMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	UAnimMontage* PickupSmallLowMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	UAnimMontage* PickupSmallCrouchMiddleMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	UAnimMontage* PickupSmallCrouchLowMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	UAnimMontage* PickupLargeHighMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	UAnimMontage* PickupLargeMiddleMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	UAnimMontage* PickupLargeLowMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	UAnimMontage* PickupLargeCrouchMiddleMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	UAnimMontage* PickupLargeCrouchLowMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	UAnimMontage* DropLargeItemMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	UAnimMontage* DropSmallItemMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	UAnimMontage* SweaterAbilityMontage;

	// Height (in cm) from the floor that is optimal for a high pickup animation to play (will NOT play while crouched)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	float HighPickupHeight;

	// Height (in cm) from the floor that is optimal for a medium pickup animation to play
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	float MiddlePickupHeight;

	// Height (in cm) from the floor that is optimal for a low pickup animation to play
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	float LowPickupHeight;

	UPROPERTY(Transient)
	bool bUsingSmallItem;

	/**
	 * Plays pickup animation based on character stance, item height and item size
	 * @param ItemLocation - World item postion (actor base location)
	 * @param bLargeItem - Is large item or small item?
	 * @return Animation length (in seconds), 0 if no animation was played
	 */
	float PlayPickupItem(const FVector& ItemLocation, bool bLargeItem);
	
	void PlayUseItemAnim(ASCItem* Item);

	void PlayDodge(EDodgeDirection DodgeDirection);

	FORCEINLINE bool IsDodging() const { return bIsDodging; }

	UFUNCTION(BlueprintCallable, Category = "IKData")
	void UpdateHandIK(const FVector& LeftHandLocation, const FVector& RightHandLocation);

	/** Clears DestroyItem_TimerHandle to prevent using an item */
	void CancelUseItem();

protected:
	UFUNCTION(BlueprintCallable, Category = "Item")
	void OnUseItem();

	UFUNCTION()
	void OnDestroyItem();

	UFUNCTION(BlueprintCallable, Category = "Item")
	void OnPickupItem();

	UFUNCTION(BlueprintCallable, Category = "Item")
	void OnDropSmallItem();

	UFUNCTION(BlueprintCallable, Category = "Item")
	void OnDropLargeItem();

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Combat")
	bool bIsDodging;

	// What speed (in cm/s) was the crouch animation authored at?
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float CrouchAuthoredMovementSpeed;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Vehicle")
	float SteeringValue;

public:
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Vehicle")
	bool bIsInCar;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "Vehicle")
	bool bIsInBoat;

	void ForceOutOfVehicle();

private:
	FTimerHandle Dodge_TimerHandle;
	FTimerHandle DestroyItem_TimerHandle;

	///////////////////////////////////////////////////
	// Weapon Info
protected:
	// Amount of blend to use while holding a weapon and moving (input is DesiredMoveSpeedState as a float)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	UCurveFloat* WeaponBoneBlendCurve;

	// How quickly to blend between WeaponBoneBlendCurve values
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float WeaponBoneBlendInterpSpeed;

	// Do we have a weapon or just a normal large item?
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon")
	bool bHasWeapon;

	// Is our weapon two handed?
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon")
	bool bTwoHanded;

	// Are we aiming our weapon (probably a gun then)
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon")
	bool bIsAiming;

	// Is our weapon a gun?
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon")
	bool bHasGun;

	// Gets the gun type from ASCGun
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon")
	ESCGunType GunType;

	// How much to blend in our weapon grip animation
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon")
	float WeaponBoneBlend;

	// Aim value for where we're looking with our gun
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon")
	float AimPitch;

	///////////////////////////////////////////////////
	// Item Additive Animations
protected:
	// Allows blending upperbody item blend animations in/out while in a montage
	UPROPERTY(EditDefaultsOnly, Category = "Item")
	FName ItemAlphaCurveName;

	// Item additive to use if none is provided by our large item (or we don't have a large item)
	UPROPERTY(EditDefaultsOnly, Category = "Item")
	UAnimSequenceBase* DefaultItemIdleAddtive;

	// Current upper body large item additive animation
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item")
	UAnimSequenceBase* ItemIdleAddtive;

	// 0.f if the large item is a weapon, set by the ItemAlphaCurveName curve when a montage is playing
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item")
	float UpperBodyItemAlpha;

	///////////////////////////////////////////////////
	// In Air
protected:
	// How long should we fall for a light landing?
	UPROPERTY(EditDefaultsOnly, Category = "InAir")
	float LightFallAirTime;

	// How long should we fall for a medium landing?
	UPROPERTY(EditDefaultsOnly, Category = "InAir")
	float FallAirTime;

	// How long should we fall for a heavy landing?
	UPROPERTY(EditDefaultsOnly, Category = "InAir")
	float HeavyFallAirTime;

	// Are we landing?
	UPROPERTY(Transient, BlueprintReadWrite, Category = "InAir")
	bool bIsLanding;

	// How hard did we land?
	UPROPERTY(Transient, BlueprintReadOnly, Category = "InAir")
	ESCLandingWeight LandingWeight;

	// Were we landing?
	UPROPERTY(Transient, BlueprintReadOnly, Category = "InAir")
	bool bWasLanding;

	// Are we still falling? Controlled by different logic than bIsFalling
	UPROPERTY(Transient, BlueprintReadWrite, Category = "InAir")
	bool bIsInAir;

	///////////////////////////////////////////////////
	// Fear
protected:
	// If FearValue is above this value, bShouldPlayFear will be true
	UPROPERTY(EditDefaultsOnly, Category = "Fear")
	float LowFearLimit;

	// If FearValue is above this value, bShouldPlayHeavyFear will be true
	UPROPERTY(EditDefaultsOnly, Category = "Fear")
	float HighFearLimit;

	// Should we play our low fear animation?
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Fear")
	bool bShouldPlayFear;

	// Should we play our high fear animation?
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Fear")
	bool bShouldPlayHeavyFear;

	///////////////////////////////////////////////////
	// State Machine Transitions
protected:
	// Speed (in cm/s) to switch between swimming and idle
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "State Machine")
	float SwimStateTransitionSpeed;

	// Result of Speed > SwimStateTransitionSpeed
	UPROPERTY(Transient, BlueprintReadOnly, Category = "State Machine")
	bool bSwimState_IdleToSwimming;
};
