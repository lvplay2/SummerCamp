// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPowerGridVolume.h"

#include "SCCounselorCharacter.h"
#include "SCGameState.h"
#include "SCPowerPlant.h"

// This is the max number of objects we'll turn on/off at a time
// Decreasing this number will result in a better frame rate, but a longer completion time of the power cycle
// The overall frame rate will still be lower than normal gameplay while power cycling the objects
#define BLACKOUT_WAVE_COUNT 10

ASCPowerGridVolume::ASCPowerGridVolume(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, CurrentBlackoutPosition(INDEX_NONE)
{
	bReplicates = true;

	PoweredArea = ObjectInitializer.CreateDefaultSubobject<UBoxComponent>(this, TEXT("MarkupArea"));
	RootComponent = PoweredArea;

	PoweredArea->SetCollisionObjectType(ECC_PowerdLight);
	PoweredArea->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PoweredArea->SetCollisionResponseToAllChannels(ECR_Ignore);
	PoweredArea->SetCollisionResponseToChannel(ECC_PowerdLight, ECR_Overlap);
	PoweredArea->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PoweredArea->bHiddenInGame = true;
	PoweredArea->ShapeColor = FColor(51, 51, 204);

	static ConstructorHelpers::FClassFinder<USCFearEventData> DefaultBlackoutFearEvent(TEXT("Blueprint'/Game/Blueprints/FearEvents/BlackoutFearEvent.BlackoutFearEvent_C'"));
	BlackOutFearEvent = DefaultBlackoutFearEvent.Class;

	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.bCanEverTick = true;
}

void ASCPowerGridVolume::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Handle overlaps
	if (!PoweredArea->OnComponentBeginOverlap.IsAlreadyBound(this, &ASCPowerGridVolume::OnBeginOverlap))
	{
		PoweredArea->OnComponentBeginOverlap.AddDynamic(this, &ASCPowerGridVolume::OnBeginOverlap);
		PoweredArea->OnComponentEndOverlap.AddDynamic(this, &ASCPowerGridVolume::OnEndOverlap);
	}
}

void ASCPowerGridVolume::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (ASCGameState* SCGameState = Cast<ASCGameState>(World->GetGameState()))
		{
			// Sign up!
			SCGameState->GetPowerPlant()->Register(this);

			// TODO: Let's also grab counselors while we're here!
		}
	}

	// Find the longest power on curve (wasn't any when implemented)
	for (const UCurveFloat* Curve : PowerOnCurves)
	{
		float Min(FLT_MAX), Max(-FLT_MAX);
		Curve->GetTimeRange(Min, Max);
		LongestPowerOnCurve = FMath::Max(LongestPowerOnCurve, Max);
	}

	// Find the longest power off curve (was ~1.25 seconds when implemented)
	for (const UCurveFloat* Curve : PowerOffCurves)
	{
		float Min(FLT_MAX), Max(-FLT_MAX);
		Curve->GetTimeRange(Min, Max);
		LongestPowerOffCurve = FMath::Max(LongestPowerOffCurve, Max);
	}

	// Gives us a lower bounds and a little breathing room between cycles
	LongestPowerOnCurve += 0.1f;
	LongestPowerOffCurve += 0.1f;
}

void ASCPowerGridVolume::TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);

	// Not currently cycling objects (tick is generally turned off in this case)
	if (CurrentBlackoutPosition == INDEX_NONE)
		return;

	// Make sure it's time to start the next cycle
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime >= NextBlackoutWave_Timestamp)
	{
		// Loop through the next set of objects
		const bool bLightsOn = !bDeactivated;
		int32 MaxObject = FMath::Min(CurrentBlackoutPosition + MaxBlackoutObjectsPerWave, PoweredObjects.Num());
		for (int32 i(CurrentBlackoutPosition); i < MaxObject; ++i)
		{
			UObject* Object = PoweredObjects[i];

			// Ran into a couple of crashes here, just removing these objects to improve future loops
			if (!IsValid(Object))
			{
				PoweredObjects.Remove(Object);
				--i;
				--MaxObject;
				continue;
			}

			// Toggle that power!
			if (ISCPoweredObject* PoweredInterface = Cast<ISCPoweredObject>(Object))
			{
				PoweredInterface->SetPowered(bLightsOn, bSkipFlicker ? nullptr : GetRandomCurve(bLightsOn));
			}
			else if (Object->Implements<USCPoweredObject>())
			{
				ISCPoweredObject::Execute_OnSetPowered(Object, bLightsOn, bSkipFlicker ? nullptr : GetRandomCurve(bLightsOn));
			}
		}

		// See if we've gone through all the objects and turn off our update
		if (MaxObject == PoweredObjects.Num())
		{
			CurrentBlackoutPosition = INDEX_NONE;
			SetActorTickEnabled(false);
		}
		// More to go!
		else
		{
			CurrentBlackoutPosition = MaxObject;
			NextBlackoutWave_Timestamp = CurrentTime + (bLightsOn ? LongestPowerOnCurve : LongestPowerOffCurve);
		}
	}
}

