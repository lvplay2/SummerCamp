// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPlayerCameraManager.h"

#include "SCGameInstance.h"
#include "SCGameState.h"
#include "SCWorldSettings.h"

#if !UE_BUILD_SHIPPING
TAutoConsoleVariable<int32> CVarDisableRainUpdates(TEXT("sc.RainPauseUpdating"), 0,
	TEXT("Will stop updating rain positions/visibility\n")
);

TAutoConsoleVariable<int32> CVarDebugRain(TEXT("sc.RainDebug"), 0,
	TEXT("Draws information about rain so you can debug it\n")
);
#endif // !UE_BUILD_SHIPPING

ASCPlayerCameraManager::ASCPlayerCameraManager(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, RainZOffset(750.f)
, RainResetDistance(500.f)
, RainRayTraceTickDelay(1.f / 8.f) // 8 Hz
, RainOcclusionHeight(250.f)
, RainGridSize(8)
, RainParticleSize(500.f)
, bIsRaining(false)
, PreviousLocation(FVector::ZeroVector)
{
	ViewPitchMin = -87.f;
	ViewPitchMax = 87.f;
	SetActorHiddenInGame(false);
}

void ASCPlayerCameraManager::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (ASCWorldSettings* WorldSettings = World ? Cast<ASCWorldSettings>(World->GetWorldSettings()) : nullptr)
	{
		if (WorldSettings->GetIsRaining())
			EnableRain();
		else
			DisableRain();

		WorldSettings->OnRainingStateChanged.AddUniqueDynamic(this, &ThisClass::OnRainUpdatedInWorldSettings);
	}
}

void ASCPlayerCameraManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearAllTimersForObject(this);
	}

	Super::EndPlay(EndPlayReason);
}

void ASCPlayerCameraManager::Destroyed()
{
	Super::Destroyed();

	// Cleanup our rain particle when destroying
	if (!RainParticleSystem.IsNull())
	{
		if (UWorld* World = GetWorld())
		{
			if (USCGameInstance* GameInstance = Cast<USCGameInstance>(World->GetGameInstance()))
			{
				GameInstance->StreamableManager.Unload(RainParticleSystem.ToSoftObjectPath());
				RainParticleSystem = nullptr;
			}
		}
	}
}

void ASCPlayerCameraManager::UpdateCamera(float DeltaTime)
{
	Super::UpdateCamera(DeltaTime);

	UpdateRain(DeltaTime);

#if !UE_BUILD_SHIPPING
	if (CVarDebugRain.GetValueOnGameThread())
	{
		DebugDraw();
	}
#endif // !UE_BUILD_SHIPPING
}

void ASCPlayerCameraManager::OnRainUpdatedInWorldSettings(bool bRainEnabled)
{
	if (bRainEnabled == bIsRaining)
		return;

	if (bRainEnabled)
		EnableRain();
	else
		DisableRain();
}

void ASCPlayerCameraManager::EnableRain()
{
	if (bIsRaining)
		return;

	// Gotta have at least one rain cloud
	RainGridSize = FMath::Max(RainGridSize, 1);

	UWorld* World = GetWorld();
	const float CurrentTime = World->GetTimeSeconds();

	// Find our starting position
	const float MinPosition(-RainParticleSize * ((float)RainGridSize * 0.5f) + (RainParticleSize * 0.5f));
	FVector RelativePosition(MinPosition, MinPosition, RainZOffset);

	// Make all our cells
	for (int32 iRow(0); iRow < RainGridSize; ++iRow)
	{
		for (int32 iCol(0); iCol < RainGridSize; ++iCol)
		{
			FSCRainGridCell NewCell;
			NewCell.RainComponent = NewObject<UParticleSystemComponent>(this, NAME_None);
			NewCell.RainComponent->bAbsoluteLocation = true;
			NewCell.RainComponent->bAbsoluteRotation = true;
			NewCell.RainComponent->bAbsoluteScale = true;
			NewCell.RainComponent->bAutoDestroy = false;
			NewCell.RainComponent->bAutoActivate = true;
			NewCell.RainComponent->RegisterComponentWithWorld(World);

			NewCell.RelativeOffset = RelativePosition;
			NewCell.NextTraceTimestamp = -RainRayTraceTickDelay; // Force an intial update

			RainGrid.Add(NewCell);

			RelativePosition.Y += RainParticleSize;
		}

		// Reset our column position
		RelativePosition.Y = MinPosition;
		RelativePosition.X += RainParticleSize;
	}

	// Rain is on!
	bIsRaining = true;

	// Load and/or apply our particle system
	if (RainParticleSystem.IsValid())
	{
		OnRainParticlesLoaded();
	}
	else
	{
		FStreamableDelegate Delegate;
		Delegate.BindUObject(this, &ThisClass::OnRainParticlesLoaded);
		USCGameInstance* GameInstance = Cast<USCGameInstance>(World->GetGameInstance());
		GameInstance->StreamableManager.RequestAsyncLoad(RainParticleSystem.ToSoftObjectPath(), Delegate);
	}
}

