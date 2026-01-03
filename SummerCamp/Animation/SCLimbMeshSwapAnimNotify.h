// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "SCCharacterBodyPartComponent.h"
#include "SCLimbMeshSwapAnimNotify.generated.h"

/**
 * @class USCLimbMeshSwapAnimNotify
 */
UCLASS()
class SUMMERCAMP_API USCLimbMeshSwapAnimNotify 
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

	/** The limb to swap the mesh on */
	UPROPERTY(EditAnywhere, Category = "AnimNotify|Limb")
	ELimb LimbToSwap;

	/** The swap name associated in the character's limb */
	UPROPERTY(EditAnywhere, Category = "AnimNotify|Limb")
	FName LimbSwapName;

	/** Optional: The skeletal mesh to swap */
	UPROPERTY(EditAnywhere, Category = "AnimNotify|Limb")
	TAssetPtr<USkeletalMesh> MeshToSwap;
};
