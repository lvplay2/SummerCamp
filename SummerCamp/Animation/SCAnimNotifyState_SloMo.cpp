// Copyright (C) 2015-2018 IllFonic, LLC.

#include "SummerCamp.h"
#include "SCAnimNotifyState_SloMo.h"

USCAnimNotifyState_SloMo::USCAnimNotifyState_SloMo(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, SloMoSpeed(1.f)
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(180, 180, 180, 255);
#endif
}

USCAnimNotifyState_SloMo::~USCAnimNotifyState_SloMo()
{
	ResetSloMoSpeed();
}

void USCAnimNotifyState_SloMo::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	if (!MeshComp)
		return;

	UWorld* World = MeshComp->GetWorld();
	if (!World)
		return;

	if (AWorldSettings* WorldSettings = World->GetWorldSettings())
	{
		WorldSettings->SetTimeDilation(SloMoSpeed);
		ModifiedWorldSettings.Add(WorldSettings);
	}
}

void USCAnimNotifyState_SloMo::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	ResetSloMoSpeed();
}

FString USCAnimNotifyState_SloMo::GetNotifyName_Implementation() const
{
	if (SloMoSpeed == 1.f)
		return FString::Printf(TEXT("SloMo: No Change"));
	else if (SloMoSpeed > 1.f)
		return FString::Printf(TEXT("What that goes faster: %.2f"), SloMoSpeed);

	return FString::Printf(TEXT("SloMo: %.2f"), SloMoSpeed);
}

void USCAnimNotifyState_SloMo::ResetSloMoSpeed()
{
	for (auto WorldSettings : ModifiedWorldSettings)
	{
		if (!IsValid(WorldSettings))
			continue;

		WorldSettings->SetTimeDilation(1.f);
	}

	ModifiedWorldSettings.Empty();
}
