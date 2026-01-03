// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCZeus.h"

#include "SCGameState.h"
#include "SCWorldSettings.h"

#include "Kismet/KismetMaterialLibrary.h"

#if !UE_BUILD_SHIPPING
extern TAutoConsoleVariable<int32> CVarDebugRain;
#endif // !UE_BUILD_SHIPPING

static const FName NAME_LightiningParam(TEXT("LightiningIntensity"));

ASCZeus::ASCZeus(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, LightningSoundThreshold(3.f)
, InitialDelay(2)
, MinTimeBetweenLightning(8)
, MaxTimeBetweenLightning(25)
, LastUsedTiming(INDEX_NONE)
, ActiveCurve(INDEX_NONE)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	bReplicates = true;
	bAlwaysRelevant = true;

	DirectionalLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("DirectionalLight"));
	RootComponent = DirectionalLight;

	ThunderAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("ThunderAudio"));
	ThunderAudio->bAllowSpatialization = false;
	ThunderAudio->bAutoActivate = false;
	ThunderAudio->SetupAttachment(RootComponent);

	RainAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("RainAudio"));
	RainAudio->bAllowSpatialization = false;
	RainAudio->bAutoActivate = false;
	RainAudio->SetupAttachment(RootComponent);
}

void ASCZeus::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCZeus, LightningTimings);
}

void ASCZeus::BeginPlay()
{
	Super::BeginPlay();

	DefaultIntensity = DirectionalLight->Intensity;

	// Generate random lightning strikes on the server
	const int32 LightningCount = LightningCurves.Num();
	if (Role == ROLE_Authority && LightningCount > 0)
	{
		ASCGameState* GameState = Cast<ASCGameState>(GetWorld()->GetGameState());

		const int32 TotalTime = FMath::Max(GameState->RemainingTime + 60, 300); // Generate at least 5 mins worth
		GenerateLightning(InitialDelay, TotalTime);
	}

	// Figure out where we are in the curve list if we're a late joiner and the match has already started
	ASCGameState* GameState = Cast<ASCGameState>(GetWorld()->GetGameState());
	if (GameState && GameState->HasMatchStarted())
	{
		const int32 CurrentGameTime = GameState->ElapsedInProgressTime;
		for (int32 iTimingIndex(0); iTimingIndex < LightningTimings.Num(); ++iTimingIndex)
		{
			FSCLightingTiming& Timing = LightningTimings[iTimingIndex];
			if (CurrentGameTime < Timing.TimeSeconds)
			{
				// This strike hasn't happened yet, we're on the one before it
				LastUsedTiming = iTimingIndex - 1;
				break;
			}
		}
	}

	RainAudio->Play();

	SetActorTickEnabled(true);
}

