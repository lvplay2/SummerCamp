// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "SCRepairPartSpawner.generated.h"

class ASCRepairPart;

/**
 * @class ASCRepairPartSpawner
 */
UCLASS()
class SUMMERCAMP_API ASCRepairPartSpawner
: public AActor
{
	GENERATED_UCLASS_BODY()

public:
	virtual bool SpawnPart(UClass* PartClass);

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USceneComponent* Root;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	class USCActorSpawnerComponent* ActorSpawner;

	FORCEINLINE ASCRepairPart* GetSpawnedPart() const { return SpawnedPart; }

private:
	UPROPERTY()
	ASCRepairPart* SpawnedPart;
};
