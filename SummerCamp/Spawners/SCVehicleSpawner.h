// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "SCVehicleSpawner.generated.h"

/**
 * @class ASCVehicleSpawner
 */
UCLASS()
class SUMMERCAMP_API ASCVehicleSpawner
: public AActor
{
	GENERATED_UCLASS_BODY()

public:
	virtual bool SpawnVehicle(TSubclassOf<AActor> ClassOverride = nullptr);

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UBillboardComponent* Billboard;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	class USCActorSpawnerComponent* ActorSpawner;

	FORCEINLINE AActor* GetSpawnedVehicle() const { return SpawnedVehicle; }

private:
	AActor* SpawnedVehicle;
};
