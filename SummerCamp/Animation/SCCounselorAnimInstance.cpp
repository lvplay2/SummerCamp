// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCCounselorAnimInstance.h"

#include "ILLIKFinderComponent.h"

#include "SCCharacterMovement.h"
#include "SCCounselorCharacter.h"
#include "SCDriveableVehicle.h"
#include "SCItem.h"
#include "SCPerkData.h"
#include "SCPlayerState.h"

const FName NAME_LeftHandIKCurve(TEXT("LeftHandIK"));
const FName NAME_RightHandIKCurve(TEXT("RightHandIK"));

USCCounselorAnimInstance::USCCounselorAnimInstance(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, MaxLookAtFear(50.f)
, HighPickupHeight(160.f)
, MiddlePickupHeight(100.f)
, LowPickupHeight(0.f)
, CrouchAuthoredMovementSpeed(105.f)

// Weapon Info
, WeaponBoneBlendInterpSpeed(4.f)

// Item Additive Animations
, ItemAlphaCurveName(TEXT("ItemWeightAlpha"))

// InAir
, LightFallAirTime(0.3f)
, FallAirTime(0.5f)
, HeavyFallAirTime(0.75f)

// Fear
, LowFearLimit(50.f)
, HighFearLimit(75.f)

// State Machine Transitions
, SwimStateTransitionSpeed(50.f)
{
}

void USCCounselorAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Counselor = Cast<ASCCounselorCharacter>(GetOwningActor());
	if (Counselor)
	{
		bIsFemale = Counselor->GetIsFemale();
		bInWheelchair = Counselor->IsInWheelchair();
	}
}

void USCCounselorAnimInstance::NativeUninitializeAnimation()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DestroyItem_TimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(Dodge_TimerHandle);
	}

	Super::NativeUninitializeAnimation();
}

void USCCounselorAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!Counselor)
		return;

	bIsDriver = Counselor->IsDriver();
	bIsGrabbed = Counselor->bIsGrabbed;
	bIsStruggling = Counselor->bIsStruggling;
	bIsWounded = Counselor->IsWounded();
	bShouldPlayWounded = bIsWounded && !bInWheelchair;
	BlockWeight = Counselor->IsBlocking() ? 1.f : 0.f;
	FearValue = Counselor->GetFear();
	bShouldIgnoreStartStops = (bIsInSpecialMove && !bIsGrabbed) || bIsMontageUsingRootMotion;
	AimPitch = Counselor->AimPitch;

	// Update Steering Value
	if (ASCDriveableVehicle* Vehicle = Counselor->GetVehicle())
	{
		SteeringValue = Vehicle->GetSteeringValue();
	}
	else
	{
		SteeringValue = 0.f;
	}

	const bool bShouldLookWithCamera = !Counselor->IsInSpecialMove() && !Counselor->IsStunned() && CurrentStance == ESCCharacterStance::Standing && FearValue < MaxLookAtFear;
	TargetLookRotationAlpha = FMath::FInterpTo(TargetLookRotationAlpha, bShouldLookWithCamera ? 1.f : 0.f, DeltaSeconds, 4.f);

	// FIXME: Most of this SHOULD be event driven, right now it's just to get the logic out of blueprint

	// Update Weapon Info
	ASCWeapon* Weapon = Counselor->GetCurrentWeapon();
	if (IsValid(Weapon))
	{
		bHasWeapon = true;
		bTwoHanded = Weapon->bIsTwoHanded;

		ASCGun* Gun = Cast<ASCGun>(Weapon);
		if (IsValid(Gun))
		{
			bHasGun = true;
			GunType = Gun->GunType;
			bIsAiming = Gun->IsAiming();

			if (bIsAiming)
				TargetLookRotationAlpha = 0.f;
		}
		else
		{
			bIsAiming = false;
			bHasGun = false;
		}

		// Flare is handled like every other weapon, other guns however handle the weapon bone differently
		if (!bHasGun || GunType != ESCGunType::Flare)
		{
			const float WeaponBoneBlendCurveValue = WeaponBoneBlendCurve->GetFloatValue(DesiredMoveSpeedState);
			WeaponBoneBlend = FMath::FInterpTo(WeaponBoneBlend, WeaponBoneBlendCurveValue, DeltaSeconds, WeaponBoneBlendInterpSpeed);
		}
		else
		{
			WeaponBoneBlend = 0.f;
		}
	}
	else if (bHasWeapon)
	{
		bHasWeapon = false;
		bTwoHanded = false;
		bIsAiming = false;
		bHasGun = false;
		WeaponBoneBlend = 0.f;
	}

	// Update Item Additive Animations
	ItemIdleAddtive = GetCurrentItemIdlePose();
	if (!IsValid(ItemIdleAddtive))
	{
		ItemIdleAddtive = DefaultItemIdleAddtive;
		UpperBodyItemAlpha = 0.f;
	}
	else
	{
		if (bHasWeapon)
		{
			UpperBodyItemAlpha = 0.f;
		}
		else
		{
			if (Montage_IsPlaying(nullptr))
			{
				UpperBodyItemAlpha = GetUnblendedCurveValue(ItemAlphaCurveName);
			}
			else
			{
				UpperBodyItemAlpha = 1.f;
			}
		}
	}

	// In air
	if (InAirTime >= LightFallAirTime && !bIsGrabbed)
	{
		if (InAirTime >= HeavyFallAirTime)
			LandingWeight = ESCLandingWeight::Heavy;
		else if (InAirTime >= FallAirTime)
			LandingWeight = ESCLandingWeight::Medium;
		else
			LandingWeight = ESCLandingWeight::Light;

		bIsLanding = true;
		bIsInAir = true;
	}
	else if (bIsLanding && (CurrentStance == ESCCharacterStance::Swimming || bIsInSpecialMove))
	{
		bIsLanding = false;
		bIsInAir = false;
	}

	// Don't slide around while landing
	if (bIsLanding != bWasLanding)
	{
		if (Counselor->IsLocallyControlled() && Counselor->GetController())
		{
			if (bIsLanding)
				Counselor->GetController()->SetIgnoreMoveInput(true);
			else
				Counselor->GetController()->SetIgnoreMoveInput(false);
		}

		bWasLanding = bIsLanding;
	}

	// Fear
	bShouldPlayFear = (!bIsAiming && MoveSpeedState < (float)EMovementSpeed::Sprint && FearValue >= LowFearLimit);
	bShouldPlayHeavyFear = (bShouldPlayFear && FearValue >= HighFearLimit);

	// State Machine Transition Cache
	bSwimState_IdleToSwimming = Speed > SwimStateTransitionSpeed;
}

