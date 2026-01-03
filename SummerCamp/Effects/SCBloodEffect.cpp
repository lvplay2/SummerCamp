// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCBloodEffect.h"

#include "SCCharacter.h"
#include "SCDriveableVehicle.h"
#include "SCGameInstance.h"

ASCBloodEffect::ASCBloodEffect(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, RaytraceDistance(500.0f)
, DecalSize(10.0f)
, DecalMaxSize(20.0f)
{
	static FName NAME_Root(TEXT("Root"));
	RootComponent = CreateDefaultSubobject<USceneComponent>(NAME_Root);

	static FName NAME_DecalSpawnDirection(TEXT("DecalSpawnDirection"));
	DecalSpawnDirection = CreateDefaultSubobject<UArrowComponent>(NAME_DecalSpawnDirection);
	DecalSpawnDirection->SetupAttachment(RootComponent);

	bAlwaysRelevant = false;
	PrimaryActorTick.bCanEverTick = true;
}

void ASCBloodEffect::SpawnEffect()
{
	if (Particle.Get())
	{
		DeferredPlayParticle();
	}
	else if (!Particle.IsNull())
	{
		// Load and play when it's ready
		USCGameInstance* GameInstance = Cast<USCGameInstance>(GetWorld()->GetGameInstance());
		GameInstance->StreamableManager.RequestAsyncLoad(Particle.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &ThisClass::DeferredPlayParticle));
	}

	if (DecalMaterial)
	{
		if (DecalSpawnDelay > 0.0f)
			GetWorldTimerManager().SetTimer(TimerHandle_SpawnDecal, this, &ASCBloodEffect::SpawnDecal, DecalSpawnDelay);
		else
			SpawnDecal();
	}
}

void ASCBloodEffect::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (Decal)
	{
		const FVector Scale = Decal->GetComponentScale();
		FVector ScaleComposite = DecalForward + DecalRight;

		ScaleComposite = FVector(FMath::Abs<float>(ScaleComposite.X), FMath::Abs<float>(ScaleComposite.Y), FMath::Abs<float>(ScaleComposite.Z));

		CurrentScaleTime += DeltaSeconds;
		const float CurrentSize = FMath::Lerp(1.0f, DecalMaxSize/DecalSize, FMath::Min(CurrentScaleTime / DecalScaleTime, 1.0f));
		ScaleComposite = ScaleComposite * CurrentSize;
		ScaleComposite = FVector(FMath::Max(ScaleComposite.X, 1.0f), FMath::Max(ScaleComposite.Y, 1.0f), FMath::Max(ScaleComposite.Z, 1.0f));
		Decal->SetRelativeScale3D(ScaleComposite);

		SetActorTickEnabled(CurrentScaleTime < DecalScaleTime);
	}
}

void ASCBloodEffect::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_SpawnDecal);
	}

	Super::EndPlay(EndPlayReason);
}

void ASCBloodEffect::DeferredPlayParticle()
{
	if (Particle.Get())
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Particle.Get(), GetTransform());
	}
}

void ASCBloodEffect::SpawnDecal()
{
	const FVector StartLoc = GetActorLocation();
	const FVector ArrowForward = DecalSpawnDirection->GetForwardVector();
	const FVector EndLoc = StartLoc + ArrowForward * RaytraceDistance;

	FCollisionQueryParams CollisionParams(FName(TEXT("SpawnBloodDecal")), true, this);
	if (IgnoreActor)
		CollisionParams.AddIgnoredActor(IgnoreActor);

	FHitResult Hit;

	if (GetWorld()->LineTraceSingleByChannel(Hit, StartLoc, EndLoc, ECollisionChannel::ECC_Visibility, CollisionParams))
	{
		if (Hit.GetActor()->IsA(ASCCharacter::StaticClass()))// TODO: Setup character blood mask instead
			return;

		const FMatrix RotMat = FRotationMatrix::MakeFromX(Hit.ImpactNormal);

		const bool ShouldBeLarge = true;//Hit.GetActor()->IsA(ASCDriveableVehicle::StaticClass());

		Decal = UGameplayStatics::SpawnDecalAttached(DecalMaterial, FVector(ShouldBeLarge ? 10.0f : 1.0f, DecalSize, DecalSize), Hit.Component.Get(), NAME_None, Hit.ImpactPoint, RotMat.Rotator(), EAttachLocation::KeepWorldPosition, DecalLifetime);

		DecalForward = RotMat.Rotator().UnrotateVector(RotMat.GetUnitAxis(EAxis::Z));
		DecalRight = RotMat.Rotator().UnrotateVector(RotMat.GetUnitAxis(EAxis::Y));

		DecalForward = FVector(FMath::Abs<float>(DecalForward.X), FMath::Abs<float>(DecalForward.Y), FMath::Abs<float>(DecalForward.Z));
		DecalRight = FVector(FMath::Abs<float>(DecalRight.X), FMath::Abs<float>(DecalRight.Y), FMath::Abs<float>(DecalRight.Z));

		SetActorTickEnabled(bShouldDecalScale);
	}
}