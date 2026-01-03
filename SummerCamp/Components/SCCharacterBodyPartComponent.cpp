// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCCharacterBodyPartComponent.h"

#include "SCCounselorCharacter.h"
#include "SCGameInstance.h"
#include "SCWeapon.h"

USCCharacterBodyPartComponent::USCCharacterBodyPartComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	static FName NAME_LimbBloodEffect(TEXT("LimbBloodEffect"));
	LimbBloodEffect = CreateDefaultSubobject<UParticleSystem>(NAME_LimbBloodEffect);

	static FName NAME_LimbGoreCap(TEXT("LimbGoreCap"));
	LimbGoreCap = CreateDefaultSubobject<UStaticMesh>(NAME_LimbGoreCap);

	static FName NAME_ParentLimbGoreCap(TEXT("ParentLimbGoreCap"));
	ParentLimbGoreCap = CreateDefaultSubobject<USkeletalMesh>(NAME_ParentLimbGoreCap);

	static FName NAME_ParentLimbBloodEffect(TEXT("ParentLimbBloodEffect"));
	ParentLimbBloodEffect = CreateDefaultSubobject<UParticleSystem>(NAME_ParentLimbBloodEffect);

	static FName NAME_NoCollision(TEXT("NoCollision"));
	SetCollisionProfileName(NAME_NoCollision);

	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void USCCharacterBodyPartComponent::InitializeComponent()
{
	Super::InitializeComponent();

	ClothingMesh = NewObject<USkeletalMeshComponent>(this);
	ClothingMesh->RegisterComponent();
	ClothingMesh->InitializeComponent();
	ClothingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ClothingMesh->SetMasterPoseComponent(this);
	ClothingMesh->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetIncludingScale);
	ClothingMesh->SetVisibility(false);
}

void USCCharacterBodyPartComponent::ShowLimb(const FName SwapName /*= NAME_None*/, TAssetPtr<USkeletalMesh> MeshToSwap /*= nullptr*/)
{
	PendingMeshSwap = MeshToSwap.Get() ? MeshToSwap : [this, &SwapName]() -> TAssetPtr<USkeletalMesh>
	{
		for (const FSCLimbMeshSwapInfo& SwapInfo : MeshSwaps)
		{
			if (SwapInfo.SwapName == SwapName)
				return SwapInfo.SwapMesh;
		}

		if (!SwapName.IsNone())
			UE_LOG(LogTemp, Warning, TEXT("ShowLimb : Couldn't find mesh swap info in %s for swap: %s. Using default mesh"), *GetName(), *SwapName.ToString());

		return SkeletalMesh;
	}();

	if (PendingMeshSwap.Get())
	{
		// Already loaded
		DeferredShowLimb();
	}
	else if (!PendingMeshSwap.IsNull())
	{
		// Load and show when it's ready
		USCGameInstance* GameInstance = Cast<USCGameInstance>(GetWorld()->GetGameInstance());
		GameInstance->StreamableManager.RequestAsyncLoad(PendingMeshSwap.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &ThisClass::DeferredShowLimb));
	}
}

