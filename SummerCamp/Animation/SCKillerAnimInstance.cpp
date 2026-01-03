// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCKillerAnimInstance.h"

#include "SCWeapon.h"
#include "SCKillerCharacter.h"
#include "SCCounselorCharacter.h"

USCKillerAnimInstance::USCKillerAnimInstance(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void USCKillerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Killer = Cast<ASCKillerCharacter>(GetOwningActor());
	WeaponIdle = GetCurrentWeaponCombatPose();
}

void USCKillerAnimInstance::NativeUninitializeAnimation()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(GrabKillTimer);
	}

	Super::NativeUninitializeAnimation();
}

void USCKillerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!Killer)
		return;

	GrabWeight = Killer->GrabValue;
	bIsRageEnabled = Killer->IsRageActive();
	bIsCounselorStruggling = Killer->GetGrabbedCounselor() ? Killer->GetGrabbedCounselor()->bIsStruggling : false;
	bIsGrabEnterPlaying = Montage_IsPlaying(GrabMontage);

	// Blueprint Cache
	bShouldIgnoreMovementStateMachine = bIsInSpecialMove || bIsMontageUsingRootMotion;
	GrabWeight_Neck01 = GrabWeight >= 1.f ? 1.f : 0.6f;
	GrabWeight_Spine02 = 1.f - GrabWeight_Neck01;
}

float USCKillerAnimInstance::GetMoveSpeedState(const float CurrentSpeed) const
{
	if (CurrentSpeed <= KINDA_SMALL_NUMBER)
		return (float)EMovementSpeed::Idle;

	if (Player->IsSprinting())
	{
		if (bCanRun)
		{
			return (float)EMovementSpeed::Sprint;
		}
		else
		{
			return (float)EMovementSpeed::Run;
		}
	}

	return (float)EMovementSpeed::Walk;
}

EMovementSpeed USCKillerAnimInstance::GetDesiredStartMoveState() const
{
	EMovementSpeed MoveSpeed = Super::GetDesiredStartMoveState();

	if (bCanRun && MoveSpeed == EMovementSpeed::Run)
		MoveSpeed = EMovementSpeed::Sprint;
	else if (!bCanRun && MoveSpeed == EMovementSpeed::Sprint)
		MoveSpeed = EMovementSpeed::Run;

	return MoveSpeed;
}

EMovementSpeed USCKillerAnimInstance::GetDesiredStopMoveState(const float CurrentSpeed, float& SpeedDifference) const
{
	EMovementSpeed MoveSpeed = Super::GetDesiredStopMoveState(CurrentSpeed, SpeedDifference);

	if (bCanRun && MoveSpeed == EMovementSpeed::Run)
		MoveSpeed = EMovementSpeed::Sprint;
	else if (!bCanRun && MoveSpeed == EMovementSpeed::Sprint)
		MoveSpeed = EMovementSpeed::Run;

	// Don't bother updating SpeedDifference for now, it's currently just a debug variable

	return MoveSpeed;
}

UAnimSequenceBase* USCKillerAnimInstance::GetCurrentWeaponCombatPose() const
{
	if (Killer == nullptr)
		return nullptr;

	const ASCWeapon* CurrentWeapon = Killer->GetCurrentWeapon();
	if (CurrentWeapon == nullptr)
	{
		TSoftClassPtr<ASCWeapon> DefaultWeaponClass = Killer->GetDefaultWeaponClass();
		DefaultWeaponClass.LoadSynchronous(); // TODO: Make Async
		CurrentWeapon = DefaultWeaponClass.Get() ? Cast<ASCWeapon>(DefaultWeaponClass.Get()->GetDefaultObject()) : nullptr;
	}

	if (CurrentWeapon)
	{
		return CurrentWeapon->GetCombatIdlePose(CurrentSkeleton, false);
	}

	return nullptr;
}

int32 USCKillerAnimInstance::ChooseGrabKillAnim() const
{
	return FMath::RandRange(0, FMath::Max(0, GrabKillMontages.Num() - 1));
}

