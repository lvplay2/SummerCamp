// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "SCCharacterBodyPartComponent.h"
#include "SCApplyBloodMaskAnimNotify.generated.h"

/**
 * @class USCApplyBloodMaskAnimNotify
 */
UCLASS()
class SUMMERCAMP_API USCApplyBloodMaskAnimNotify 
: public UAnimNotify
{
	GENERATED_UCLASS_BODY()

	// Begin UAnimNotify interface
	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	// End UAnimNotify interface

	/** Name for this notify */
	UPROPERTY(EditAnywhere, Category = "AnimNotify")
	FName NotifyDisplayName;

	/** Blood mask to be applied. If None, the LimbUVTexture from the specified limb will be used.*/
	UPROPERTY(EditAnywhere, Category = "AnimNotify|BloodMask")
	UTexture* BloodMask;

	/** Limb to apply the mask to */
	UPROPERTY(EditAnywhere, Category = "AnimNotify|BloodMask")
	ELimb Limb;

	/** Texture parameter this blood mask should be applied. If None, the TextureParameterName from the specified limb will be used. */
	UPROPERTY(EditAnywhere, Category = "AnimNotify|BloodMask")
	FName TextureParameter;

	/** Texture parameter this blood mask should be applied. If None, the ParentMaterialElementIndices from the specified limb will be used.*/
	UPROPERTY(EditAnywhere, Category = "AnimNotify|BloodMask")
	TArray<int32> MaterialElementIndices;
};
