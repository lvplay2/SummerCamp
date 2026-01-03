// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"

#include "SCCabinGroupVolume.generated.h"

/**
 * @class ASCCabinGroupVolume
 */
UCLASS()
class SUMMERCAMP_API ASCCabinGroupVolume
: public AActor
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default")
	UBoxComponent* Volume;

	/** The minimum number of cabins that will spawn in this volume */
	UPROPERTY(EditAnywhere, Category = "Default")
	int32 MinCabinCount;

	/** The maximum number of cabins that will spawn in this volume */
	UPROPERTY(EditAnywhere, Category = "Default")
	int32 MaxCabinCount;
};