float USCCounselorAnimInstance::GetMovementPlaybackRate() const
{
	const USCCharacterMovement* CharacterMovement = Cast<const USCCharacterMovement>(Counselor->GetCharacterMovement());

#if 0 // Disabled for now per GUN feedback
	// Slow run doesn't have its own animation set, adjust for that manually here
	if (!Counselor->IsSprinting() && Counselor->IsRunning() && Counselor->bStaminaDepleted)
	{
		const float BaseRunSpeed = CharacterMovement->MaxRunSpeed;
		const float SlowRunSpeed = CharacterMovement->MaxSlowRunSpeed;

		return Super::GetMovementPlaybackRate() * (SlowRunSpeed / BaseRunSpeed);
	}
#endif

	// Crouch isn't authored at the design speed, adjust for that manually here
	if (Counselor->IsCrouching())
	{
		const float CrouchedMoveSpeed = CharacterMovement->MaxWalkSpeedCrouched;
		const float CrouchRatio = CrouchedMoveSpeed / CrouchAuthoredMovementSpeed;

		// HACK: Apparently we need to apply this twice? Not sure where the math went wrong.
		return Player->GetSpeedModifier() * CrouchRatio * CrouchRatio;
	}

	return Super::GetMovementPlaybackRate();
}

void USCCounselorAnimInstance::PlayUseItemAnim(ASCItem* Item)
{
	if (!Item)
		return;

	bUsingSmallItem = true;

	if (UAnimMontage* UseItemMontage = Item->GetUseItemMontage(CurrentSkeleton))
	{
		// don't do it again please.
		if (Montage_IsPlaying(UseItemMontage))
			return;

		float MontageLength = Montage_Play(UseItemMontage);
		if (MontageLength > 0.f)
		{
			GetWorld()->GetTimerManager().SetTimer(DestroyItem_TimerHandle, this, &ThisClass::OnDestroyItem, MontageLength, false);
		}
		else
		{
			OnUseItem();
			OnDestroyItem();
		}
	}
	else
	{
		OnUseItem();
		OnDestroyItem();
	}
}

void USCCounselorAnimInstance::CancelUseItem()
{
	if (bUsingSmallItem)
	{
		if (GetWorld())
			GetWorld()->GetTimerManager().ClearTimer(DestroyItem_TimerHandle);

		bUsingSmallItem = false;
	}
}

void USCCounselorAnimInstance::OnUseItem()
{
	if (Counselor && bUsingSmallItem)
	{
		Counselor->OnUseItem();
		bUsingSmallItem = false;
	}
}

void USCCounselorAnimInstance::OnDestroyItem()
{
	if (Counselor)
	{
		// If we haven't triggered the notify do it now.
		if (bUsingSmallItem)
			Counselor->OnUseItem();

		Counselor->OnDestroyItem();
		bUsingSmallItem = false;
	}
}

