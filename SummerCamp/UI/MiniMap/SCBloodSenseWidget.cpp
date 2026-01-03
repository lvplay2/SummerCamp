// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCBloodSenseWidget.h"
#include "Image.h"
#include "SCKillerCharacter.h"

USCBloodSenseWidget::USCBloodSenseWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, DefaultRadius(16000.f)
, NextPingTimer(-1.f)
, TotalPingTime(1.f)
, PingMinMaxDelay(0.5f, 2.f)
{
}

bool USCBloodSenseWidget::UpdateMapWidget_Implementation(FVector& currentPosition, FRotator& currentRotation)
{
	AActor* OwnerActor = Cast<AActor>(Owner);
	if (!OwnerActor)
	{
		return true;
	}
	if (!bIsActive && OwnerActor->GetVelocity().Size() > 0)
	{
		NextPingTimer -= GetWorld()->DeltaTimeSeconds;
		if (NextPingTimer <= 0)
		{
			BeginPing();
		}
	}
	else if (bIsActive)
	{
		currentPosition += Offset;
		CurrentPingTime -= GetWorld()->DeltaTimeSeconds;
		Blip->SetOpacity(1 - FMath::Abs(CurrentPingTime) / (TotalPingTime * 0.5f));
		if (CurrentPingTime < -(TotalPingTime * 0.5f))
		{
			EndPing();
		}
	}
	return true;
}

void USCBloodSenseWidget::BeginPing()
{
	//ASCCharacter* Character = Cast<ASCCharacter>(Owner);
	//if (!Character)
	//{
	//	return;
	//}

	//// Get radius modifier from owner and the killer.
	//float radiusMod = Character->GetBloodsenseMod();

	//for (TActorIterator<ASCKillerCharacter> It(GetWorld(), ASCKillerCharacter::StaticClass()); It; ++It)
	//{
	//	radiusMod *= It->GetBloodsenseMod();
	//}

	//// Find new radius, and get a random value between 0 and it for our offset.
	//float maxDist = radiusMod * DefaultRadius;
	//maxDist = FMath::FRandRange(0, maxDist);

	//// Randomly rotate the offset.
	//Offset = FVector::ForwardVector * maxDist;
	//Offset = Offset.RotateAngleAxis(FMath::RandRange(0, 360), FVector::UpVector);

	//CurrentPingTime = TotalPingTime * 0.5f;
	//bIsActive = true;
}

void USCBloodSenseWidget::EndPing()
{
	/*ASCCharacter* Character = Cast<ASCCharacter>(Owner);
	if (!Character)
	{
		return;
	}

	Blip->SetOpacity(0);
	NextPingTimer = FMath::FRandRange(PingMinMaxDelay.X, PingMinMaxDelay.Y) * Character->GetBloodsenseFrequencyMod();
	bIsActive = false;*/
}
