// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCCheatManager.h"

#include "SCCharacter.h"
#include "SCDriveableVehicle.h"
#include "SCGameMode.h"
#include "SCPlayerController.h"
#include "SCPlayerState.h"
#include "SCWorldSettings.h"

USCCheatManager::USCCheatManager(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void USCCheatManager::RepairCar()
{
	for (TActorIterator<ASCDriveableVehicle> It(GetWorld()); It; ++It)
	{
		if (It->VehicleType == EDriveableVehicleType::Car)
			It->ForceRepair();
	}
}

void USCCheatManager::StartCar()
{
	for (TActorIterator<ASCDriveableVehicle> It(GetWorld()); It; ++It)
	{
		if (It->VehicleType == EDriveableVehicleType::Car)
			It->ForceStart();
	}
}

void USCCheatManager::RepairBoat()
{
	for (TActorIterator<ASCDriveableVehicle> It(GetWorld()); It; ++It)
	{
		if (It->VehicleType == EDriveableVehicleType::Boat)
			It->ForceRepair();
	}
}

void USCCheatManager::StartBoat()
{
	for (TActorIterator<ASCDriveableVehicle> It(GetWorld()); It; ++It)
	{
		if (It->VehicleType == EDriveableVehicleType::Boat)
			It->ForceStart();
	}
}

void USCCheatManager::GiveXP(TSubclassOf<USCScoringEvent> XPClass)
{
	if (XPClass)
	{
		for (TActorIterator<ASCCharacter> It(GetWorld(), ASCCharacter::StaticClass()); It; ++It)
		{
			if (It->IsLocallyControlled())
			{
				if (ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
				{
					GameMode->HandleScoreEvent(It->PlayerState, XPClass);
				}
			}
		}
	}
}

void USCCheatManager::RevivePlayer()
{
	if (ASCPlayerController* PC = GetOuterASCPlayerController())
	{
		if (ASCPlayerState* PS = Cast<ASCPlayerState>(PC->PlayerState))
			PS->OnRevivePlayer();

		if (ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
			GameMode->RestartPlayer(PC);
	}
}

void USCCheatManager::SetRaining(bool bRain)
{
	if (UWorld* World = GetWorld())
	{
		if (ASCWorldSettings* WorldSettings = Cast<ASCWorldSettings>(World->GetWorldSettings()))
		{
			if (WorldSettings->GetIsRaining() != bRain)
				WorldSettings->SetRaining(bRain);
		}
	}
}

void USCCheatManager::ToggleInfiniteSweater()
{
	if (ASCPlayerController* PC = GetOuterASCPlayerController())
	{
		if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(PC->AcknowledgedPawn))
		{
			Counselor->bUnlimitedSweaterStun = !Counselor->bUnlimitedSweaterStun;
		}
	}
}