void USCCounselorAnimInstance::PlayDodge(EDodgeDirection DodgeDirection)
{
	if (Counselor == nullptr)
		return;

	UAnimMontage* DodgeMontage = nullptr;
	switch (DodgeDirection)
	{
	case EDodgeDirection::Left:
		DodgeMontage = DodgeLeftMontage;
		break;
	case EDodgeDirection::Right:
		DodgeMontage = DodgeRightMontage;
		break;
	default: // Just in case
	case EDodgeDirection::Back:
		DodgeMontage = DodgeBackMontage;
		break;
	}

	// Play the appropriate dodge animation
	if (DodgeMontage) // paranoid check
	{
		// Apply perk modifiers
		float DodgeSpeedMod = 0.f;
		if (ASCPlayerState* PS = Counselor ? Counselor->GetPlayerState() : nullptr)
		{
			for (const USCPerkData* Perk : PS->GetActivePerks())
			{
				check(Perk);

				if (!Perk)
					continue;

				DodgeSpeedMod += Perk->DodgeSpeedModifier;
			}
		}

		float MontagePlaySpeed = 1.f + DodgeSpeedMod;

		bIsDodging = true;
		const float MontageLength = Montage_Play(DodgeMontage, MontagePlaySpeed);
		if (MontageLength > 0.f)
		{
			if (CVarDebugCombat.GetValueOnGameThread() > 0 && Counselor->IsLocallyControlled())
			{
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Cyan, FString::Printf(TEXT("Perk Dodge Speed Mod: %.2f"), DodgeSpeedMod), false);
			}

			FTimerDelegate Delegate;
			Delegate.BindLambda([this](){ bIsDodging = false; });
			GetWorld()->GetTimerManager().SetTimer(Dodge_TimerHandle, Delegate, MontageLength, false);
		}
		else
		{
			bIsDodging = false;
		}
	}
}

float USCCounselorAnimInstance::PlayPickupItem(const FVector& ItemLocation, bool bLargeItem)
{
	if (Counselor == nullptr)
		return 0.f;

	if (bLargeItem)
	{
		RightHandIKLocation = ItemLocation;
	}
	else
	{
		LeftHandIKLocation = ItemLocation;
	}

	float MontageLength = 0.f;
	const float FloorHeight = Counselor->GetActorLocation().Z - Counselor->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float ItemHeight = ItemLocation.Z - FloorHeight;

	const float LowDifference = FMath::Abs(ItemHeight - LowPickupHeight);
	const float MiddleDifference = FMath::Abs(ItemHeight - MiddlePickupHeight);

	if (Counselor->CurrentStance == ESCCharacterStance::Standing || Counselor->CurrentStance == ESCCharacterStance::Combat)
	{
		const float HighDifference = FMath::Abs(ItemHeight - HighPickupHeight);

		// Item is close to center
		if (MiddleDifference < LowDifference && MiddleDifference < HighDifference)
			MontageLength = Montage_Play(bLargeItem ? PickupLargeMiddleMontage : PickupSmallMiddleMontage);
		// Item is closer to our high height
		else if (HighDifference < LowDifference)
			MontageLength = Montage_Play(bLargeItem ? PickupLargeHighMontage : PickupSmallHighMontage);
		// Item is closer to our low height
		else
			MontageLength = Montage_Play(bLargeItem ? PickupLargeLowMontage : PickupSmallLowMontage);
	}
	else if (Counselor->CurrentStance == ESCCharacterStance::Crouching)
	{
		// Item is close to center
		if (MiddleDifference < LowDifference)
			MontageLength = Montage_Play(bLargeItem ? PickupLargeCrouchMiddleMontage : PickupSmallCrouchMiddleMontage);
		// Item is closer to our low height
		else
			MontageLength = Montage_Play(bLargeItem ? PickupLargeCrouchLowMontage : PickupSmallCrouchLowMontage);
	}

	if (MontageLength <= 0.f)
	{
		OnPickupItem();
	}

	return MontageLength;
}

void USCCounselorAnimInstance::OnPickupItem()
{
	if (Counselor && Counselor->Role == ROLE_Authority)
	{
		Counselor->AddOrSwapPickingItem();
	}
}

void USCCounselorAnimInstance::OnDropSmallItem()
{
	if (!Counselor)
		return;

	if (Counselor->Role == ROLE_Authority)
	{
		Counselor->DropCurrentSmallItem();
	}
}

void USCCounselorAnimInstance::OnDropLargeItem()
{
	if (!Counselor)
		return;

	if (Counselor->Role == ROLE_Authority)
	{
		Counselor->DropEquippedItem();
	}
}

void USCCounselorAnimInstance::UpdateHandIK(const FVector& LeftHandLocation, const FVector& RightHandLocation)
{
	LeftHandIKLocation = LeftHandLocation;
	RightHandIKLocation = RightHandLocation;
}

void USCCounselorAnimInstance::ForceOutOfVehicle()
{
	Montage_Stop(0.f);

	bIsInCar = false;
	bIsInBoat = false;
}
