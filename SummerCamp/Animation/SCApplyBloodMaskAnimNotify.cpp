// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCApplyBloodMaskAnimNotify.h"
#include "SCCounselorCharacter.h"

USCApplyBloodMaskAnimNotify::USCApplyBloodMaskAnimNotify(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

FString USCApplyBloodMaskAnimNotify::GetNotifyName_Implementation() const
{
	if (!NotifyDisplayName.IsNone())
		return NotifyDisplayName.ToString();

	return FString(TEXT("SC Apply Blood Mask"));
}

void USCApplyBloodMaskAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(MeshComp->GetOwner()))
	{
		USCCharacterBodyPartComponent* const CounselorLimb = [&]() -> USCCharacterBodyPartComponent*
		{
			switch (Limb)
			{
			case ELimb::HEAD:		return Counselor->FaceLimb;
			case ELimb::TORSO:		return Counselor->TorsoLimb;
			case ELimb::LEFT_ARM:	return Counselor->LeftArmLimb;
			case ELimb::RIGHT_ARM:	return Counselor->RightArmLimb;
			case ELimb::LEFT_LEG:	return Counselor->LeftLegLimb;
			case ELimb::RIGHT_LEG:	return Counselor->RightLegLimb;
			}

			return nullptr;
		}();

		FGoreMaskData Gore;
		Gore.BloodMask = (!BloodMask && CounselorLimb) ? CounselorLimb->LimbUVTexture : BloodMask;
		Gore.TextureParameter = (TextureParameter.IsNone() && CounselorLimb) ? CounselorLimb->TextureParameterName : TextureParameter;
		Gore.MaterialElementIndices = (MaterialElementIndices.Num() == 0 && CounselorLimb) ? CounselorLimb->ParentMaterialElementIndices : MaterialElementIndices;
		Counselor->ApplyGore(Gore);
	}
}