void ASCPlayerCameraManager::DisableRain()
{
	if (!bIsRaining)
		return;

	// Clean up our components
	for (FSCRainGridCell& Cell : RainGrid)
	{
		if (IsValid(Cell.RainComponent))
		{
			Cell.RainComponent->DeactivateSystem();
			Cell.RainComponent->DestroyComponent();
			Cell.RainComponent = nullptr;
		}
	}

	// Forget about our grid
	RainGrid.Empty();

	bIsRaining = false;
}

void ASCPlayerCameraManager::OnRainParticlesLoaded()
{
	if (!bIsRaining)
		return;

	// Apply our template to all rain particles
	UParticleSystem* RainTemplate = RainParticleSystem.Get();
	for (FSCRainGridCell& Cell : RainGrid)
	{
		Cell.RainComponent->SetTemplate(RainTemplate);
	}
}

void ASCPlayerCameraManager::UpdateRain(float DeltaTime)
{
#if !UE_BUILD_SHIPPING
	if (CVarDisableRainUpdates.GetValueOnGameThread())
		return;
#endif // !UE_BUILD_SHIPPING

	// No rain, no update
	if (!bIsRaining)
		return;

	const FVector CurrentLocation = GetCameraLocation();
	const float Dist = (PreviousLocation - CurrentLocation).Size2D();
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const float HalfSize = RainParticleSize * 0.5f;

	// Only update when we move enough
	if (Dist < HalfSize)
		return;

	// Move our component to where the camera is
	for (FSCRainGridCell& Cell : RainGrid)
	{
		// See if it's time to update
		if (RainRayTraceTickDelay > KINDA_SMALL_NUMBER && CurrentTime < Cell.NextTraceTimestamp)
			continue;

		const FVector CellLocation(CurrentLocation + Cell.RelativeOffset);
		Cell.RainComponent->SetWorldLocation(CellLocation);

		// If we move far enough in one frame, reset the particles so they don't lag behind us
		if (Dist > RainResetDistance)
		{
			// NOTE: Calling ResetParticles here isn't good enough, they get stuck in an off state. We have to force them off and back on again next frame.
			Cell.RainComponent->Deactivate();
			Cell.bIsOccluded = false;

			// We have to delay the activation to next frame since this function is called as part of the render update.
			// If we active it now the emitters won't be created and it'll be deactivated on the next frame.
			FTimerDelegate Delegate;
			Delegate.BindLambda([](UParticleSystemComponent* RainComponent)
			{
				if (IsValid(RainComponent))
					RainComponent->Activate(true);
			}, Cell.RainComponent);
			GetWorld()->GetTimerManager().SetTimerForNextTick(Delegate);
		}

		const bool bWasOccluded = Cell.bIsOccluded;

		// Try to keep a random offset so we don't have our ray traces pile up
		if (RainRayTraceTickDelay > KINDA_SMALL_NUMBER)
		{
			Cell.NextTraceTimestamp = CurrentTime + (RainRayTraceTickDelay + (RainRayTraceTickDelay * FMath::FRand()));
		}

		Cell.bIsOccluded = false;

		// Shift our trace towards the player so clipping is much harder to notice
		FVector TraceLocation(CellLocation);
		if (FMath::Abs(Cell.RelativeOffset.X) > KINDA_SMALL_NUMBER)
		{
			TraceLocation.X += FMath::FloatSelect(Cell.RelativeOffset.X, -HalfSize, HalfSize);
		}

		if (FMath::Abs(Cell.RelativeOffset.Y) > KINDA_SMALL_NUMBER)
		{
			TraceLocation.Y += FMath::FloatSelect(Cell.RelativeOffset.Y, -HalfSize, HalfSize);
		}

		// Ray trace
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, TraceLocation, TraceLocation - FVector::UpVector * (RainZOffset + 100.f), ECollisionChannel::ECC_Camera))
		{
			if (FMath::Abs(Hit.Location.Z - CurrentLocation.Z) > RainOcclusionHeight)
			{
				Cell.bIsOccluded = true;

#if !UE_BUILD_SHIPPING
				if (CVarDebugRain.GetValueOnGameThread())
				{
					DrawDebugLine(GetWorld(), CellLocation, Hit.Location, FColor::Red, false, RainRayTraceTickDelay, 0, 1.25f);
					DrawDebugSphere(GetWorld(), Hit.Location, 32.f, 16, FColor::Purple, false, RainRayTraceTickDelay, 1, 1.2f);
				}
#endif // !UE_BUILD_SHIPPING
			}
			// Collision was pretty close to the camera height, so let's see if there's anything actually between us and the camera
			else
			{
				TraceLocation = Hit.ImpactPoint + FVector::UpVector * 1.f;
				if (GetWorld()->LineTraceSingleByChannel(Hit, TraceLocation, CurrentLocation, ECollisionChannel::ECC_Camera))
				{
					Cell.bIsOccluded = true;
				}

#if !UE_BUILD_SHIPPING
				if (CVarDebugRain.GetValueOnGameThread())
				{
					DrawDebugLine(GetWorld(), CellLocation, Cell.bIsOccluded ? Hit.Location : CurrentLocation, Cell.bIsOccluded ? FColor::Red : FColor::Yellow, false, RainRayTraceTickDelay, 0, 1.25f);
					if (Cell.bIsOccluded)
						DrawDebugSphere(GetWorld(), Hit.Location, 32.f, 16, FColor::Purple, false, RainRayTraceTickDelay, 1, 1.2f);
				}
#endif // !UE_BUILD_SHIPPING
			}
		}

