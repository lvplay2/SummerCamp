// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCEscapeVolume.h"

#include "ILLPlayerController.h"

#include "SCCameraSplineComponent.h"
#include "SCCounselorCharacter.h"
#include "SCDriveableVehicle.h"
#include "SCGameMode_Hunt.h"
#include "SCKillerCharacter.h"
#include "SCMinimapIconComponent.h"
#include "SCSplineCamera.h"
#include "SCVehicleSeatComponent.h"

// Sets default values
ASCEscapeVolume::ASCEscapeVolume(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CameraLerpTime(6.f)
	, EscapeType(ESCEscapeType::Police)
	, EscapeDelay(5.f)
{
	bReplicates = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("TriggerRoot"));

	EscapeVolume = CreateDefaultSubobject<USphereComponent>(TEXT("EscapeVolume"));
	EscapeVolume->SetCollisionProfileName(TEXT("EscapeTrigger"));
	EscapeVolume->SetCollisionObjectType(ECC_Escape);
	EscapeVolume->SetupAttachment(RootComponent);

	ExitPath = CreateDefaultSubobject<USplineComponent>(TEXT("ExitPath"));
	ExitPath->SetupAttachment(RootComponent);

	EscapeCamSpline = CreateDefaultSubobject<USCCameraSplineComponent>(TEXT("EscapeCameraSpline"));
	EscapeCamSpline->SetupAttachment(RootComponent);

	EscapeCam = CreateDefaultSubobject<USCSplineCamera>(TEXT("EscapeCamera"));
	EscapeCam->SetupAttachment(EscapeCamSpline);
	EscapeCam->bAutoActivate = true;

	SpectatorStartPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpectatorStartPoint"));
	SpectatorStartPoint->SetupAttachment(RootComponent);

	MinimapIconComponent = CreateDefaultSubobject<USCMinimapIconComponent>(TEXT("MinimapIconComponent"));
	MinimapIconComponent->SetupAttachment(RootComponent);

	PrimaryActorTick.bStartWithTickEnabled = false;
	bIgnoreExitSpline = false;
}

void ASCEscapeVolume::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	EscapeVolume->OnComponentBeginOverlap.AddUniqueDynamic(this, &ASCEscapeVolume::OnOverlap);
}

void ASCEscapeVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_SetDriverCamera);
	}

	Super::EndPlay(EndPlayReason);
}

