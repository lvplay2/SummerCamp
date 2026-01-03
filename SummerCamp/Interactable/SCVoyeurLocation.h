// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "SCInteractComponent.h"
#include "SCSplineCamera.h"
#include "SCVoyeurLocation.generated.h"

enum class EILLInteractMethod : uint8;

/**
* @class ASCVoyeurLocation
*/
UCLASS()
class SUMMERCAMP_API ASCVoyeurLocation
	: public AActor
{
	GENERATED_BODY()

public:
	ASCVoyeurLocation(const FObjectInitializer& ObjectInitializer);

	// BEGIN AActor interface
	virtual void PostInitializeComponents() override;
	// END AActor interface

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	USCSplineCamera* SplineCamera;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	USCInteractComponent* VoyeurInteract;

	UFUNCTION()
	int32 CanInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation);

};
