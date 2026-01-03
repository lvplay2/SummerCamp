// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Components/BoxComponent.h"
#include "SCIndoorComponent.generated.h"

/**
 * @class USCIndoorComponent
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SUMMERCAMP_API USCIndoorComponent
: public UBoxComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USCIndoorComponent();

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Shack")
	bool IsJasonsShack;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Shack")
	bool IsPamelasHeadInHere;
};