void ASCZeus::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Need a world, need a light
	UWorld* World = GetWorld();
	if (!World || LightningTimings.Num() == 0)
		return;

	// We don't have an active curve, look for one
	if (ActiveCurve == INDEX_NONE)
	{
		ASCGameState* GameState = Cast<ASCGameState>(World->GetGameState());
		const int32 CurrentGameTime = GameState->ElapsedInProgressTime;
		if (CurrentGameTime != PreviousGameTime)
		{
			// If we've run out of lightining strikes, generate another minute of them
			if (LastUsedTiming >= LightningTimings.Num() - 1 && Role == ROLE_Authority)
			{
				const int32 StartTime = LightningTimings[LightningTimings.Num() - 1].TimeSeconds;
				GenerateLightning(StartTime, 60);
			}

			for (int32 iTimingIndex(LastUsedTiming + 1); iTimingIndex < LightningTimings.Num(); ++iTimingIndex)
			{
				FSCLightingTiming& Timing = LightningTimings[iTimingIndex];
				if (CurrentGameTime < Timing.TimeSeconds)
					break;

				LastUsedTiming = iTimingIndex;
				LightingCurveStartTime = World->GetTimeSeconds();
				PreviousIntensityScale = 1.f;
				ActiveCurve = Timing.CurveIndex;
				bThunderPlayed = false;
				break;
			}

			PreviousGameTime = CurrentGameTime;
		}
	}

	// We have an active curve, update it
	if (ActiveCurve != INDEX_NONE)
	{
		// Grab our curve (probably!)
		UCurveFloat* CurrentCurve = LightningCurves[ActiveCurve];
		if (!CurrentCurve)
		{
			ActiveCurve = INDEX_NONE;
			DirectionalLight->SetIntensity(DefaultIntensity);
			if (UMaterialParameterCollection* RainMPC = GetRainMaterialParameterCollection())
			{
				UKismetMaterialLibrary::SetScalarParameterValue(this, RainMPC, NAME_LightiningParam, 0.f);
			}
			return;
		}

		// Find our full range
		const float CurrentTime = World->GetTimeSeconds();
		float MinTime(0.f), MaxTime(0.f);
		CurrentCurve->GetTimeRange(MinTime, MaxTime);
		MaxTime += LightingCurveStartTime;

		// We've overstayed our welcome, revert to default
		if (CurrentTime >= MaxTime)
		{
			ActiveCurve = INDEX_NONE;
			DirectionalLight->SetIntensity(DefaultIntensity);
			if (UMaterialParameterCollection* RainMPC = GetRainMaterialParameterCollection())
			{
				UKismetMaterialLibrary::SetScalarParameterValue(this, RainMPC, NAME_LightiningParam, 0.f);
			}
			return;
		}

		// Brighten that light up!
		const float NewIntensityScale = CurrentCurve->GetFloatValue(CurrentTime - LightingCurveStartTime);
		DirectionalLight->SetIntensity(DefaultIntensity * NewIntensityScale);

		// Scale up our sky too!
		if (UMaterialParameterCollection* RainMPC = GetRainMaterialParameterCollection())
		{
			float MinValue(1.f), MaxValue(1.f);
			CurrentCurve->GetValueRange(MinValue, MaxValue);
			const float ScaleDelta = FMath::Clamp((NewIntensityScale - MinValue) / FMath::Max(MaxValue, KINDA_SMALL_NUMBER), 0.f, 1.f);

			UKismetMaterialLibrary::SetScalarParameterValue(this, RainMPC, NAME_LightiningParam, ScaleDelta);
		}

		// Pow!
		if (!bThunderPlayed && PreviousIntensityScale <= LightningSoundThreshold && NewIntensityScale > LightningSoundThreshold)
		{
			bThunderPlayed = true;
			ThunderAudio->Play();
		}

		PreviousIntensityScale = NewIntensityScale;
	}

#if !UE_BUILD_SHIPPING
	if (CVarDebugRain.GetValueOnGameThread())
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Cyan, FString::Printf(TEXT("Lightning Queue (Total Strikes: %d, Current Time: %d)"), LightningTimings.Num(), Cast<ASCGameState>(World->GetGameState())->ElapsedInProgressTime), false);

		// Don't print a bunch of text twice on clients in the editor
		if (!GEngine->IsEditor() || Role == ROLE_Authority)
		{
			for (int32 iTimingIndex(0); iTimingIndex < LightningTimings.Num(); ++iTimingIndex)
			{
				FColor Color(FColor::Cyan);
				if (iTimingIndex <= LastUsedTiming)
					Color = FColor::Red;

				// Current strike
				if (iTimingIndex == LastUsedTiming && ActiveCurve != INDEX_NONE)
					Color = FColor::Emerald;

				const FSCLightingTiming& Timing = LightningTimings[iTimingIndex];
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, Color, FString::Printf(TEXT("%d. Curve %d @ %d seconds"), iTimingIndex + 1, Timing.CurveIndex, Timing.TimeSeconds), false);
			}
		}
	}
#endif //!UE_BUILD_SHIPPING
}

void ASCZeus::GenerateLightning(const int32 StartTime, const int32 SecondsToFill)
{
	const int32 UpperLimit = StartTime + SecondsToFill;
	const int32 LightningCount = LightningCurves.Num();
	int32 MinTime = StartTime;

	// This will generate a list of timestamps and curves to use throughout the match
	do
	{
		const uint8 Index = FMath::RandRange(0, LightningCount - 1);
		const int32 Time = FMath::RandRange(MinTime, MinTime + MaxTimeBetweenLightning);

		MinTime = Time + MinTimeBetweenLightning;
		LightningTimings.Add(FSCLightingTiming({Time, Index}));
	} while(MinTime < UpperLimit);
}

UMaterialParameterCollection* ASCZeus::GetRainMaterialParameterCollection() const
{
	if (ASCGameState* GS = GetWorld() ? GetWorld()->GetGameState<ASCGameState>() : nullptr)
	{
		if (ASCWorldSettings* WorldSettings = Cast<ASCWorldSettings>(GS->GetWorldSettings()))
		{
			return WorldSettings->RainMaterialParameterColleciton;
		}
	}

	return nullptr;
}