void USCCharacterBodyPartComponent::DetachLimb(const FVector& Impulse /*= FVector::ZeroVector*/, TAssetPtr<UTexture> BloodMaskOverride /*= nullptr*/, const bool bShowLimb /*= true*/)
{
	if (USkeletalMeshComponent* ParentSkelMesh = Cast<USkeletalMeshComponent>(GetAttachParent()))
	{
		const FName AttachSocket = GetAttachSocketName();
		// Hide bone on parent mesh where we are detaching from
		ParentSkelMesh->HideBoneByName(BoneToHide.IsNone() ? AttachSocket : BoneToHide, EPhysBodyOp::PBO_None);

		// Turn collision on
		SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		static FName NAME_Ragdoll(TEXT("Ragdoll"));
		SetCollisionProfileName(NAME_Ragdoll);
		SetCollisionResponseToChannel(ECC_AnimCameraBlocker, ECollisionResponse::ECR_Ignore);

		// Detach the limb
		if (bShowLimb)
		{
			SetComponentTickEnabled(true);
			SetVisibility(true, true);
			DetachFromComponent(FDetachmentTransformRules(EDetachmentRule::KeepWorld, true));
			SetWorldLocation(ParentSkelMesh->GetSocketTransform(AttachSocket).GetLocation());
			SetMasterPoseComponent(nullptr);
			SetAllBodiesCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
			SetAllBodiesSimulatePhysics(true);
			AddImpulse(Impulse);
		}
		
		USCGameInstance* GameInstance = Cast<USCGameInstance>(GetWorld()->GetGameInstance());

		// Apply gore caps
		if (bShowLimb)
		{
			if (LimbGoreCap.Get())
			{
				DeferredShowLimbGoreCap();
			}
			else if (!LimbGoreCap.IsNull())
			{
				GameInstance->StreamableManager.RequestAsyncLoad(LimbGoreCap.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &ThisClass::DeferredShowLimbGoreCap));
			}
		}
		ParentSkeletalMesh = ParentSkelMesh;
		if (ParentLimbGoreCap.Get())
		{
			DeferredShowParentLimbGoreCap();
		}
		else if (!ParentLimbGoreCap.IsNull())
		{
			GameInstance->StreamableManager.RequestAsyncLoad(ParentLimbGoreCap.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &ThisClass::DeferredShowParentLimbGoreCap));
		}

		// Spawn blood effects
		if (bShowLimb)
		{
			if (LimbBloodEffect.Get())
			{
				DeferredPlayLimbBloodEffect();
			}
			else if (!LimbBloodEffect.IsNull())
			{
				GameInstance->StreamableManager.RequestAsyncLoad(LimbBloodEffect.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &ThisClass::DeferredPlayLimbBloodEffect));
			}
		}
		if (ParentLimbBloodEffect.Get())
		{
			DeferredPlayParentLimbBloodEffect();
		}
		else if (!ParentLimbBloodEffect.IsNull())
		{
			GameInstance->StreamableManager.RequestAsyncLoad(ParentLimbBloodEffect.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &ThisClass::DeferredPlayParentLimbBloodEffect));
		}
	}
}

void USCCharacterBodyPartComponent::ShowBlood(TAssetPtr<UTexture> BloodMaskOverride/* = nullptr*/, FName TextureParameterOverride/* = NAME_None*/)
{
	if (IsRunningDedicatedServer())
		return;

	PendingBloodMask = BloodMaskOverride ? BloodMaskOverride : LimbUVTexture;
	if (PendingBloodMask.IsNull())
		return;

	PendingBloodParameter = !TextureParameterOverride.IsNone() ? TextureParameterOverride : TextureParameterName;
	if (PendingBloodMask.Get())
	{
		// Already loaded
		DeferredApplyBloodMask();
	}
	else if (!PendingBloodMask.IsNull())
	{
		// Load and play when it's ready
		USCGameInstance* GameInstance = Cast<USCGameInstance>(GetWorld()->GetGameInstance());
		GameInstance->StreamableManager.RequestAsyncLoad(PendingBloodMask.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &ThisClass::DeferredApplyBloodMask));
	}
}

void USCCharacterBodyPartComponent::RemoveBlood()
{
	if (IsRunningDedicatedServer())
		return;

	for (const auto& LimbIndex : LimbMaterialElementIndices)
	{
		if (UMaterialInstanceDynamic* LimbMat = Cast<UMaterialInstanceDynamic>(GetMaterial(LimbIndex)))
		{
			LimbMat->ClearParameterValues();
		}
	}
}

