// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCShoreItemSpawner.h"

#include "UObjectToken.h"
#include "MapErrors.h"
#include "MessageLog.h"

ASCShoreItemSpawner::ASCShoreItemSpawner(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	USceneComponent* SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComp"));

	RootComponent = SceneComponent;
#if WITH_EDITORONLY_DATA
	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));

	if (!IsRunningCommandlet())
	{
		// Structure to hold one-time initialization
		struct FConstructorStatics
		{
			ConstructorHelpers::FObjectFinderOptional<UTexture2D> ShoreIconObject;
			FName ID_ShoreItemSpawner;
			FText NAME_ShoreSpawner;
			FConstructorStatics()
				: ShoreIconObject(TEXT("/Game/UI/Interact/Textures/Interaction_Icons/Img_Weapon_Bat_Pickup_01"))
				, ID_ShoreItemSpawner(TEXT("ShoreItemSpawner"))
				, NAME_ShoreSpawner(NSLOCTEXT("SpriteCategory", "ShoreItemSpawner", "Shore Item Spawners"))
			{
			}
		};
		static FConstructorStatics ConstructorStatics;

		if (SpriteComponent)
		{
			SpriteComponent->Sprite = ConstructorStatics.ShoreIconObject.Get();
			SpriteComponent->RelativeScale3D = FVector(0.35f, 0.35f, 0.35f);
			SpriteComponent->ScreenSize = 1.f;
			SpriteComponent->SpriteInfo.Category = ConstructorStatics.ID_ShoreItemSpawner;
			SpriteComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_ShoreSpawner;
			SpriteComponent->bIsScreenSizeScaled = true;

			SpriteComponent->SetupAttachment(SceneComponent);
		}
	}
#endif

	bHidden = true;
	bCanBeDamaged = false;
}



#if WITH_EDITOR

void ASCShoreItemSpawner::CheckForErrors()
{
	Super::CheckForErrors();

	FMessageLog MapCheck("MapCheck");

	// Check vertical placement
	FHitResult Hit(1.f);
	const FVector TraceStart = GetActorLocation();
	const FVector TraceEnd = GetActorLocation() - FVector(0.f, 0.f, 100.0f);
	static FName NAME_ShoreSpawnerFindFloor = FName(TEXT("ShoreSpawnerFindFloor"));
	GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, FCollisionQueryParams(NAME_ShoreSpawnerFindFloor, false));
	if (!Hit.bBlockingHit)
	{
		static FName NAME_ShoreSpawnerUnderGround(TEXT("ShoreSpawnerUnderGround"));
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("ActorName"), FText::FromString(GetName()));
		MapCheck.Warning()
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(FText::Format(FText::FromString(TEXT("{ActorName} is under the ground")), Arguments)))
			->AddToken(FMapErrorToken::Create(NAME_ShoreSpawnerUnderGround));
	}
}
#endif