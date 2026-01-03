// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "SCCabinSpawner.generated.h"

/**
 * @class ASCCabinSpawner
 */
UCLASS()
class SUMMERCAMP_API ASCCabinSpawner
: public AActor
{
	GENERATED_UCLASS_BODY()

public:
	virtual bool SpawnCabin();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USceneComponent* Root;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	class USCActorSpawnerComponent* ActorSpawner;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UStaticMeshComponent* PreviewMesh;

private:
	bool bCabinSpawned;
};