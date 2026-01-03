// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "SCTeddyBearTriggerVolume.generated.h"

UCLASS()
class SUMMERCAMP_API ASCTeddyBearTriggerVolume 
: public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASCTeddyBearTriggerVolume(const FObjectInitializer& ObjectInitializer);

	// Begin AActor interface
	virtual void PostInitializeComponents() override;
	// End AActor interface

	// Handle overlap events
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	// Trigger Area
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default")
	UBoxComponent* TriggerArea;
};