void ASCPowerGridVolume::AddPoweredObject(UObject* PoweredObject)
{
	PoweredObjects.Add(PoweredObject);
}

void ASCPowerGridVolume::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Add actors
	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(OtherActor))
	{
		Counselors.AddUnique(Counselor);
	}
}

void ASCPowerGridVolume::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// Remove actors
	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(OtherActor))
	{
		Counselors.Remove(Counselor);
	}
}

void ASCPowerGridVolume::RequestDeactivateLights(const bool bDeactivateLights, const bool bShouldSkipFlicker /*= false*/)
{
	if (Role < ROLE_Authority)
	{
		SERVER_DeactivateLights(bDeactivateLights, bShouldSkipFlicker);
	}
	else
	{
		MULTICAST_DeactivateLights(bDeactivateLights, bShouldSkipFlicker);
	}
}

bool ASCPowerGridVolume::SERVER_DeactivateLights_Validate(const bool bDeactivateLights, const bool bShouldSkipFlicker /*= false*/)
{
	return true;
}

void ASCPowerGridVolume::SERVER_DeactivateLights_Implementation(const bool bDeactivateLights, const bool bShouldSkipFlicker /*= false*/)
{
	RequestDeactivateLights(bDeactivateLights, bShouldSkipFlicker);
}

void ASCPowerGridVolume::MULTICAST_DeactivateLights_Implementation(const bool bDeactivateLights, const bool bShouldSkipFlicker /*= false*/)
{
	DeactivateLights(bDeactivateLights, bShouldSkipFlicker);
}

void ASCPowerGridVolume::DeactivateLights(const bool bDeactivateLights, const bool bShouldSkipFlicker /*= false*/)
{
	if (bDeactivated == bDeactivateLights)
		return;

	const bool bLightsOn = !bDeactivateLights;

	// Scary fear events!
	for (ASCCounselorCharacter* Counselor : Counselors)
	{
		if (IsValid(Counselor))
		{
			if (bDeactivateLights)
			{
				Counselor->PushFearEvent(BlackOutFearEvent);
				Counselor->PowerCutSFXVO();
			}
			else
			{
				Counselor->PopFearEvent(BlackOutFearEvent);
			}

			// TODO: Call a random emote here and start a dance party!
		}
	}

	// Prepare for our rolling blackout next frame
	bSkipFlicker = bShouldSkipFlicker;
	bDeactivated = bDeactivateLights;
	MaxBlackoutObjectsPerWave = FMath::Max(PoweredObjects.Num() / BLACKOUT_WAVE_COUNT, BLACKOUT_WAVE_COUNT);
	CurrentBlackoutPosition = 0;
	NextBlackoutWave_Timestamp = GetWorld()->GetTimeSeconds();
	SetActorTickEnabled(true);

	OnPowerChanged.Broadcast(!IsDeactivated());
}

UCurveFloat* ASCPowerGridVolume::GetRandomCurve(const bool bPowerOn) const
{
	if (bPowerOn)
	{
		if (PowerOnCurves.Num() > 0)
			return PowerOnCurves[FMath::RandHelper(PowerOnCurves.Num())];
	}
	else
	{
		if (PowerOffCurves.Num() > 0)
			return PowerOffCurves[FMath::RandHelper(PowerOffCurves.Num())];
	}

	// No curves? Rude.
	return nullptr;
}