void USCCharacterBodyPartComponent::ApplyOutfit(const FSCCounselorOutfit* Outfit)
{
	const USCClothingData* OutfitData = Outfit ? Outfit->ClothingClass.GetDefaultObject() : nullptr;

	// Set our mesh
	USkeletalMesh* Mesh = OutfitData ? OutfitData->DismembermentMeshes[(uint8)LimbType].Get() : nullptr;
	if (Mesh)
	{
		ClothingMesh->SetSkeletalMesh(Mesh);

		if (!IsRunningDedicatedServer())
		{
			// Clear our current override materials
			for (int32 iMat(0); iMat < ClothingMesh->GetNumMaterials(); ++iMat)
			{
				ClothingMesh->SetMaterial(iMat, nullptr);
			}

			// Set our materials
			for (const FSCClothingMaterialPair& Pair : Outfit->MaterialPairs)
			{
				const int32 MaterialIndex = OutfitData->GetMaterialIndexFromSlotIndex(ClothingMesh->SkeletalMesh, Pair.SlotIndex);
				if (MaterialIndex != INDEX_NONE)
				{
					if (const USCClothingSlotMaterialOption* SlotOption = Pair.SelectedSwatch.GetDefaultObject())
					{
						ClothingMesh->SetMaterial(MaterialIndex, SlotOption->MaterialInstance.Get());
					}
				}
			}
		}
	}
	else
	{
		ClothingMesh->SetSkeletalMesh(nullptr);
	}
}

void USCCharacterBodyPartComponent::GetDamageGoreInfo(TSubclassOf<ASCWeapon> WeaponType, FGoreMaskData& DamageGore) const
{
	// Look for a blood mask that is associated with the specified weapon
	for (auto DamageGoreInfo : CombatDamageGoreInfo)
	{
		if (DamageGoreInfo.WeaponType == WeaponType)
		{
			DamageGore.BloodMask = DamageGoreInfo.BloodMask;
			break;
		}
	}

	// Don't have a weapon specific blood mask, so use the default mask
	if (DamageGore.BloodMask == nullptr)
	{
		DamageGore.BloodMask = LimbUVTexture;
	}

	DamageGore.MaterialElementIndices = ParentMaterialElementIndices;
	DamageGore.TextureParameter = TextureParameterName;
}

void USCCharacterBodyPartComponent::DeferredShowLimb()
{
	ASCCounselorCharacter* CounselorOwner = Cast<ASCCounselorCharacter>(GetOwner());
	USkeletalMeshComponent* ParentComponent = ParentSkeletalMesh ? ParentSkeletalMesh : Cast<USkeletalMeshComponent>(GetAttachParent());
	if (PendingMeshSwap.Get() && ParentComponent && CounselorOwner && ClothingMesh)
	{
		SetComponentTickEnabled(true);

		// Torso is a special case
		if (LimbType == ELimb::TORSO)
		{
			// Swap our body to our new horribly disfigured body
			ParentComponent->SetSkeletalMesh(PendingMeshSwap.Get(), false);

			// No accidentally going naked.
			if (ClothingMesh->SkeletalMesh)
			{
				// Show our new clothing
				ClothingMesh->AttachToComponent(ParentComponent, FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("root"));
				ClothingMesh->SetMasterPoseComponent(ParentComponent);
				ClothingMesh->SetVisibility(true, true);

				// and finally hide our old clothing (it probably covers too much anyway)
				CounselorOwner->CounselorOutfitComponent->SetVisibility(false, true);
			}
		}
		else
		{
			SetSkeletalMesh(PendingMeshSwap.Get());

			const FName AttachSocket = GetAttachSocketName();
			// Hide bone on parent mesh where we are showing this limb at
			ParentComponent->HideBoneByName(BoneToHide.IsNone() ? AttachSocket : BoneToHide, EPhysBodyOp::PBO_None);

			// Show this limb
			SetVisibility(true, true);
		}
	}
}

void USCCharacterBodyPartComponent::DeferredShowLimbGoreCap()
{
	if (LimbGoreCap.Get() && GetWorld() && !LimbGoreCapComponent)
	{
		LimbGoreCapComponent = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass());
		LimbGoreCapComponent->SetStaticMesh(LimbGoreCap.Get());
		LimbGoreCapComponent->RegisterComponentWithWorld(GetWorld());
		LimbGoreCapComponent->AttachToComponent(this, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, false), LimbGoreCapSocket);
	}
}

