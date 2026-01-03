// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCHairAnimInstance.h"

USCHairAnimInstance::USCHairAnimInstance(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, TargetFramerate(60.f)
, MaxAlpha(0.8f)
{
}

void USCHairAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	InvFramerate = 1.f / FMath::Clamp(TargetFramerate, 1.f, 120.f);
}

void USCHairAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	ScaledAlpha = FMath::Min(InvFramerate / FMath::Max(DeltaSeconds, KINDA_SMALL_NUMBER), 1.f) * MaxAlpha;
}
