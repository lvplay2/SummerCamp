// Copyright (C) 2015-2017 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLAIWaypoint.h"

#include "ILLAIController.h"
#include "ILLCharacter.h"

TAutoConsoleVariable<int32> CVarShowWaypointLinks(TEXT("ill.ShowWaypointArrows"), 1,
	TEXT("Displays arrows showing the links between waypoints.\n")
	TEXT(" 0: Off\n")
	TEXT(" 1: On\n"));

AILLAIWaypoint::AILLAIWaypoint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, LinkColor(FColor::Green)
	, RotationRate(1.f)
	, MoveRate(1.f)
	, MoveToArrowTimeout(5.f)
{
#if WITH_EDITOR
	PrimaryActorTick.bCanEverTick = true;
#endif

	DirectionToFace = CreateDefaultSubobject<UArrowComponent>(TEXT("DirectionToFace"));
	DirectionToFace->SetupAttachment(RootComponent);

	WaypointTextComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DebugText"));
	WaypointTextComponent->SetupAttachment(RootComponent);
}

void AILLAIWaypoint::PostLoad()
{
	Super::PostLoad();

	UpdateWaypointText();

}

void AILLAIWaypoint::PostActorCreated()
{
	Super::PostActorCreated();

	UpdateWaypointText();
}

#if WITH_EDITOR
void AILLAIWaypoint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	UpdateWaypointText();
}

void AILLAIWaypoint::BeginPlay()
{
	Super::BeginPlay();

	// Stop showing links between waypoints while playing
	if (!bDrawLinkArrowDuringPlay)
		SetActorTickEnabled(false);
}

void AILLAIWaypoint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// Allow waypoint links to show again
	SetActorTickEnabled(true);
}

void AILLAIWaypoint::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CVarShowWaypointLinks.GetValueOnGameThread() > 0)
	{
		if (NextWaypoint)
		{
			// Draw an arrow from this waypoint to it's NextWaypoint
			const FVector ToNextWaypoint = NextWaypoint->GetActorLocation() - GetActorLocation();
			const float Distance = ToNextWaypoint.Size();
			const FVector Direction = ToNextWaypoint / Distance;
			DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), NextWaypoint->GetActorLocation() + Direction * -10.f, 20.f, LinkColor, false, 0.f, 0, 1.f);
		}
	}
}
#endif

void AILLAIWaypoint::UpdateWaypointText()
{
	if (WaypointTextComponent->Text.IsEmpty() || WaypointTextComponent->Text.EqualTo(FText::FromString(TEXT("Text"))))
		WaypointTextComponent->SetText(BehaviorToExecute ? FText::FromString(TEXT("BT")) : FText::FromString(TEXT("")));
	WaypointTextComponent->bHiddenInGame = true;
	WaypointTextComponent->SetVisibility(BehaviorToExecute ? true : false);
	WaypointTextComponent->SetTextRenderColor(LinkColor);
	WaypointTextComponent->SetRelativeRotation(GetActorRotation());
}

FTransform AILLAIWaypoint::GetArrowTransform() const
{
	return DirectionToFace->GetComponentTransform();
}

bool FSynchronizedWaypointInfo::IsSynchronized() const
{
	if (SynchronizedWayoint && RequiredCharacter && SynchronizedWayoint->IsActorAtWaypoint(RequiredCharacter))
	{
		if (AILLAIController* AIController = Cast<AILLAIController>(RequiredCharacter->Controller))
			return AIController->bCanExecuteSynchronizedWaypointActions;
	}

	return false;
}
