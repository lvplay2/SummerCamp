// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCDetachLimbAnimNotify.h"
#include "SCCounselorCharacter.h"

FString ELimbToString(ELimb Limb)
{
	switch (Limb)
	{
	case ELimb::HEAD:		return TEXT("Head");
	case ELimb::TORSO:		return TEXT("Torso");
	case ELimb::LEFT_ARM:	return TEXT("Left Arm");
	case ELimb::RIGHT_ARM:	return TEXT("Right Arm");
	case ELimb::LEFT_LEG:	return TEXT("Left Leg");
	case ELimb::RIGHT_LEG:	return TEXT("Right Leg");
	}

	return TEXT("");
}

USCDetachLimbAnimNotify::USCDetachLimbAnimNotify(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, bShowLimbAfterDetach(true)
{
}

FString USCDetachLimbAnimNotify::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Detach Limb: %s"), *ELimbToString(LimbToDetach));
}

void USCDetachLimbAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(MeshComp->GetOwner()))
	{
		USCCharacterBodyPartComponent* Limb = nullptr;
		switch (LimbToDetach)
		{
			case ELimb::HEAD:		Limb = Counselor->FaceLimb; break;
			case ELimb::TORSO:		Limb = Counselor->TorsoLimb; break;
			case ELimb::LEFT_ARM:	Limb = Counselor->LeftArmLimb; break;
			case ELimb::RIGHT_ARM:	Limb = Counselor->RightArmLimb; break;
			case ELimb::LEFT_LEG:	Limb = Counselor->LeftLegLimb; break;
			case ELimb::RIGHT_LEG:	Limb = Counselor->RightLegLimb; break;
		}

		// Detach the limb
		if (Limb)
		{
			Counselor->DetachLimb(Limb, Counselor->GetActorRotation().RotateVector(Impulse), BloodMaskOverride, bShowLimbAfterDetach);
		}
	}
}
