// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "PostProcessNotifyState.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "SCCharacter.h"
#include "SCSpectatorPawn.h"

UPostProcessNotifyState::UPostProcessNotifyState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, TimePassed(0.f)
	, Intensity(0.f)
	, BlendInTime(.25f)
	, BlendOutTime(.25f)
	, NotifyName("Empty")
{

#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(200, 200, 255, 255);
#endif // WITH_EDITORONLY_DATA
}

void UPostProcessNotifyState::BeginDestroy()
{
	PostProcessVolume.Reset();
	Super::BeginDestroy();
}

void UPostProcessNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	PostProcessVolume.Reset();
	AActor* VolumeOwner = nullptr;
	if (ASCCharacter* Character = Cast<ASCCharacter>(MeshComp->GetOwner()))
	{
		if (Character->IsLocallyControlled())
		{
			VolumeOwner = Character;
		}
		else if (PlayForAllClients && GEngine)
		{
			if (APlayerController* PC = GEngine->GetFirstLocalPlayerController(Character->GetWorld()))
			{
				if (!PC->IsLocalController())
					return;

				if (ASCCharacter* LocalCharacter = Cast<ASCCharacter>(PC->GetPawn()))
				{
					VolumeOwner = LocalCharacter;
				}
				else if (ASCSpectatorPawn* LocalSpectator = Cast<ASCSpectatorPawn>(PC->GetPawn()))
				{
					VolumeOwner = LocalSpectator;
				}
			}
		}
	}
	if (!VolumeOwner)
		return;

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Owner = VolumeOwner;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	PostProcessVolume = VolumeOwner->GetWorld()->SpawnActor<APostProcessVolume>(SpawnInfo);

	if (PostProcessVolume.Get())
	{
		PostProcessVolume.Get()->bUnbound = true;
		PostProcessVolume.Get()->Priority = 100.0f;
		PostProcessVolume.Get()->BlendWeight = 0.f;
	}

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
	Intensity = 0;

	if (PostProcessVolume.Get())
	{
		PostProcessVolume.Get()->Settings = NotifyPostProcessSettings;
		PostProcessVolume.Get()->BlendWeight = Intensity;
	}

	Super::NotifyBegin(MeshComp, Animation, TotalDuration);
}

void UPostProcessNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime)
{
	if (ASCCharacter* Character = Cast<ASCCharacter>(MeshComp->GetOwner()))
	{
		if (!Character->IsLocallyControlled() && !PlayForAllClients)
		{
			return;
		}
	}

	if (!IsValid(PostProcessVolume.Get()))
		return;

	TimePassed += FrameDeltaTime;

	if (TimePassed < BlendInTime && BlendInTime > 0.f)
	{
		Intensity = TimePassed / BlendInTime;
	}
	else if (TimePassed > BlendOutStart && BlendOutTime > 0.f)
	{
		Intensity = (TimePassed - BlendOutStart) / BlendOutTime;
	}
	else if (Intensity != 1.f)
	{
		Intensity = 1.f;
	}

	PostProcessVolume.Get()->BlendWeight = Intensity;

	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime);
}

void UPostProcessNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!IsValid(PostProcessVolume.Get()))
		return;

	// KILL KILL KILL!!!
	PostProcessVolume.Get()->SetLifeSpan(0.01f);
	PostProcessVolume.Reset();

	Super::NotifyEnd(MeshComp, Animation);
}

void UPostProcessNotifyState::BranchingPointNotifyBegin(FBranchingPointNotifyPayload& BranchingPointPayload)
{
	PostProcessVolume.Reset();
	AActor* VolumeOwner = nullptr;
	if (ASCCharacter* Character = Cast<ASCCharacter>(BranchingPointPayload.SkelMeshComponent->GetOwner()))
	{
		if (Character->IsLocallyControlled())
		{
			VolumeOwner = Character;
		}
		else if (PlayForAllClients && GEngine)
		{
			if (APlayerController* PC = GEngine->GetFirstLocalPlayerController(Character->GetWorld()))
			{
				if (!PC->IsLocalController())
					return;

				if (ASCCharacter* LocalCharacter = Cast<ASCCharacter>(PC->GetPawn()))
				{
					VolumeOwner = LocalCharacter;
				}
				else if (ASCSpectatorPawn* LocalSpectator = Cast<ASCSpectatorPawn>(PC->GetPawn()))
				{
					VolumeOwner = LocalSpectator;
				}
			}
		}
	}
	if (!VolumeOwner)
		return;

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Owner = VolumeOwner;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	PostProcessVolume = VolumeOwner->GetWorld()->SpawnActor<APostProcessVolume>(SpawnInfo);

	if (PostProcessVolume.Get())
	{
		PostProcessVolume.Get()->bUnbound = true;
		PostProcessVolume.Get()->Priority = 100.0f;
		PostProcessVolume.Get()->BlendWeight = 0.f;
	}

	TimePassed = 0.f;
	if (UAnimMontage* Montage = Cast<UAnimMontage>(BranchingPointPayload.SequenceAsset))
	{
		const float CurrentPosition = BranchingPointPayload.SkelMeshComponent->GetAnimInstance()->Montage_GetPosition(Montage);
		TimePassed = CurrentPosition - BranchingPointPayload.NotifyEvent->GetTriggerTime();
	}

	BlendOutStart = FMath::Max(BranchingPointPayload.NotifyEvent->GetDuration() - BlendOutTime, 0.f);
	Intensity = 0;

	if (PostProcessVolume.Get())
	{
		PostProcessVolume.Get()->Settings = NotifyPostProcessSettings;
		PostProcessVolume.Get()->BlendWeight = Intensity;
	}

	Super::BranchingPointNotifyBegin(BranchingPointPayload);
}

void UPostProcessNotifyState::BranchingPointNotifyTick(FBranchingPointNotifyPayload& BranchingPointPayload, float FrameDeltaTime)
{
	if (ASCCharacter* Character = Cast<ASCCharacter>(BranchingPointPayload.SkelMeshComponent->GetOwner()))
	{
		if (!Character->IsLocallyControlled() && !PlayForAllClients)
		{
			return;
		}
	}

	if (!IsValid(PostProcessVolume.Get()))
		return;

	TimePassed += FrameDeltaTime;

	if (TimePassed < BlendInTime && BlendInTime > 0.f)
	{
		Intensity = TimePassed / BlendInTime;
	}
	else if (TimePassed > BlendOutStart && BlendOutTime > 0.f)
	{
		Intensity = (TimePassed - BlendOutStart) / BlendOutTime;
	}
	else if (Intensity != 1.f)
	{
		Intensity = 1.f;
	}

	PostProcessVolume.Get()->BlendWeight = Intensity;

	Super::BranchingPointNotifyTick(BranchingPointPayload, FrameDeltaTime);
}

void UPostProcessNotifyState::BranchingPointNotifyEnd(FBranchingPointNotifyPayload& BranchingPointPayload)
{
	if (!IsValid(PostProcessVolume.Get()))
		return;

	// KILL KILL KILL!!!
	PostProcessVolume.Get()->SetLifeSpan(0.01f);
	PostProcessVolume.Reset();

	Super::BranchingPointNotifyEnd(BranchingPointPayload);
}

FString UPostProcessNotifyState::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("PP_%s"), *NotifyName);
}
