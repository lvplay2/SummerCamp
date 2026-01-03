// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPostProcessBlendableNotify.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "SCCharacter.h"
#include "SCSpectatorPawn.h"

USCPostProcessBlendableNotify::USCPostProcessBlendableNotify(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, TimePassed(0.f)
	, Intensity(0.f)
	, MinIntensity(0.f)
	, MaxIntensity(1.f)
	, BlendInTime(.25f)
	, BlendOutTime(.25f)

{

#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(200, 200, 255, 255);
#endif // WITH_EDITORONLY_DATA
}

void USCPostProcessBlendableNotify::BeginDestroy()
{
	DynamicInstance.Reset();
	PostProcessVolume.Reset();
	Super::BeginDestroy();
}

void USCPostProcessBlendableNotify::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	if (DynamicInstance != nullptr)
		return;

	if (!IsValid(MeshComp))
	{
		UE_LOG(LogEngine, Warning, TEXT("Missing SkeletalMeshComponent"));
		return;
	}

	if (!IsValid(Animation))
	{
		UE_LOG(LogEngine, Warning, TEXT("Missing UAnimSequenceBase"));
		return;
	}

	PostProcessVolume.Reset();
	if (ASCCharacter* Character = Cast<ASCCharacter>(MeshComp->GetOwner()))
	{
		if (Character->IsLocallyControlled())
		{
			PostProcessVolume = Character->GetPostProcessVolume();
			DynamicInstance = UMaterialInstanceDynamic::Create(MaterialInstance, PostProcessVolume.Get());
		}
		else if (PlayForAllClients && GEngine)
		{
			if (APlayerController* PC = GEngine->GetFirstLocalPlayerController(Character->GetWorld()))
			{
				if (!PC->IsLocalController())
					return;

				if (ASCCharacter* LocalCharacter = Cast<ASCCharacter>(PC->GetPawn()))
				{
					PostProcessVolume = LocalCharacter->GetPostProcessVolume();
					DynamicInstance = UMaterialInstanceDynamic::Create(MaterialInstance, PostProcessVolume.Get());
				}
				else if (ASCSpectatorPawn* LocalSpectator = Cast<ASCSpectatorPawn>(PC->GetPawn()))
				{
					PostProcessVolume = LocalSpectator->GetPostProcessVolume();
					DynamicInstance = UMaterialInstanceDynamic::Create(MaterialInstance, PostProcessVolume.Get());
				}
			}
		}
	}

	if (!PostProcessVolume.IsValid() || !DynamicInstance.IsValid())
		return;

	TimePassed = 0.f;
	if (UAnimMontage* Montage = Cast<UAnimMontage>(Animation))
	{
		const float CurrentPosition = MeshComp->GetAnimInstance()->Montage_GetPosition(Montage);
		const float DeltaPosition = CurrentPosition - 0.1f;
		TArray<const FAnimNotifyEvent*> Notifies;
		Montage->GetAnimNotifiesFromDeltaPositions(DeltaPosition, CurrentPosition, Notifies);
		if (Notifies.Num() > 0)
		{
			TimePassed = CurrentPosition - Notifies[Notifies.Num() - 1]->GetTriggerTime();
		}
	}

	BlendOutStart = FMath::Max(TotalDuration - BlendOutTime, 0.f);
	Intensity = MinIntensity;

	for (FBlendableTextureOverride Override : OverrideTextures)
	{
		if (Override.Texture == nullptr)
			continue;

		DynamicInstance.Get()->SetTextureParameterValue(Override.ParameterName, Override.Texture);
	}

	for (FBlendableIntesityHooks Hook : IntensityHooks)
	{
		if (Hook.UseBlendIn)
			DynamicInstance.Get()->SetScalarParameterValue(Hook.ParameterName, Intensity);
	}
	PostProcessVolume.Get()->Settings.AddBlendable(DynamicInstance.Get(), 1.f);

	Super::NotifyBegin(MeshComp, Animation, TotalDuration);
}

void USCPostProcessBlendableNotify::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime)
{
	if (ASCCharacter* Character = Cast<ASCCharacter>(MeshComp->GetOwner()))
	{
		if (!Character->IsLocallyControlled() && !PlayForAllClients)
		{
			return;
		}
	}

	if (!PostProcessVolume.IsValid() || !DynamicInstance.IsValid())
		return;

	TimePassed += FrameDeltaTime;

	if (TimePassed < BlendInTime && BlendInTime > 0.f)
	{
		Intensity = FMath::Lerp(MinIntensity, MaxIntensity, TimePassed / BlendInTime);

		for (FBlendableIntesityHooks Hook : IntensityHooks)
		{
			if (Hook.UseBlendIn)
				DynamicInstance.Get()->SetScalarParameterValue(Hook.ParameterName, Intensity);
		}
	}
	else if (TimePassed > BlendOutStart && BlendOutTime > 0.f)
	{
		Intensity = FMath::Lerp(MaxIntensity, MinIntensity, (TimePassed - BlendOutStart) / BlendOutTime);

		for (FBlendableIntesityHooks Hook : IntensityHooks)
		{
			if (Hook.UseBlendOut)
				DynamicInstance.Get()->SetScalarParameterValue(Hook.ParameterName, Intensity);
		}
	}
	else if (Intensity != MaxIntensity)
	{
		Intensity = MaxIntensity;

		for (FBlendableIntesityHooks Hook : IntensityHooks)
		{
			if (Hook.UseBlendOut)
				DynamicInstance.Get()->SetScalarParameterValue(Hook.ParameterName, Intensity);
		}
	}

	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime);
}