#if !UE_BUILD_SHIPPING
		if (CVarDebugRain.GetValueOnGameThread() && !Cell.bIsOccluded)
		{
			DrawDebugLine(GetWorld(), CellLocation, TraceLocation - FVector::UpVector * RainZOffset, FColor::Green, false, RainRayTraceTickDelay, 0, 1.25f);
		}
#endif // !UE_BUILD_SHIPPING

		// Set our visibility
		if (Cell.bIsOccluded != bWasOccluded)
		{
			Cell.RainComponent->SetVisibility(!Cell.bIsOccluded);
		}
	}

	PreviousLocation = CurrentLocation;
}

void ASCPlayerCameraManager::DebugDraw()
{
#if !UE_BUILD_SHIPPING
	if (!bIsRaining)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Red, TEXT("Rain is off"), false);
		return;
	}

	// Helper variables
	const float HalfWidth(RainParticleSize * 0.5f);
	const float HalfHeight(RainZOffset * 0.5f);
	const FVector ParticleBoxSize(HalfWidth - 2.f, HalfWidth - 2.f, RainZOffset); // Scaling down a tad so you can see each box
	const FVector BoxCenter(PreviousLocation - (FVector::UpVector * HalfHeight));
	//const float CurrentTime = GetWorld()->GetTimeSeconds();

	if (CVarDisableRainUpdates.GetValueOnGameThread() > 0)
	{
		DrawDebugSphere(GetWorld(), PreviousLocation, 16.f, 16, FColor::Cyan);
	}

	// NOTE: Text is disabled since at large grid sizes (7+), it's hard to see what's going on
	// Print all that chit!
	//GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Cyan, TEXT("Raining Debug"), false);
	for (int32 iRow(0); iRow < RainGridSize; ++iRow)
	{
		//GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Cyan, FString::Printf(TEXT("Row %d"), iRow + 1), false);
		for (int32 iCol(0); iCol < RainGridSize; ++iCol)
		{
			const int32 Index = iRow * RainGridSize + iCol;
			const FSCRainGridCell& Cell = RainGrid[Index];

			DrawDebugBox(GetWorld(), BoxCenter + Cell.RelativeOffset, ParticleBoxSize, Cell.bIsOccluded ? FColor::Red : FColor::Green, false, 0.f, 0, 0.5f);

			// Print out our cell info:
			// Col)	RelativeOffset				TimeSinceTrace	IsOccluded?	(ParticlesOn?)
			// 2)	(-1000.0, 1000.0, 500.0)	0.11			Visible		(On)
			/*GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, Cell.bIsOccluded ? FColor::Red : FColor::Cyan, FString::Printf(TEXT("%d) (%.1f, %.1f, %.1f) %.1f %s (%s)  "),
				iCol + 1,
				Cell.RelativeOffset.X, Cell.RelativeOffset.Y, Cell.RelativeOffset.Z,
				Cell.NextTraceTimestamp - CurrentTime,
				Cell.bIsOccluded ? TEXT("Occluded") : TEXT("Visible"),
				Cell.RainComponent->bIsActive && Cell.RainComponent->IsVisible() ? TEXT("On") : TEXT("Off")
			), false);*/
		}

	}
#endif // !UE_BUILD_SHIPPING
}
