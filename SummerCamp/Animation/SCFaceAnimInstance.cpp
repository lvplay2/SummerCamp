// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCFaceAnimInstance.h"

#include "SCAnimInstance.h"

#include "SCCounselorCharacter.h"

TAutoConsoleVariable<int32> CVarDebugFace(TEXT("sc.DebugFacialAnimation"), 0,
	TEXT("Displays debug information for faces.\n")
	TEXT(" 0: None\n")
	TEXT(" 1: Local Only\n")
	TEXT(" 2: Every Face\n")
);

USCFaceAnimInstance::USCFaceAnimInstance(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
// Morph Targets
, JawAlphaName(TEXT("Face_Jaw"))
, BrowFearName(TEXT("Brow_Fear"))
, BrowShockName(TEXT("Brow_Shock"))
, BrowAngerName(TEXT("Brow_Anger"))
, BrowLAlphaName(TEXT("L_Brow_Alpha"))
, BrowRAlphaName(TEXT("R_Brow_Alpha"))
, BlinkOverrideCurveName(TEXT("Blink"))
, BlinkLeftName(TEXT("Blink_L"))
, BlinkRightName(TEXT("Blink_R"))

// Fear
, FearFaceMin(0.75f)
, ShockFaceMin(0.35f)
, FearToNeutralInterpSpeed(5.f)
, FearMorphTargetName(TEXT("Fear"))
, ShockMorphTargetName(TEXT("Shock"))

// Eyes
, LookDistance(2500.f)
, MaxEyeLookYawOffset(30.f)
, MaxEyeLookPitchOffset(25.f)
, LeftEyeBoneName(TEXT("L_Eye"))
, RightEyeBoneName(TEXT("R_Eye"))
, EyeMovementOverride(1.f)
, EyeMovementSpeed(1.f)
, MinLookUpdateFrequency(5.f)
, MaxLookUpdateFrequency(10.f)
, MinEyelidWeight(0.f)
, MaxEyelidWeight(1.f)
, EyelidMovementSpeed(1.f)
, MinBlinkUpdateFrequency(5.f)
, MaxBlinkUpdateFrequency(10.f)
, bAnimationOverride(false)
, bShouldLookAround(false)
, bShouldBlink(false)
, CurrentEyelidWeight(0.f)
, DesiredEyelidWeight(1.f)
, MaxLookTime(1.5f)
, LookTime(0.f)
, LookUpdateTime(0.f)
, BlinkUpdateTime(0.f)
{
	FacesOfFearNames.Add(TEXT("Fear"));
	FacesOfFearNames.Add(TEXT("Shock"));

	MontageFaceNames.Add({TEXT("Concept"), TEXT("Concept"), 0.f});
	MontageFaceNames.Add({TEXT("Face_Fear"), TEXT("Fear"), 0.f});
	MontageFaceNames.Add({TEXT("Face_Shock"), TEXT("Shock"), 0.f});
	MontageFaceNames.Add({TEXT("Face_Anger"), TEXT("Anger"), 0.f});
	MontageFaceNames.Add({TEXT("Face_Disgust"), TEXT("Disgust"), 0.f});
	MontageFaceNames.Add({TEXT("Face_Happy"), TEXT("Happy"), 0.f});
}

void USCFaceAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	TryGetParentAnimInstance();

	Counselor = Cast<ASCCounselorCharacter>(TryGetPawnOwner());

	// Initialize our blinking to something other than zero so we don't start being alive by blinking
	// Go into the world wide-eyed! Explore! Feel! LIVE!
	BlinkUpdateTime = FMath::FRandRange(MinBlinkUpdateFrequency, MaxBlinkUpdateFrequency);
}

void USCFaceAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (BaseAnimInstance == nullptr)
	{
		TryGetParentAnimInstance();
		if (BaseAnimInstance == nullptr)
			return;
	}

	const bool bWasDead = bIsDead;
	if (Counselor)
	{
		bIsDead = !Counselor->IsAlive();
		if (bIsDead)
		{
			if (!bWasDead)
			{
				SetMorphTarget(BlinkLeftName, 1.f);
				SetMorphTarget(BlinkRightName, 1.f);
			}
			return;
		}

		FearAlpha = Counselor->GetFear() * 0.01f;
		EyeMovementOverride = !Counselor->IsInContextKill();
	}

	// Update all curves
	BodyCurveValues.Empty(CurveNames.Num());
	for (const FName& Name : CurveNames)
	{
		BodyCurveValues.Add(Name, BaseAnimInstance->GetCurveValue(Name));
	}

	const bool bWasBodyPlayingMontage = bIsBodyPlayingMontage;
	bIsBodyPlayingMontage = BaseAnimInstance->Montage_IsPlaying(nullptr);

	// Montage playing, turn our fear faces off
	if (bWasBodyPlayingMontage != bIsBodyPlayingMontage && bIsBodyPlayingMontage)
	{
		for (const FName Name : FacesOfFearNames)
		{
			SetMorphTarget(Name, 0.f);
		}
	}

	// Update morph target values
	if (bIsBodyPlayingMontage)
	{
		for (auto& Morph : MontageFaceNames)
		{
			const float Value = GetBodyCurveValue(Morph.CurveName);
			if (FMath::Abs(Value - Morph.LastSetValue) > KINDA_SMALL_NUMBER)
			{
				SetMorphTarget(Morph.MorphTargetName, Value);
				Morph.LastSetValue = Value;
			}
		}

		JawAlpha = GetBodyCurveValue(JawAlphaName);
		BrowFear = GetBodyCurveValue(BrowFearName);
		BrowShock = GetBodyCurveValue(BrowShockName);
		BrowAnger = GetBodyCurveValue(BrowAngerName);
		BrowLAlpha = GetBodyCurveValue(BrowLAlphaName);
		BrowRAlpha = GetBodyCurveValue(BrowRAlphaName);
	}
	else if (EmotionCurve)
	{
		// Shock and Fear!
		float MinTime, MaxTime;
		EmotionCurve->GetTimeRange(MinTime, MaxTime);
		const float InTime = FMath::Fmod(GetWorld()->TimeSeconds, MaxTime);
		const FVector EmotionalValue = EmotionCurve->GetVectorValue(InTime);

		// Shock!
		if (FearAlpha > ShockFaceMin)
		{
			TargetShock = FMath::Min(EmotionalValue.X, FearAlpha);
		}
		else
		{
			TargetShock = FMath::FInterpTo(TargetShock, 0.f, DeltaSeconds, FearToNeutralInterpSpeed);
		}

		// And fear!
		if (FearAlpha > FearFaceMin)
		{
			TargetFear = FMath::Min(EmotionalValue.Y, FearAlpha);
		}
		else
		{
			TargetFear = FMath::FInterpTo(TargetFear, 0.f, DeltaSeconds, FearToNeutralInterpSpeed);
		}

		SetMorphTarget(FearMorphTargetName, TargetFear);
		SetMorphTarget(ShockMorphTargetName, TargetShock);
	}

	if (bAnimationOverride)
	{
		const float BlinkValue = GetBodyCurveValue(BlinkOverrideCurveName);
		SetMorphTarget(BlinkLeftName, BlinkValue);
		SetMorphTarget(BlinkRightName, BlinkValue);
	}

	// Update the eyes
	UpdateEyes(DeltaSeconds);

	if (CVarDebugFace.GetValueOnGameThread() >= 1)
	{
		const APawn* Pawn = Cast<APawn>(GetOwningActor());
		if (CVarDebugFace.GetValueOnGameThread() >= 2 || (Pawn && Pawn->IsLocallyControlled()))
		{
			float Total = 0.f;
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Yellow, FString::Printf(TEXT("%s's face is using these curves: "), *Pawn->GetName()), false);
			for (const auto& Pair : BodyCurveValues)
			{
				Total += Pair.Value;
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, Pair.Value > 0.f ? FColor::Cyan : FColor(184, 184, 184), FString::Printf(TEXT("%s @ %.4f"), *Pair.Key.ToString(), Pair.Value), false);
			}
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, Total > 1.1f ? FColor::Red : FColor::Cyan, FString::Printf(TEXT("Total: %.2f\n"), Total) , false);
		}

		const USkeletalMeshComponent* FaceMesh = GetOwningComponent();
		const FVector LeftEyeLocation = FaceMesh->GetBoneLocation(LeftEyeBoneName);
		const FVector RightEyeLocation = FaceMesh->GetBoneLocation(RightEyeBoneName);
		const FVector MidEyeLocation = (LeftEyeLocation + RightEyeLocation) * 0.5f;
		const FRotator HeadRotation = FRotator(FaceMesh->GetBoneQuaternion(FaceMesh->GetParentBone(LeftEyeBoneName)));
		const FVector WorldLookPosition = MidEyeLocation + HeadRotation.RotateVector(RelativeLookPosition);

		DrawDebugSphere(GetWorld(), WorldLookPosition, 16.f, 8, FColor::Red, false, 0.f);
		DrawDebugDirectionalArrow(GetWorld(), MidEyeLocation, MidEyeLocation + HeadRotation.Vector() * 15.f, 1.f, FColor::Red);

		DrawDebugDirectionalArrow(GetWorld(), LeftEyeLocation, LeftEyeLocation + FaceMesh->GetBoneQuaternion(LeftEyeBoneName).Vector() * 10.f, 1.f, FColor::Green);
		DrawDebugDirectionalArrow(GetWorld(), RightEyeLocation, RightEyeLocation + FaceMesh->GetBoneQuaternion(RightEyeBoneName).Vector() * 10.f, 1.f, FColor::Green);
	}
}

