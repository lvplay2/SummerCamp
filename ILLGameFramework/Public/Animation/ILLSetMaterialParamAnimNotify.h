// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "Materials/MaterialParameterCollection.h"
#include "ILLSetMaterialParamAnimNotify.generated.h"

/** A scalar parameter */
USTRUCT()
struct FScalarParameter // CLEANUP: pjackson: FILLNotifyScalarParameter, scope to UILLSetMaterialParamAnimNotify?
{
	GENERATED_USTRUCT_BODY()

	FScalarParameter()
	{
	}

	/** Material index to set the parameter */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	int32 MaterialIndex;

	/** The name of the parameter */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FName ParameterName;

	/** The scalar value for this parameter */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float ScalarValue;
};

/** A vector parameter */
USTRUCT()
struct FVectorParameter // CLEANUP: pjackson: @see FScalarParameter
{
	GENERATED_USTRUCT_BODY()

	FVectorParameter()
	{
	}

	/** Material index to set the parameter */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	int32 MaterialIndex;

	/** The name of the parameter */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FName ParameterName;

	/** The color value for this parameter */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FLinearColor ColorValue;
};

/** A texture parameter */
USTRUCT()
struct FTextureParameter // CLEANUP: pjackson: @see FScalarParameter
{
	GENERATED_USTRUCT_BODY()

	FTextureParameter()
	{
	}

	/** Material index to set the parameter */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	int32 MaterialIndex;

	/** The name of the parameter */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FName ParameterName;

	/** Texture to set */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	UTexture* Texture; // OPTIMIZE: pjackson: TAssetPtr<> and stream!
};

/**
* @class UILLSetMaterialParamAnimNotify
*/
UCLASS(meta = (DisplayName = "ILL Set Material Parameter"))
class ILLGAMEFRAMEWORK_API UILLSetMaterialParamAnimNotify 
: public UAnimNotify
{
	GENERATED_UCLASS_BODY()
	
	// Begin UAnimNotify interface
	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	// End UAnimNotify interface

	/** Scalar parameters to be set */
	UPROPERTY(EditAnywhere, Category = "AnimNotify|Material")
	TArray<FScalarParameter> ScalarParameters;

	/** Vector parameters to be set */
	UPROPERTY(EditAnywhere, Category = "AnimNotify|Material")
	TArray<FVectorParameter> VectorParameters;

	/** Texture parameters to be set */
	UPROPERTY(EditAnywhere, Category = "AnimNotify|Material")
	TArray<FTextureParameter> TextureParameters;

	/** Name for this notify */
	UPROPERTY(EditAnywhere, Category = "AnimNotify")
	FName NotifyDisplayName;
};
