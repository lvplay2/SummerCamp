// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPowerPlant.h"

#include "SCPoweredObject.h"
#include "SCPowerGridVolume.h"

USCPowerPlant::USCPowerPlant(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

bool USCPowerPlant::Register(AActor* PoweredActor)
{
	if (!IsValid(PoweredActor))
		return false;

	// See if we have a grid that we already fit with
	bool bHookedIntoGrid = false;
	for (ASCPowerGridVolume* Grid : PowerGrids)
	{
		float SqDistance(0.f);
		FVector ClosestPoint(FVector::ZeroVector);
		const bool bFoundCollision = Grid->PoweredArea->GetSquaredDistanceToCollision(
			PoweredActor->GetActorLocation(), SqDistance, ClosestPoint);

		if (bFoundCollision && SqDistance <= KINDA_SMALL_NUMBER)
		{
			Grid->AddPoweredObject(PoweredActor);
			bHookedIntoGrid = true;
			break;
		}
	}

	// Nobody contains us, store for later
	if (bHookedIntoGrid == false)
	{
		OrphanedPoweredActors.Add(PoweredActor);
	}

	return true;
}

bool USCPowerPlant::Register(USceneComponent* PoweredComponent)
{
	if (!IsValid(PoweredComponent))
		return false;

	// See if we have a grid that we already fit with
	bool bHookedIntoGrid = false;
	for (ASCPowerGridVolume* Grid : PowerGrids)
	{
		float SqDistance(0.f);
		FVector ClosestPoint(FVector::ZeroVector);
		const bool bFoundCollision = Grid->PoweredArea->GetSquaredDistanceToCollision(
			PoweredComponent->GetComponentLocation(), SqDistance, ClosestPoint);

		if (bFoundCollision && SqDistance <= KINDA_SMALL_NUMBER)
		{
			Grid->AddPoweredObject(PoweredComponent);
			bHookedIntoGrid = true;
			break;
		}
	}

	// Nobody contains us, store for later
	if (bHookedIntoGrid == false)
	{
		OrphanedPoweredComponents.Add(PoweredComponent);
	}

	return true;
}

bool USCPowerPlant::Register(ASCPowerGridVolume* PowerGrid)
{
	// Check all our actors
	for (int32 iObject(0); iObject < OrphanedPoweredActors.Num(); ++iObject)
	{
		float SqDistance(0.f);
		FVector ClosestPoint(FVector::ZeroVector);
		AActor* PoweredActor = OrphanedPoweredActors[iObject];
		const bool bFoundCollision = PowerGrid->PoweredArea->GetSquaredDistanceToCollision(
			PoweredActor->GetActorLocation(), SqDistance, ClosestPoint);

		if (bFoundCollision && SqDistance <= KINDA_SMALL_NUMBER)
		{
			PowerGrid->AddPoweredObject(PoweredActor);
			OrphanedPoweredActors.RemoveAt(iObject--);
		}
	}

	// Check all our components
	for (int32 iObject(0); iObject < OrphanedPoweredComponents.Num(); ++iObject)
	{
		float SqDistance(0.f);
		FVector ClosestPoint(FVector::ZeroVector);
		USceneComponent* PoweredComponent = OrphanedPoweredComponents[iObject];
		const bool bFoundCollision = PowerGrid->PoweredArea->GetSquaredDistanceToCollision(
			PoweredComponent->GetComponentLocation(), SqDistance, ClosestPoint);

		if (bFoundCollision && SqDistance <= KINDA_SMALL_NUMBER)
		{
			PowerGrid->AddPoweredObject(PoweredComponent);
			OrphanedPoweredComponents.RemoveAt(iObject--);
		}
	}

	// Add it to the pile
	PowerGrids.Add(PowerGrid);

	return true;
}

void USCPowerPlant::DebugDraw(UWorld* World, FVector ReferencePoint, int32 DebugLevel) const
{
	if (DebugLevel < 1)
		return;

	FColor DrawColor = FColor::Cyan;
	const auto DrawArrow = [&](FVector ObjectLocation)
	{
		// Arrow (far)/sphere (near)
		const FVector ToItem = ObjectLocation - ReferencePoint;
		if (ToItem.SizeSquared() <= (150.f * 150.f))
		{
			DrawDebugSphere(World, ObjectLocation, 25.f, 8, DrawColor, false, 0.f, 0, 1.f);
		}
		else
		{
			const float Distance = ToItem.Size();
			const FVector Direction = ToItem / Distance;
			const float DistanceScale = FMath::Lerp(50.f, 200.f, FMath::Clamp(Distance / 10000.f, 0.f, 1.f));
			DrawDebugDirectionalArrow(World, ReferencePoint, ReferencePoint + Direction * DistanceScale, 15.f, DrawColor, false, 0.f, 0, 1.f);
		}
	};

	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Cyan, FString(TEXT("Power Plant Stats")), false);

	// Grids
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Cyan, FString::Printf(TEXT("%d power grids"), PowerGrids.Num()), false);
	for (const ASCPowerGridVolume* Grid : PowerGrids)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, Grid->IsDeactivated() ? FColor::Red : FColor::Green, FString::Printf(TEXT(" - %s (Powering %d objects)"), *Grid->GetName(), Grid->GetPoweredObjectCount()), false);
	}

	// Actors
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Cyan, FString::Printf(TEXT("%d orphaned actors"), OrphanedPoweredActors.Num()), false);
	if (DebugLevel >= 2)
	{
		for (const AActor* Actor : OrphanedPoweredActors)
		{
			const FVector ObjectLocation = Actor->GetActorLocation();
			DrawArrow(ObjectLocation);

			if (DebugLevel >= 3)
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Cyan, FString::Printf(TEXT(" - %s @ %s"), *Actor->GetName(), *ObjectLocation.ToString()), false);
		}
	}

	// Components
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Cyan, FString::Printf(TEXT("%d orphaned components"), OrphanedPoweredComponents.Num()), false);
	if (DebugLevel >= 2)
	{
		for (const USceneComponent* Component : OrphanedPoweredComponents)
		{
			const FVector ObjectLocation = Component->GetComponentLocation();
			DrawArrow(ObjectLocation);

			if (DebugLevel >= 3)
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Cyan, FString::Printf(TEXT(" - %s : %s @ %s"), *Component->GetOwner()->GetName(), *Component->GetName(), *ObjectLocation.ToString()), false);
		}
	}
}
