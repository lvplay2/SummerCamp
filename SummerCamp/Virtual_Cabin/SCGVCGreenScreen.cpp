// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGVCGreenScreen.h"


// Sets default values
ASCGVCGreenScreen::ASCGVCGreenScreen(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	UArrowComponent* RootArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("RootComponent"));
	m_ObjectAnchor = CreateDefaultSubobject<UArrowComponent>(TEXT("ObjectAnchor"));
	m_ObjectAnchor->SetupAttachment(RootArrow);
}

// Called when the game starts or when spawned
void ASCGVCGreenScreen::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASCGVCGreenScreen::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

FVector ASCGVCGreenScreen::GetObjectAnchorPos()
{
	if (m_ObjectAnchor != nullptr)
	{
		return m_ObjectAnchor->GetComponentLocation();
	}
	return FVector(0,0,0);
}