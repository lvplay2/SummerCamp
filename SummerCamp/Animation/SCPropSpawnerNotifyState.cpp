// Copyright (C) 2015-2018 IllFonic, LLC.

#include "SummerCamp.h"
#include "SCPropSpawnerNotifyState.h"

#include "SCKillerCharacter.h"
#include "SCItem.h"

USCPropSpawnerNotifyState::USCPropSpawnerNotifyState(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, bHideEquippedItem(true)
, SkeletalPropMesh(nullptr)
, SocketName(NAME_None)
, LocationOffset(FVector::ZeroVector)
, RotationOffset(FRotator::ZeroRotator)
, bAutoDestroy(true)
, bItemWasVisible(false)
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(200, 128, 255, 255);
#endif
}

void USCPropSpawnerNotifyState::BeginDestroy()
{
	// In base we didn't get it earlier
	TryCleanupSkeletalMeshes();
	RestoreItemVisiblity();
	OwningCharacter = nullptr;

	Super::BeginDestroy();
}

void USCPropSpawnerNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	// Safety first
	if (MeshComp->GetWorld() == nullptr || MeshComp->GetOwner() == nullptr)
		return;

	// Build our component to attach to the skeletal mesh
	USkeletalMeshComponent* SkeletalMeshComp = nullptr;
	if (SkeletalPropMesh)
	{
		SkeletalMeshComp = NewObject<USkeletalMeshComponent>(MeshComp->GetOwner());
		SkeletalMeshComp->RegisterComponentWithWorld(MeshComp->GetWorld());
		SkeletalMeshComp->SetSkeletalMesh(SkeletalPropMesh);
		SkeletalMeshComp->AttachToComponent(MeshComp, FAttachmentTransformRules::SnapToTargetIncludingScale, SocketName);
		SkeletalMeshComp->RelativeLocation = LocationOffset;
		SkeletalMeshComp->RelativeRotation = RotationOffset;
		SkeletalMeshComp->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::AlwaysTickPose;

		SkeletalMeshComp->Activate();

		SkeletalMeshCompList.Add(SkeletalMeshComp);

		if (PropAnimation != nullptr)
			SkeletalMeshComp->PlayAnimation(PropAnimation, bLoopPropAnimation);
	}

	// Hide our current item
	if (bHideEquippedItem)
	{
		OwningCharacter = Cast<ASCCharacter>(MeshComp->GetOwner());
		if (OwningCharacter)
		{
			// HACK: Temporarily let the player know about this mesh!
			OwningCharacter->TempWeapon = SkeletalMeshComp;
			++OwningCharacter->PropSpawnerHiddingItem;

			// Make sure the temp weapon is bloody if Jason's weapon was already bloody
			ASCKillerCharacter* OwningKiller = Cast<ASCKillerCharacter>(OwningCharacter);
			if (SkeletalMeshComp && OwningKiller && OwningKiller->IsWeaponBloody() && !IsRunningDedicatedServer())
			{
				if (UMaterialInstanceDynamic* WeaponMat = SkeletalMeshComp->CreateAndSetMaterialInstanceDynamic(0))
				{
					static const FName NAME_ShowWeaponBlood(TEXT("ShowWeaponBlood"));
					WeaponMat->SetScalarParameterValue(NAME_ShowWeaponBlood, 1.f);
				}
			}

			if (ASCItem* CurrentItem = OwningCharacter->GetEquippedItem())
			{
				bItemWasVisible = !CurrentItem->bHidden;
				if (bItemWasVisible)
					CurrentItem->SetActorHiddenInGame(true);
			}
		}
	}

	Super::NotifyBegin(MeshComp, Animation, TotalDuration);
}

void USCPropSpawnerNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	// Clean up our prop
	TryCleanupSkeletalMeshes();
	RestoreItemVisiblity();
	OwningCharacter = nullptr;

	Super::NotifyEnd(MeshComp, Animation);
}

FString USCPropSpawnerNotifyState::GetNotifyName_Implementation() const
{
	// We have non-default data, display that info
	FString Output = FString::Printf(TEXT("Prop: %s"), SkeletalPropMesh ? *SkeletalPropMesh->GetName() : TEXT("None"));
	
	if (bHideEquippedItem)
	{
		Output += TEXT(" (hide)");
	}

	if (!bAutoDestroy)
	{
		Output += TEXT(" (persistent)");
	}

	return Output;
}

void USCPropSpawnerNotifyState::RestoreItemVisiblity()
{
	// Show our current item again (maybe)
	if (bHideEquippedItem)
	{
		if (IsValid(OwningCharacter))
		{
			// HACK: Temporarly let the player know about this mesh!
			OwningCharacter->TempWeapon = nullptr;
			--OwningCharacter->PropSpawnerHiddingItem;

			if (bItemWasVisible)
			{
				if (ASCItem* CurrentItem = OwningCharacter->GetEquippedItem())
				{
					CurrentItem->SetActorHiddenInGame(false);
				}
			}
		}
	}
}

void USCPropSpawnerNotifyState::TryCleanupSkeletalMeshes()
{
	if (bAutoDestroy)
	{
		for (TWeakObjectPtr<USkeletalMeshComponent> SkeletalMeshComp : SkeletalMeshCompList)
		{
			if (IsValid(SkeletalMeshComp.Get()))
			{
				SkeletalMeshComp.Get()->SetHiddenInGame(true);
				SkeletalMeshComp.Get()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
				SkeletalMeshComp.Get()->DestroyComponent();
				SkeletalMeshComp.Reset();
			}
		}
	}
	else
	{

	}

	// Empty the list either way, we don't want to keep our references
	SkeletalMeshCompList.Empty();
}
