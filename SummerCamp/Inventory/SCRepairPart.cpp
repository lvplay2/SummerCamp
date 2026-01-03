// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCRepairPart.h"

#include "ILLInventoryComponent.h"

#include "SCBridgeCore.h"
#include "SCBridgeNavUnit.h"
#include "SCCounselorCharacter.h"
#include "SCDriveableVehicle.h"
#include "SCEscapePod.h"
#include "SCGameMode.h"
#include "SCPhoneJunctionBox.h"
#include "SCPlayerState.h"
#include "SCRepairComponent.h"
#include "SCSpecialMoveComponent.h"
#include "SCVehicleStarterComponent.h"

ASCRepairPart::ASCRepairPart(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	ClosestRepairGoalDistanceSquared = FLT_MAX;
	LastGoalCacheUpdateTime = -9999.f;
	LastGoalDistanceUpdateTime = -9999.f;
}

bool ASCRepairPart::Use(bool bFromInput)
{
	// Get the owner's PlayerState and track item use
	ASCPlayerState* OwnerPS = [&]() -> ASCPlayerState*
	{
		if (UILLInventoryComponent* Inventory = GetOuterInventory())
		{
			if (ASCCharacter* Character = Cast<ASCCharacter>(Inventory->GetCharacterOwner()))
			{
				return Character->GetPlayerState();
			}
		}
		else if (ASCCharacter* Character = Cast<ASCCharacter>(GetOwner()))
		{
			return Character->GetPlayerState();
		}

		return nullptr;
	}();
	
	if (OwnerPS)
	{
		OwnerPS->TrackItemUse(GetClass(), (uint8)ESCItemStatFlags::Used);
	}

	return !bFromInput;
}

bool ASCRepairPart::CanUse(bool bFromInput)
{
	return !bFromInput;
}

void ASCRepairPart::ConditionalUpdateGoalCache() const
{
	if (GetWorld()->GetTimeSeconds()-LastGoalCacheUpdateTime >= 5.f)
	{
		UpdateGoalCache();
	}
}

void ASCRepairPart::ConditionalUpdateGoalDistance() const
{
	UWorld* World = GetWorld();
	const float TimeSeconds = World->GetTimeSeconds();
	if (TimeSeconds-LastGoalDistanceUpdateTime >= 1.f)
	{
		// Update ClosestRepairGoalDistanceSquared
		ClosestRepairGoalDistanceSquared = FLT_MAX;
		for (TPair<USCInteractComponent*,FVector> RepairComponent : RepairGoalCache)
		{
			const float DistanceSq = (RepairComponent.Value - GetActorLocation()).SizeSquared();
			if (ClosestRepairGoalDistanceSquared > DistanceSq)
			{
				ClosestRepairGoalDistanceSquared = DistanceSq;
			}
		}

		// Flag when we did this
		LastGoalDistanceUpdateTime = TimeSeconds;
	}
}

void ASCRepairPart::UpdateGoalCache() const
{
	// Clear the cache
	RepairGoalCache.Empty(RepairGoalCache.Num());

	// Rebuild it from the world
	UWorld* World = GetWorld();
	const ASCGameMode* GameMode = World->GetAuthGameMode<ASCGameMode>();

	auto MapInteractPoint = [](const USCInteractComponent* RepairComponent) -> FVector
	{
		TArray<USceneComponent*> ChildComponents;
		RepairComponent->GetChildrenComponents(false, ChildComponents);
		for (USceneComponent* Child : ChildComponents)
		{
			if (Child->IsA<USCSpecialMoveComponent>())
			{
				return Child->GetComponentLocation();
			}
		}

		return FNavigationSystem::InvalidLocation;
	};

	// Check vehicles
	for (ASCDriveableVehicle* Vehicle : GameMode->GetAllSpawnedVehicles())
	{
		if (!IsValid(Vehicle))
			continue;

		if (Vehicle->bIsRepaired && Vehicle->StartVehicleComponent->IsRepaired())
			continue;

		auto MapVehicleInteractComponent = [](USCRepairComponent* RepairComponent) -> USCInteractComponent*
		{
			if (RepairComponent->IsA<USCVehicleStarterComponent>())
			{
				// Car starters are a special case
				if (ASCDriveableVehicle* VehicleOwner = Cast<ASCDriveableVehicle>(RepairComponent->GetOwner()))
				{
					for (USCVehicleSeatComponent* Seat : VehicleOwner->Seats)
					{
						if (Seat->IsDriverSeat())
						{
							return Seat;
						}
					}
				}
			}

			return RepairComponent;
		};

		USCRepairComponent* RepairComponent = nullptr;
		if (Vehicle->IsRepairPartNeeded(GetClass(), RepairComponent))
		{
			USCInteractComponent* MappedComponent = MapVehicleInteractComponent(RepairComponent);

			// Make sure the car is otherwise ready to go before we try to start it
			if (GetClass() == GameMode->GetCarKeysClass())
			{
				// Only needs the keys
				if (Vehicle->GetNumRepairsStillNeeded() == 0)
					RepairGoalCache.Add(MappedComponent, MapInteractPoint(MappedComponent));
			}
			else if (MappedComponent)
			{
				RepairGoalCache.Add(MappedComponent, MapInteractPoint(MappedComponent));
			}
		}
	}

	// Check Police Phone fuse
	for (ASCPhoneJunctionBox* PhoneBox : GameMode->GetAllPhoneJunctionBoxes())
	{
		if (PhoneBox->HasFuse())
			continue;

		USCRepairComponent* RepairComponent = nullptr;
		if (PhoneBox->IsRepairPartNeeded(GetClass(), RepairComponent))
		{
			RepairGoalCache.Add(RepairComponent, MapInteractPoint(RepairComponent));
		}
	}

	// Check Escape Pods
	for (ASCEscapePod* EscapePod : GameMode->GetAllEscapePods())
	{
		USCRepairComponent* RepairComponent = nullptr;
		if (EscapePod->IsRepairPartNeeded(GetClass(), RepairComponent))
		{
			RepairGoalCache.Add(RepairComponent, MapInteractPoint(RepairComponent));
		}
	}

	// Check Grendel Bridge components
	for (ASCBridgeCore* BridgeCore : GameMode->GetAllBridgeCores())
	{
		USCRepairComponent* RepairComponent = nullptr;
		if (BridgeCore->IsRepairPartNeeded(GetClass(), RepairComponent))
		{
			RepairGoalCache.Add(RepairComponent, MapInteractPoint(RepairComponent));
		}
	}

	for (ASCBridgeNavUnit* NavUnit : GameMode->GetAllBridgeNavUnits())
	{
		USCRepairComponent* RepairComponent = nullptr;
		if (NavUnit->IsRepairPartNeeded(GetClass(), RepairComponent))
		{
			RepairGoalCache.Add(RepairComponent, MapInteractPoint(RepairComponent));
		}
	}

	// Update this while it's in cache
	ConditionalUpdateGoalDistance();

	// Flag when we did this
	LastGoalCacheUpdateTime = World->GetTimeSeconds();
}
