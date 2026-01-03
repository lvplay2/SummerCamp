// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLWidgetComponent.h"

#include "ILLGameBlueprintLibrary.h"
#include "ILLPlayerController.h"
#include "ILLUserWidget.h"

UILLWidgetComponent::UILLWidgetComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	PrimaryComponentTick.bAllowTickOnDedicatedServer = false;
	bCanEverAffectNavigation = false;

	DestructionTimer = 2.0f;
	DestroyTimer = 0.0f;
}

void UILLWidgetComponent::OnRegister()
{
	Super::OnRegister();

	if (bUseDistanceNotify)
	{
		PrimaryComponentTick.bCanEverTick = true;

		RemoveWidgetFromScreen();
		Widget = nullptr;
	}
}

void UILLWidgetComponent::InitWidget()
{
	Super::InitWidget();

	// Register the owning actor as the ActorContext
	if (UILLUserWidget* ILLWidget = Cast<UILLUserWidget>(Widget))
	{
		ILLWidget->ComponentContext = this;
		ILLWidget->ActorContext = GetOwner();
	}
}

void UILLWidgetComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UWorld* World = GetWorld();
	APawn* LocalPawn = nullptr;
	if (AILLPlayerController* PC = World ? UILLGameBlueprintLibrary::GetFirstLocalPlayerController(World) : nullptr)
	{
		LocalPawn = PC->GetPawn();
	}

	// Check if we should cull when the game state is not in-progress
	bool bIsNear = false;
	if (bOutOfRangeWhenMatchNotInProgress)
	{
		if (AGameState* GameState = Cast<AGameState>(World->GetGameState()))
		{
			bIsNear = GameState->IsMatchInProgress();
		}
	}
	else
	{
		bIsNear = true;
	}

	// Check distance if the above checks pass
	if (bIsNear && DistanceToNotify > 0.0f)
	{
		bIsNear = false;
		if (LocalPawn)
		{
			const FVector PlayerLoc = LocalPawn->GetActorLocation();
			const FVector IconLoc = GetComponentLocation();
			const float DistanceSq = (PlayerLoc - IconLoc).SizeSquared();
			bIsNear = DistanceSq < FMath::Square(DistanceToNotify);
		}
	}

	// Now see if our state is different
	if (bIsNear != bLastWasNear)
	{
		if (bIsNear)
		{
			if (DestroyTimer > 0.0f)
				DestroyTimer = 0.0f;
			else
				InitWidget();

			OnLocalCharacterInRange.Broadcast();

			if (UILLUserWidget* ILLWidget = Cast<UILLUserWidget>(Widget))
				ILLWidget->OnLocalCharacterInRange();
		}
		else
		{
			OnLocalCharacterOutOfRange.Broadcast();

			if (UILLUserWidget* ILLWidget = Cast<UILLUserWidget>(Widget))
				ILLWidget->OnLocalCharacterOutOfRange();

			// set up timer to destroy widget if destruction time is greater than 0
			if (DestructionTimer > 0.0f)
				DestroyTimer = DestructionTimer;
		}

		bLastWasNear = bIsNear;
	}

	if (DestroyTimer > 0.0f)
	{
		DestroyTimer -= DeltaTime;
		if (DestroyTimer <= 0.0f)
		{
			if (GetWorld())
				RemoveWidgetFromScreen();
			Widget = nullptr;
			DestroyTimer = 0.0f;
		}
	}
}
