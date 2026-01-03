// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCMaterialOverride.generated.h"

/**
 * @struct FMaterialOverrideSetting
 */
USTRUCT(BlueprintType)
struct FMaterialOverrideSetting
{
	GENERATED_USTRUCT_BODY()

	/** The material indices to apply the override material to. */
	UPROPERTY(EditDefaultsOnly)
	TArray<uint32> MaterialIds;

	/** The material to apply. */
	UPROPERTY(EditDefaultsOnly)
	TAssetPtr<UMaterialInstance> OverrideMaterial;
};


/**
* @class USCMaterialOverride
*/
UCLASS(MinimalAPI, const, Blueprintable, BlueprintType)
class USCMaterialOverride
	: public UDataAsset
{
	GENERATED_BODY()
public:
	/** The full list of materials and their slot ids to apply. */
	UPROPERTY(EditDefaultsOnly)
	TArray<FMaterialOverrideSetting> Materials;

	/** Static helper function used to apply a USCMaterialOverride object to a SkeletalMeshComponent. */
	static void ApplyMaterialOverride(const UWorld* World, USCMaterialOverride* MatOverride, TWeakObjectPtr<USkeletalMeshComponent> Mesh);

private:
	/** Internal callback used for when the material reference has finished asnyc loading. */
	void ApplyMaterial(TWeakObjectPtr<USkeletalMeshComponent> Mesh, FMaterialOverrideSetting MatSetting) const;
};