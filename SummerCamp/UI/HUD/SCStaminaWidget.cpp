// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCStaminaWidget.h"
#include "SCCounselorCharacter.h"
#include "SCDriveableVehicle.h"

USCStaminaWidget::USCStaminaWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

float USCStaminaWidget::GetStamina() const
{
	ASCCounselorCharacter* Counselor = nullptr;

	if (GetOwningPlayerPawn() == nullptr)
		return 0.0f;

	const bool IsCar = GetOwningPlayerPawn()->IsA(ASCDriveableVehicle::StaticClass());
	Counselor = Cast<ASCCounselorCharacter>(IsCar ? Cast<ASCDriveableVehicle>(GetOwningPlayerPawn())->Driver : GetOwningPlayerPawn());
	if (Counselor)
	{
		return Counselor->GetStamina() / Counselor->GetMaxStamina();
	}

	return 0.f;
}

bool USCStaminaWidget::CanUseStamina() const
{
	ASCCounselorCharacter* Counselor = nullptr;

	if (GetOwningPlayerPawn() == nullptr)
		return false;

	const bool IsCar = GetOwningPlayerPawn()->IsA(ASCDriveableVehicle::StaticClass());
	Counselor = Cast<ASCCounselorCharacter>(IsCar ? Cast<ASCDriveableVehicle>(GetOwningPlayerPawn())->Driver : GetOwningPlayerPawn());
	if (Counselor)
	{
		return !Counselor->bStaminaDepleted;
	}

	return false;
}

float USCStaminaWidget::GetCurrentFear() const
{
	ASCCounselorCharacter* Counselor = nullptr;

	if (GetOwningPlayerPawn() == nullptr)
		return 0.0f;

	const bool IsCar = GetOwningPlayerPawn()->IsA(ASCDriveableVehicle::StaticClass());
	Counselor = Cast<ASCCounselorCharacter>(IsCar ? Cast<ASCDriveableVehicle>(GetOwningPlayerPawn())->Driver : GetOwningPlayerPawn());
	if (Counselor)
	{
		return Counselor->GetNormalizedFear();
	}

	return 0.f;
}