void USCPostProcessBlendableNotify::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (PostProcessVolume.IsValid() && DynamicInstance.IsValid())
	{
		PostProcessVolume.Get()->Settings.RemoveBlendable(DynamicInstance.Get());
		PostProcessVolume.Get()->MarkComponentsRenderStateDirty();
	}

	PostProcessVolume.Reset();
	DynamicInstance.Reset();

	Super::NotifyEnd(MeshComp, Animation);
}

void USCPostProcessBlendableNotify::BranchingPointNotifyBegin(FBranchingPointNotifyPayload& BranchingPointPayload)
{
	if (DynamicInstance != nullptr)
		return;

	PostProcessVolume.Reset();
	if (ASCCharacter* Character = Cast<ASCCharacter>(BranchingPointPayload.SkelMeshComponent->GetOwner()))
	{
		if (Character->IsLocallyControlled())
		{
			PostProcessVolume = Character->GetPostProcessVolume();
			DynamicInstance = UMaterialInstanceDynamic::Create(MaterialInstance, PostProcessVolume.Get());
		}
		else if (PlayForAllClients && GEngine)
		{
			if (APlayerController* PC = GEngine->GetFirstLocalPlayerController(Character->GetWorld()))
			{
				if (!PC->IsLocalController())
					return;

				if (ASCCharacter* LocalCharacter = Cast<ASCCharacter>(PC->GetPawn()))
				{
					PostProcessVolume = LocalCharacter->GetPostProcessVolume();
					DynamicInstance = UMaterialInstanceDynamic::Create(MaterialInstance, PostProcessVolume.Get());
				}
				else if (ASCSpectatorPawn* LocalSpectator = Cast<ASCSpectatorPawn>(PC->GetPawn()))
				{
					PostProcessVolume = LocalCharacter->GetPostProcessVolume();
					DynamicInstance = UMaterialInstanceDynamic::Create(MaterialInstance, PostProcessVolume.Get());
				}
			}
		}
	}

	if (!PostProcessVolume.IsValid() || !DynamicInstance.IsValid())
		return;

	TimePassed = 0.f;
	if (UAnimMontage* Montage = Cast<UAnimMontage>(BranchingPointPayload.SequenceAsset))
	{
		const float CurrentPosition = BranchingPointPayload.SkelMeshComponent->GetAnimInstance()->Montage_GetPosition(Montage);
		TimePassed = CurrentPosition - BranchingPointPayload.NotifyEvent->GetTriggerTime();
	}

	BlendOutStart = FMath::Max(BranchingPointPayload.NotifyEvent->GetDuration() - BlendOutTime, 0.f);
	Intensity = MinIntensity;

	for (FBlendableTextureOverride Override : OverrideTextures)
	{
		DynamicInstance.Get()->SetTextureParameterValue(Override.ParameterName, Override.Texture);
	}

	for (FBlendableIntesityHooks Hook : IntensityHooks)
	{
		if (Hook.UseBlendIn)
			DynamicInstance.Get()->SetScalarParameterValue(Hook.ParameterName, Intensity);
	}
	PostProcessVolume.Get()->Settings.AddBlendable(DynamicInstance.Get(), 1.f);

	Super::BranchingPointNotifyBegin(BranchingPointPayload);
}

void USCPostProcessBlendableNotify::BranchingPointNotifyTick(FBranchingPointNotifyPayload& BranchingPointPayload, float FrameDeltaTime)
{
	if (ASCCharacter* Character = Cast<ASCCharacter>(BranchingPointPayload.SkelMeshComponent->GetOwner()))
	{
		if (!Character->IsLocallyControlled() && !PlayForAllClients)
		{
			return;
		}
	}

	if (!PostProcessVolume.IsValid() || !DynamicInstance.IsValid())
		return;

	TimePassed += FrameDeltaTime;

	if (TimePassed < BlendInTime && BlendInTime > 0.f)
	{
		Intensity = FMath::Lerp(MinIntensity, MaxIntensity, TimePassed / BlendInTime);

		for (FBlendableIntesityHooks Hook : IntensityHooks)
		{
			if (Hook.UseBlendIn)
				DynamicInstance.Get()->SetScalarParameterValue(Hook.ParameterName, Intensity);
		}
	}
	else if (TimePassed > BlendOutStart && BlendOutTime > 0.f)
	{
		Intensity = FMath::Lerp(MaxIntensity, MinIntensity, (TimePassed - BlendOutStart) / BlendOutTime);

		for (FBlendableIntesityHooks Hook : IntensityHooks)
		{
			if (Hook.UseBlendOut)
				DynamicInstance.Get()->SetScalarParameterValue(Hook.ParameterName, Intensity);
		}
	}
	else if (Intensity != MaxIntensity)
	{
		Intensity = MaxIntensity;

		for (FBlendableIntesityHooks Hook : IntensityHooks)
		{
			if (Hook.UseBlendOut)
				DynamicInstance.Get()->SetScalarParameterValue(Hook.ParameterName, Intensity);
		}
	}

	Super::BranchingPointNotifyTick(BranchingPointPayload, FrameDeltaTime);
}

void USCPostProcessBlendableNotify::BranchingPointNotifyEnd(FBranchingPointNotifyPayload& BranchingPointPayload)
{
	if (PostProcessVolume.IsValid() && DynamicInstance.IsValid())
	{
		PostProcessVolume.Get()->Settings.RemoveBlendable(DynamicInstance.Get());
		PostProcessVolume.Get()->MarkComponentsRenderStateDirty();
	}

	PostProcessVolume.Reset();
	DynamicInstance.Reset();

	Super::BranchingPointNotifyEnd(BranchingPointPayload);
}

FString USCPostProcessBlendableNotify::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Blendable: %s"), MaterialInstance == nullptr ? TEXT("Empty") : *MaterialInstance->GetName());
}
