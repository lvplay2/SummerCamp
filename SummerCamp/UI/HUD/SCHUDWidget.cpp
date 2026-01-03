// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCHUDWidget.h"

#include "SCCounselorCharacter.h"
#include "SCDriveableVehicle.h"
#include "SCMiniMapBase.h"
#include "SCImposterOutfit.h"
#include "SCParanoiaKillerCharacter.h"

USCMiniMapBase* USCHUDWidget::GetMinimap()
{
	return Minimap;
}

ASCCounselorCharacter* USCHUDWidget::GetOwningCounselor() const
{
	APawn* OwningPawn = GetOwningPlayerPawn();
	if (OwningPawn && OwningPawn->IsA(ASCCounselorCharacter::StaticClass()))
		return Cast<ASCCounselorCharacter>(OwningPawn);

	// Otherwise we're a vehicle maybe!
	if (ASCDriveableVehicle* Vehicle = Cast<ASCDriveableVehicle>(OwningPawn))
		return Cast<ASCCounselorCharacter>(Vehicle->Driver);

	return nullptr;
}

ESlateVisibility USCHUDWidget::GetParanoiaAbilityVisiblity() const
{
	APawn* OwningPawn = GetOwningPlayerPawn();

	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(OwningPawn))
	{
		if (Counselor->HasSpecialItem(ASCImposterOutfit::StaticClass()))
			return ESlateVisibility::Visible;
	}

	if (ASCParanoiaKillerCharacter* Killer = Cast<ASCParanoiaKillerCharacter>(OwningPawn))
		return ESlateVisibility::Visible;

	return ESlateVisibility::Collapsed;
}
