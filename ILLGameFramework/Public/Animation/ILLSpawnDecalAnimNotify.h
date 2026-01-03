// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "ILLSpawnDecalAnimNotify.generated.h"

/** Decal info */
USTRUCT()
struct FDecalInfo
{
	GENERATED_USTRUCT_BODY()

	FDecalInfo()
	: DecalDepth(1.0f)
	{
	}

	/** The material for the decal */
	UPROPERTY(EditAnywhere, Category = "Decal")
	UMaterialInterface* DecalMaterial;

	/** Apply random roll rotation to the decal */
	UPROPERTY(EditAnywhere, Category = "Decal")
	bool bApplyRandomRotation;

	/** Width and height of DecalMaterial */
	UPROPERTY(EditAnywhere, Category = "Decal")
	float DecalSize;

	/** Depth of DecalMaterial */
	UPROPERTY(EditAnywhere, Category = "Decal")
	float DecalDepth;

	/** How far should the ray trace go */
	UPROPERTY(EditAnywhere, Category = "Decal")
	float RayLength;

	/** The socket on the character to start the ray trace */
	UPROPERTY(EditAnywhere, Category = "Decal")
	FName CharacterSocket;

	/** Offset from the CharacterSocket */
	UPROPERTY(EditAnywhere, Category = "Decal")
	FVector SocketOffset;

	/** The direction the ray trace should go in relation from CharacterSocket */
	UPROPERTY(EditAnywhere, Category = "Decal")
	FVector RayDirection;
};

/**
 * @class UILLSpawnDecalAnimNotify
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLSpawnDecalAnimNotify 
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

	/** Decals to spawn */
	UPROPERTY(EditAnywhere, Category = "AnimNotify|Decals")
	TArray<FDecalInfo> Decals;
};
