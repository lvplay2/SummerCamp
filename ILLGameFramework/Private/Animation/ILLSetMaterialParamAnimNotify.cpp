// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLSetMaterialParamAnimNotify.h"

UILLSetMaterialParamAnimNotify::UILLSetMaterialParamAnimNotify(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

FString UILLSetMaterialParamAnimNotify::GetNotifyName_Implementation() const
{
	if (!NotifyDisplayName.IsNone())
		return NotifyDisplayName.ToString();

	return FString(TEXT("ILL Set Material Parameter"));
}

void UILLSetMaterialParamAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (IsRunningDedicatedServer())
		return;

	// Set scalar parameters
	for (const FScalarParameter& ScalarParam : ScalarParameters)
	{
		if (ScalarParam.MaterialIndex >= 0 && ScalarParam.MaterialIndex < MeshComp->GetNumMaterials())
		{
			if (UMaterialInstanceDynamic* Mat = MeshComp->CreateAndSetMaterialInstanceDynamic(ScalarParam.MaterialIndex))
			{
				Mat->SetScalarParameterValue(ScalarParam.ParameterName, ScalarParam.ScalarValue);
			}
		}
	}

	// Set vector parameters
	for (const FVectorParameter& VectorParam : VectorParameters)
	{
		if (VectorParam.MaterialIndex >= 0 && VectorParam.MaterialIndex < MeshComp->GetNumMaterials())
		{
			if (UMaterialInstanceDynamic* Mat = MeshComp->CreateAndSetMaterialInstanceDynamic(VectorParam.MaterialIndex))
			{
				Mat->SetVectorParameterValue(VectorParam.ParameterName, VectorParam.ColorValue);
			}
		}
	}

	// Set texture parameters
	for (const FTextureParameter& TextureParam : TextureParameters)
	{
		if (TextureParam.MaterialIndex >= 0 && TextureParam.MaterialIndex < MeshComp->GetNumMaterials())
		{
			if (UMaterialInstanceDynamic* Mat = MeshComp->CreateAndSetMaterialInstanceDynamic(TextureParam.MaterialIndex))
			{
				Mat->SetTextureParameterValue(TextureParam.ParameterName, TextureParam.Texture);
			}
		}
	}
}
