// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCMaterialOverride.h"

#include "SCGameInstance.h"

void USCMaterialOverride::ApplyMaterialOverride(const UWorld* World, USCMaterialOverride* MatOverride, TWeakObjectPtr<USkeletalMeshComponent> Mesh)
{
	// no world, no game instance. No game instance, no async loading. No point... just leave
	if (!World)
		return;
	if (IsRunningDedicatedServer())
		return;

	USCGameInstance* GameInstance = Cast<USCGameInstance>(World->GetGameInstance());

	// loop through all of the material settings we want to apply
	for (const FMaterialOverrideSetting& MatSetting : MatOverride->Materials)
	{
		// grab the raw asset for the material instance
		TAssetPtr<UMaterialInstance> OverrideMat = MatSetting.OverrideMaterial;

		// check to see if the material is loaded
		if (OverrideMat.Get())
		{
			// material was already loaded, go ahead and apply it to the mesh
			MatOverride->ApplyMaterial(Mesh, MatSetting);
		}
		else if (!OverrideMat.IsNull()) // if Get() is null then this material hasnt been loaded into memory yet
		{
			// request the async load of the material and pass the apply callback so it can be applied as soon as it completes
			GameInstance->StreamableManager.RequestAsyncLoad(OverrideMat.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(MatOverride, &ThisClass::ApplyMaterial, Mesh, MatSetting));
		}
	}
}

void USCMaterialOverride::ApplyMaterial(TWeakObjectPtr<USkeletalMeshComponent> Mesh, FMaterialOverrideSetting MatSetting) const
{
	if (IsRunningDedicatedServer())
		return;

	// loop through each index and apply the material to that slot
	for (int32 Index : MatSetting.MaterialIds)
	{
		if (Mesh.IsValid())
			Mesh.Get()->CreateDynamicMaterialInstance(Index, MatSetting.OverrideMaterial.Get());
	}
}
