// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCPoweredObject.h"
#include "SCPowerPlant.generated.h"

class ASCPowerGridVolume;

/**
 * @class USCPowerPlant
 * @brief Handles registration of any powered object to the closest power grid to that object. The grid itself controls turning that object on and off
 * @see USCPowerGrid
 */
UCLASS()
class SUMMERCAMP_API USCPowerPlant
: public UObject
{
	GENERATED_BODY()

public:
	USCPowerPlant(const FObjectInitializer& ObjectInitializer);

	/** Adds an actor to the power plant, searching for valid power grids */
	bool Register(AActor* PoweredActor);

	/** Adds a component to the power plant, searching for valid power grids */
	bool Register(USceneComponent* PoweredComponent);

	/** Adds a power grid to the power plant, collecting all actors and components possible in the process */
	bool Register(ASCPowerGridVolume* PowerGrid);

	/**
	 * Output debug information for the power plant
	 * @param ReferencePoint - Position to draw arrow from
	 * @param DebugLevel - 1=General Stats,2=Arrows to orphaned actors/components,3=Names/Locations of actors/components
	 */
	void DebugDraw(UWorld* World, FVector ReferencePoint, int32 DebugLevel) const;

	/** @return A list of all our power grids */
	UFUNCTION(BlueprintPure, Category = "Power")
	const TArray<ASCPowerGridVolume*>& GetPowerGrids() const { return PowerGrids; }

private:
	// Any actors that could not find a grid to attach to
	UPROPERTY()
	TArray<AActor*> OrphanedPoweredActors;

	// Any components that could not find a grid to attach to
	UPROPERTY()
	TArray<USceneComponent*> OrphanedPoweredComponents;

	// List of all our power grids
	UPROPERTY()
	TArray<ASCPowerGridVolume*> PowerGrids;
};