void ASCEscapeVolume::MULTICAST_ActivateVolume_Implementation()
{
	EscapeVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void ASCEscapeVolume::DeactivateVolume_Implementation()
{
	EscapeVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

bool ASCEscapeVolume::CanEscapeHere() const
{
	return EscapeVolume->GetCollisionEnabled() != ECollisionEnabled::NoCollision;
}

bool ASCEscapeVolume::IsUnobstructed()
{
	TArray<FOverlapResult> OutOverlaps;
	FVector Start = EscapeVolume->GetComponentLocation();

	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);

	// Create the sphere and get the overlapping actors
	if (GetWorld()->OverlapMultiByChannel(OutOverlaps, Start, FQuat::Identity, ECollisionChannel::ECC_Pawn, EscapeVolume->GetCollisionShape(), CollisionParams))
	{
		for (FOverlapResult overlap : OutOverlaps)
		{
			if (overlap.GetActor()->IsA(ASCKillerCharacter::StaticClass()))
			{
				return false;
			}
		}
	}

	return true;
}

void ASCEscapeVolume::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Role == ROLE_Authority)
	{
		if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(OtherActor))
		{
			if (Killer->IsShiftActive())
				Killer->CancelShift();

			// Early out here to avoid doing the math for the spline stuff
			return;
		}

		if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(OtherActor))
		{
			// Already escaping or being murdered or grabbed, fuck off
			if (Counselor->IsEscaping() || Counselor->IsInContextKill() || Counselor->bIsGrabbed || !Counselor->IsAlive())
				return;
		}
	}

	// Recreate the spline based on the OtherActor's current position
	const int32 TotalSplinePoints = ExitPath->GetNumberOfSplinePoints();
	USplineComponent* NewExitPath = nullptr;
	if (TotalSplinePoints > 0 && !bIgnoreExitSpline)
	{
		float ShortestDist = (OtherActor->GetActorLocation() - ExitPath->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World)).SizeSquared2D();
		int32 ShortestIndex = 0;

		for (int32 iSplinePoint(0); iSplinePoint < TotalSplinePoints; ++iSplinePoint)
		{
			const float CurrentDist = (OtherActor->GetActorLocation() - ExitPath->GetLocationAtSplinePoint(iSplinePoint, ESplineCoordinateSpace::World)).SizeSquared2D();
			if (CurrentDist < ShortestDist)
			{
				ShortestDist = CurrentDist;
				ShortestIndex = iSplinePoint;
			}
		}

		TArray<FVector> NewSplinePoints;
		NewSplinePoints.Add(OtherActor->GetActorLocation());
		for (int32 iSplinePoint(ShortestIndex); iSplinePoint < TotalSplinePoints; ++iSplinePoint)
			NewSplinePoints.Add(ExitPath->GetLocationAtSplinePoint(iSplinePoint, ESplineCoordinateSpace::World));

		NewExitPath = NewObject<USplineComponent>(GetOuter());
		NewExitPath->ClearSplinePoints();
		for (int32 iSplinePoint(0); iSplinePoint < NewSplinePoints.Num(); ++iSplinePoint)
			NewExitPath->AddSplinePoint(NewSplinePoints[iSplinePoint], ESplineCoordinateSpace::World);

		NewExitPath->UpdateSpline();
	}

	if(EscapeType == ESCEscapeType::Car || EscapeType == ESCEscapeType::Boat)
	{
		if (OtherComp->IsA<USkeletalMeshComponent>())
		{
			if (ASCDriveableVehicle* DriveableVehicle = Cast<ASCDriveableVehicle>(OtherActor))
			{
				// Already escaping, fuck off
				if (DriveableVehicle->bEscaping)
					return;

				for (auto Seat : DriveableVehicle->Seats)
				{
					if (Seat->CounselorInSeat && Seat->CounselorInSeat->IsLocallyControlled())
					{
						if (Seat->IsDriverSeat())
						{
							FTimerDelegate Delegate;
							Delegate.BindLambda([this](ASCCounselorCharacter* Driver)
							{
								SetupEscapeCamera(Driver);
							}, Seat->CounselorInSeat);
							GetWorldTimerManager().SetTimer(TimerHandle_SetDriverCamera, Delegate, 0.75f, false);
						}
						else
						{
							SetupEscapeCamera(Seat->CounselorInSeat);
						}
					}
				}

				float Delay = bUseSplineCameraDurationAsDelay ? EscapeCamSpline->Duration : EscapeDelay;
				DriveableVehicle->SetEscaped(this, NewExitPath, Delay);
			}
		}
	}
	else
	{
		if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(OtherActor))
		{
			// Already escaping or being murdered or grabbed, fuck off
			if (Counselor->IsEscaping() || Counselor->IsInContextKill() || Counselor->bIsGrabbed || !Counselor->IsAlive())
				return;

			float Delay = bUseSplineCameraDurationAsDelay ? EscapeCamSpline->Duration : EscapeDelay;
			Counselor->SetEscaped(NewExitPath, Delay);

			if (Counselor->IsLocallyControlled())
				SetupEscapeCamera(Counselor);
		}
	}
}

void ASCEscapeVolume::SetupEscapeCamera(ASCCounselorCharacter* Counselor)
{
	if (AController* Controller = Counselor->GetCharacterController())
		Controller->SetControlRotation(EscapeCam->GetComponentRotation());

	Counselor->TakeCameraControl(this, CameraLerpTime);
	EscapeCam->ActivateCamera();
}