void USCKillerAnimInstance::PlayGrabKillAnim(int32 Index/* = 0*/)
{
	if (GrabKillMontages.Num() <= 0)
	{
		KillGrabbedCounselor();
		return;
	}

	bIsGrabKilling = true;

	float MontageLength = Montage_Play(GrabKillMontages[Index]);

	if (MontageLength > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(GrabKillTimer, FTimerDelegate::CreateUObject(this, &USCKillerAnimInstance::OnEndGrabKill), MontageLength, false);
	}
	else
	{
		OnEndGrabKill();
	}
}

void USCKillerAnimInstance::OnEndGrabKill()
{
	bIsGrabKilling = false;
}

void USCKillerAnimInstance::KillGrabbedCounselor()
{
	if (Killer)
	{
		if (ASCCounselorCharacter* Counselor = Killer->GetGrabbedCounselor())
		{
			Killer->DropCounselor();
			Counselor->KilledBy(Killer);
		}
	}
}

void USCKillerAnimInstance::ClearContextKillForGrab()
{
	if (!Killer)
		return;

	// Only valid when we're playing a pickup animation!
	if (!ensure(GrabWeight > 0.f))
	{
		return;
	}

	// Clear for the killer
	Killer->ClearContextKill();

	// And the counselor being grabbed
	if (ASCCounselorCharacter* Counselor = Killer->GetGrabbedCounselor())
		Counselor->ClearContextKill();
}

void USCKillerAnimInstance::UpdateHandIK(const FVector& LeftHandLocation, const FVector& RightHandLocation)
{
	LeftHandIKLocation = LeftHandLocation;
	RightHandIKLocation = RightHandLocation;
}

void USCKillerAnimInstance::PlayThrow()
{
	// if we're already playing move to "Throw" section.
	if (Montage_IsPlaying(ThrowMontage))
	{
		if (Montage_GetCurrentSection(ThrowMontage) != TEXT("Throw"))
		{
			Montage_JumpToSection(TEXT("Throw"));
		}
	}
	else
	{
		Montage_Play(ThrowMontage);
	}
}

void USCKillerAnimInstance::EndThrow()
{
	// Force montage to stop.
	if (Montage_IsPlaying(ThrowMontage))
	{
		Montage_Stop(0.25, ThrowMontage);
	}
}

void USCKillerAnimInstance::HandleFootstep(const FName Bone, const bool bJustLanded /*= false*/)
{
	if (Killer)
	{
		if (Killer->IsShiftActive())
			return;

		if (Killer->IsStalkActive())
			return;
	}

	Super::HandleFootstep(Bone);
}

float USCKillerAnimInstance::UpdateGrabState(bool HasCounselor)
{
	static const FName NAME_DefaultSection(TEXT("Default"));
	static const FName NAME_SuccessSection(TEXT("GrabSuccess"));
	static const FName NAME_MissedSection(TEXT("GrabMissed"));
	if (Montage_IsPlaying(GrabMontage))
	{
		if (HasCounselor)
		{
			Montage_SetNextSection(NAME_DefaultSection, NAME_SuccessSection, GrabMontage);

			const float CurrentPosition = Montage_GetPosition(GrabMontage);
			const float TimeLeftForSection = GrabMontage->GetSectionTimeLeftFromPos(CurrentPosition);
			const float NewSectionTime = GrabMontage->GetSectionLength(GrabMontage->GetSectionIndex(NAME_SuccessSection));
			return TimeLeftForSection + NewSectionTime;
		}
		else
		{
			const float CurrentPosition = Montage_GetPosition(GrabMontage);
			const float TimeLeftForSection = GrabMontage->GetSectionTimeLeftFromPos(CurrentPosition);
			const float NewSectionTime = GrabMontage->GetSectionLength(GrabMontage->GetSectionIndex(NAME_MissedSection));
			return TimeLeftForSection + NewSectionTime;
		}
	}
	return 0.f;
}
