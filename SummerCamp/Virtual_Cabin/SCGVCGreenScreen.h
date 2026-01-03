// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "SCGVCGreenScreen.generated.h"

UCLASS()
class SUMMERCAMP_API ASCGVCGreenScreen
: public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Green Screen")
	UArrowComponent* m_ObjectAnchor;

	// Sets default values for this actor's properties
	ASCGVCGreenScreen(const FObjectInitializer& ObjectInitializer);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	FVector GetObjectAnchorPos();
};
