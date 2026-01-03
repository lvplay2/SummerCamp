// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once
#include "SCMaterialOverride.h"
#include "SCJasonSkin.generated.h"

/**
* @class USCJasonSkin
*/
UCLASS(MinimalAPI, const, Blueprintable, BlueprintType)
class USCJasonSkin
	: public UDataAsset
{
	GENERATED_BODY()
public:

	/** The material set to be applied to Jasons body mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftClassPtr<USCMaterialOverride> MeshMaterial;

	/** The material set to be applied to Jasons mask. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftClassPtr<USCMaterialOverride> MaskMaterial;

	/** The counselor near music that should be associated with this skin. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	USoundBase* OverrideNearCounselorMusic;

	/** The unlock level this skin becomes available, leave 0 for always unlocked. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 UnlockLevel;

	/** The dlc string this skin is associated, leave blank for no dlc. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FString DLCUnlock;

	USCJasonSkin()
		: MeshMaterial(nullptr)
		, MaskMaterial(nullptr)
		,UnlockLevel(0) {}

	FORCEINLINE bool operator ==(const USCJasonSkin& Other) const
	{
		return (MeshMaterial == Other.MeshMaterial && MaskMaterial == Other.MaskMaterial);
	}
};