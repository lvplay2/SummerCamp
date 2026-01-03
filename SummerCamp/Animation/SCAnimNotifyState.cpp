// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCAnimDriverComponent.h"
#include "SCAnimNotifyState.h"


void USCAnimNotifyState::NotifyBegin(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation, float TotalDuration)
{
	FAnimNotifyData NotifyData(TotalDuration, 0, 0, 0, MeshComp->GetAnimInstance(), MeshComp);

	for (TObjectIterator<USCAnimDriverComponent> It; It; ++It)
	{
		USCAnimDriverComponent* AnimDriver = *It;
		if (IsValid(AnimDriver))
		{
			if (AnimDriver->GetNotifyName() == NotifyName && AnimDriver->BeginAnimDriver(MeshComp->GetOwner(), NotifyData))
			{
				AnimDrivers.AddUnique(AnimDriver);
			}
		}
	}
}

void USCAnimNotifyState::NotifyTick(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation, float FrameDeltaTime)
{
	FAnimNotifyData NotifyData(0, 0, 0, FrameDeltaTime, MeshComp->GetAnimInstance(), MeshComp);

	for (USCAnimDriverComponent* AnimDriver : AnimDrivers)
	{
		if (IsValid(AnimDriver))
		{
			AnimDriver->TickAnimDriver(NotifyData);
		}
	}
}

void USCAnimNotifyState::NotifyEnd(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation)
{
	FAnimNotifyData NotifyData(0, 0, 0, 0, MeshComp->GetAnimInstance(), MeshComp);

	for (USCAnimDriverComponent* AnimDriver : AnimDrivers)
	{
		if (IsValid(AnimDriver))
		{
			AnimDriver->EndAnimDriver(NotifyData);
		}
	}

	AnimDrivers.Empty();
}

FString USCAnimNotifyState::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Driver: %s"), *NotifyName.ToString());
}