void USCCharacterBodyPartComponent::HideLimbGoreCaps()
{
	if (LimbGoreCapComponent)
	{
		LimbGoreCapComponent->DetachFromComponent(FDetachmentTransformRules(EDetachmentRule::KeepWorld, false));
		LimbGoreCapComponent->DestroyComponent();
		LimbGoreCapComponent = nullptr;
	}

	if (ParentLimbGoreCapComponent)
	{
		ParentLimbGoreCapComponent->DetachFromComponent(FDetachmentTransformRules(EDetachmentRule::KeepWorld, false));
		ParentLimbGoreCapComponent->DestroyComponent();
		ParentLimbGoreCapComponent = nullptr;
	}
}

void USCCharacterBodyPartComponent::DeferredShowParentLimbGoreCap()
{
	if (ParentLimbGoreCap.Get() && GetWorld() && ParentSkeletalMesh && !ParentLimbGoreCapComponent)
	{
		ParentLimbGoreCapComponent = NewObject<USkeletalMeshComponent>(ParentSkeletalMesh, USkeletalMeshComponent::StaticClass());
		ParentLimbGoreCapComponent->SetSkeletalMesh(ParentLimbGoreCap.Get());
		ParentLimbGoreCapComponent->RegisterComponentWithWorld(GetWorld());
		ParentLimbGoreCapComponent->AttachToComponent(ParentSkeletalMesh, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, false), ParentLimbGoreCapSocket);
		ParentLimbGoreCapComponent->SetMasterPoseComponent(ParentSkeletalMesh);
	}
}

void USCCharacterBodyPartComponent::DeferredPlayLimbBloodEffect()
{
	if (LimbBloodEffect.Get())
	{
		UGameplayStatics::SpawnEmitterAttached(LimbBloodEffect.Get(), this, LimbEmitterSocket);
	}
}

void USCCharacterBodyPartComponent::DeferredPlayParentLimbBloodEffect()
{
	if (ParentLimbBloodEffect.Get())
	{
		if (USkeletalMeshComponent* ParentSkelMesh = ParentSkeletalMesh ? ParentSkeletalMesh : Cast<USkeletalMeshComponent>(GetAttachParent()))
		{
			UGameplayStatics::SpawnEmitterAttached(ParentLimbBloodEffect.Get(), ParentSkelMesh, ParentLimbEmitterSocket);
		}
	}
}

void USCCharacterBodyPartComponent::DeferredApplyBloodMask()
{
	if (IsRunningDedicatedServer())
		return;

	if (PendingBloodMask.Get())
	{
		// Limb blood
		for (const auto& LimbIndex : LimbMaterialElementIndices)
		{
			if (LimbIndex < GetNumMaterials())
			{
				if (UMaterialInstanceDynamic* LimbMat = CreateAndSetMaterialInstanceDynamic(LimbIndex))
				{
					LimbMat->SetTextureParameterValue(PendingBloodParameter, PendingBloodMask.Get());
				}
			}
		}

		// Blood on the clothes
		for (int iMat(0); iMat < ClothingMesh->GetNumMaterials(); ++iMat)
		{
			if (UMaterialInstanceDynamic* Mat = ClothingMesh->CreateAndSetMaterialInstanceDynamic(iMat))
			{
#if WITH_EDITOR
				UTexture* CurrentTexture(nullptr);
				const bool bHasParam = Mat->GetTextureParameterValue(PendingBloodParameter, CurrentTexture);
				if (bHasParam)
				{
					UE_LOG(LogTemp, Warning, TEXT("DeferredApplyBloodMask changing %s from %s to %s"), *PendingBloodParameter.ToString(), *GetNameSafe(CurrentTexture), *GetNameSafe(PendingBloodMask.Get()));
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("DeferredApplyBloodMask %s doesn't have param %s!"), *GetNameSafe(Mat), *PendingBloodParameter.ToString());
				}
#endif
				Mat->SetTextureParameterValue(PendingBloodParameter, PendingBloodMask.Get());
			}
		}
	}
}
