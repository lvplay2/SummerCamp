// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCLimbMeshSwapAnimNotify.h"
#include "SCCounselorCharacter.h"

USCLimbMeshSwapAnimNotify::USCLimbMeshSwapAnimNotify(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

FString USCLimbMeshSwapAnimNotify::GetNotifyName_Implementation() const
{
	if (!NotifyDisplayName.IsNone())
		return NotifyDisplayName.ToString();

	return FString(TEXT("SC Swap Limb Mesh"));
}

void USCLimbMeshSwapAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(MeshComp->GetOwner()))
	{
		USCCharacterBodyPartComponent* const Limb = [&]() -> USCCharacterBodyPartComponent*
		{
			switch (LimbToSwap)
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

		// Swap the mesh
		if (Limb)
		{
			Limb->ShowLimb(LimbSwapName, MeshToSwap);
		}
	}
}