float USCFaceAnimInstance::GetBodyCurveValue(const FName CurveName) const
{
	const float* Value = BodyCurveValues.Find(CurveName);
	if (Value)
		return *Value;

	return 0.f;
}

void USCFaceAnimInstance::CloseEyes()
{
	SetMorphTarget(EyelidMorphTarget, 0.f);
	SetMorphTarget(EyelashMorphTarget, 0.f);
}

void USCFaceAnimInstance::OpenEyes()
{
	SetMorphTarget(EyelidMorphTarget, 1.f);
	SetMorphTarget(EyelashMorphTarget, 1.f);
}

void USCFaceAnimInstance::TryGetParentAnimInstance()
{
	USkeletalMeshComponent* SkeletalMeshComp = GetOwningComponent();
	USkeletalMeshComponent* ParentSkeleton = Cast<USkeletalMeshComponent>(SkeletalMeshComp->GetAttachParent());

	if (ParentSkeleton)
	{
		BaseAnimInstance = ParentSkeleton->GetAnimInstance();
	}
}

void USCFaceAnimInstance::UpdateEyes(float DeltaSeconds)
{
	ASCCounselorCharacter* OwningCounselor = Cast<ASCCounselorCharacter>(GetOwningActor());

	if (!OwningCounselor || !OwningCounselor->IsAlive() || OwningCounselor->bIsDying)
		return;

	const USkeletalMeshComponent* CounselorMesh = OwningCounselor->GetMesh();
	const USkeletalMeshComponent* FaceMesh = GetOwningComponent();
	if (!CounselorMesh || !FaceMesh)
		return;

	if (bAnimationOverride)
	{
		// Look at the target joint
		const static FName NAME_TargetJoint(TEXT("target_jnt"));
		const FVector TargetJointLocation = CounselorMesh->GetBoneLocation(NAME_TargetJoint);
		const FVector LeftEyeLocation = FaceMesh->GetBoneLocation(LeftEyeBoneName);
		const FVector RightEyeLocation = FaceMesh->GetBoneLocation(RightEyeBoneName);
		DesiredLeftEyeRotation = (TargetJointLocation - LeftEyeLocation).Rotation();
		DesiredRightEyeRotation = (TargetJointLocation - RightEyeLocation).Rotation();
			
		// Eyelids are handled by animation
	}
	else
	{
		BlinkUpdateTime -= DeltaSeconds;
		if (BlinkUpdateTime <= 0.f)
		{
			bShouldBlink = true;
			BlinkUpdateTime = FMath::FRandRange(MinBlinkUpdateFrequency, MaxBlinkUpdateFrequency);
		}

		// Pick a new desired rotation
		if (DesiredLeftEyeRotation.Equals(LeftEyeRotation, 0.01f))
		{
			if (bIsLookingForward)
			{
				LookUpdateTime -= DeltaSeconds;
				if (LookUpdateTime <= 0.f)
				{
					bShouldLookAround = true;
					LookUpdateTime = FMath::FRandRange(MinLookUpdateFrequency, MaxLookUpdateFrequency);
					LookTime = FMath::FRandRange(0.f, MaxLookTime);
				}
			}

			bool bUpdateLook = true;
			if (bShouldLookAround && bIsLookingForward)
			{
				// Pick a random rotation for the eyes
				const float Yaw = FMath::FRandRange(-MaxEyeLookYawOffset, MaxEyeLookYawOffset);
				const float Pitch = FMath::FRandRange(-MaxEyeLookPitchOffset, MaxEyeLookPitchOffset);

				const FVector LookDir = FRotator(Pitch, Yaw, 0.f).Vector().GetSafeNormal();
				RelativeLookPosition = LookDir * LookDistance;
				bIsLookingForward = false;
				bShouldLookAround = false;
			}
			else if (!bIsLookingForward)
			{
				if (LookTime > 0.f)
				{
					// Keep looking at the target
					LookTime -= DeltaSeconds;
					bUpdateLook = false;
				}
				else
				{
					// Go back to looking forward
					RelativeLookPosition = FVector::ForwardVector * LookDistance;
					bIsLookingForward = true;
				}
			}

			if (bUpdateLook)
			{
				const FVector LeftEyeLocation = FaceMesh->GetBoneLocation(LeftEyeBoneName);
				const FVector RightEyeLocation = FaceMesh->GetBoneLocation(RightEyeBoneName);
				const FVector MidEyeLocation = (LeftEyeLocation + RightEyeLocation) * 0.5f;
				const FRotator HeadRotation = FRotator(FaceMesh->GetBoneQuaternion(FaceMesh->GetParentBone(LeftEyeBoneName)));

				const FVector WorldLookPosition = MidEyeLocation + HeadRotation.RotateVector(RelativeLookPosition);
				DesiredLeftEyeRotation = HeadRotation - (WorldLookPosition - LeftEyeLocation).Rotation();
				DesiredRightEyeRotation = HeadRotation - (WorldLookPosition - RightEyeLocation).Rotation();

				DesiredLeftEyeRotation = FRotator(0.f, DesiredLeftEyeRotation.Pitch, DesiredLeftEyeRotation.Yaw);
				DesiredRightEyeRotation = FRotator(0.f, DesiredRightEyeRotation.Pitch, DesiredRightEyeRotation.Yaw);
			}
		}

		// Update Eyelids
		// Pick a new desired eyelid weight
		if (FMath::IsNearlyEqual(DesiredEyelidWeight, CurrentEyelidWeight, 0.1f))
		{
			if (FMath::IsNearlyEqual(DesiredEyelidWeight, MinEyelidWeight, 0.1f) && bShouldBlink)
			{
				DesiredEyelidWeight = MaxEyelidWeight;
				bShouldBlink = false;
			}
			else
			{
				DesiredEyelidWeight = MinEyelidWeight;
			}
		}
		const float DeltaEyelidWeight = FMath::Clamp(FMath::Lerp(CurrentEyelidWeight, DesiredEyelidWeight, EyelidMovementSpeed * DeltaSeconds), MinEyelidWeight, MaxEyelidWeight);

		// Set eyelid and eyelash morph target weights
		SetMorphTarget(EyelidMorphTarget, DeltaEyelidWeight);
		SetMorphTarget(EyelashMorphTarget, DeltaEyelidWeight);

		CurrentEyelidWeight = DeltaEyelidWeight;
	}
	
	// Set eye rotation
	LeftEyeRotation = FMath::RInterpTo(LeftEyeRotation, DesiredLeftEyeRotation, DeltaSeconds, EyeMovementSpeed);
	RightEyeRotation = FMath::RInterpTo(RightEyeRotation, DesiredRightEyeRotation, DeltaSeconds, EyeMovementSpeed);
}
