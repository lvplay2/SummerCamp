// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCSplineCamera.h"
#include "SCCharacter.h"

#if WITH_EDITOR
# include "LevelEditorViewport.h"
#endif

#include "SCTypes.h"

USCSplineCamera::USCSplineCamera(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ViewportPreview(false)
	, PreviewTime(KINDA_SMALL_NUMBER)
	, ValidityTraceFidelity(0.06f)
	, CameraMaxTraceDistance(120.f)
	, ActiveFocalComponent(nullptr)
	, SplineDuration(1.f)
	, ActiveTime(0.f)
	, BlendOutPreTime(0.f)
	, bReturnCameraOnEnd(false)
	, ReturnCameraTo(nullptr)
	, BlendBackType(VTBlend_EaseInOut)
	, HandicamShakeFrequency(0.f, 0.f, 0.f)
	, HandicamShakeFrequenceRandomizer(0.f)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void USCSplineCamera::PostInitProperties()
{
	Super::PostInitProperties();
	bAutoActivate = false;
}

void USCSplineCamera::BeginPlay()
{
	Super::BeginPlay();

	InitCameraSpline();
	SetComponentTickEnabled(false);

	CachedDefaultTransform = FSCCameraSplinePoint(0, RelativeLocation, ESplinePointType::Linear, RelativeRotation);
	CachedDefaultTransform.Scale = DefaultHandicamStrength;
	CachedDefaultTransform.FOV = FieldOfView;
}

void USCSplineCamera::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!CameraSpline && !bAlwaysApplyHandicam)
	{
		SetComponentTickEnabled(false);
		return;
	}

	FSCCameraSplinePoint LocalPoint = CachedDefaultTransform;
	float Duration = 1.0f;

	// if we have a spline to follow determine our point on it first.
	if (CameraSpline)
	{
		/** Clamp our active time between 0 and the camera splines duration.
		 * There is a buffer from both ends (KINDA_SMALL_NUMBER) to account for floating point and spline issues with
		 * referencing the first and last point exactly.
		 */
		ActiveTime = FMath::Clamp(ActiveTime + (bPingPongForward ? DeltaTime : -DeltaTime), KINDA_SMALL_NUMBER, CameraSpline->Duration - KINDA_SMALL_NUMBER);


		// Get the FSCCameraSplinePoint at the active time along the spline
		LocalPoint = CameraSpline->GetSplinePointAtTime(ActiveTime, ESplineCoordinateSpace::Local);

		// We need to do a trace to determine if we need to push the camera forward
		FCollisionQueryParams SC_TraceParams = FCollisionQueryParams(FName(TEXT("CameraTrace")), true, GetAttachmentRootActor());
		SC_TraceParams.bTraceComplex = false;
		SC_TraceParams.bTraceAsyncScene = true;
		SC_TraceParams.bReturnPhysicalMaterial = false;

		FHitResult CameraHit(ForceInit);
		FSCCameraSplinePoint WorldPoint = CameraSpline->GetSplinePointAtTime(ActiveTime, ESplineCoordinateSpace::World);

		float TraceDistance = CameraMaxTraceDistance;
		if (ActiveFocalComponent)
		{
			TraceDistance = FVector::Dist(ActiveFocalComponent->GetComponentLocation(), WorldPoint.Position);
		}

		// We do this trace in reverse so we know where to place the camera.
		if (GetWorld()->LineTraceSingleByChannel(CameraHit, WorldPoint.Position + WorldPoint.Rotation.Vector() * TraceDistance, WorldPoint.Position, ECC_AnimCameraBlocker, SC_TraceParams))
		{
			// Update the camera's local position to be in front of the found collider.
			float Distance = FVector::Dist(CameraHit.Location, WorldPoint.Position);

			// move the camera to the collision point and out along the normal slightly to prevent FOV penetration
			LocalPoint.Position += LocalPoint.Rotation.Vector() * Distance + (CameraHit.ImpactNormal * 5.f);

			// We also want to open the FOV as we push it forward. Lets clamp it between 30 and 120 for FOV.
			LocalPoint.FOV = FMath::Clamp(LocalPoint.FOV + Distance * (Distance / TraceDistance), 30.f, 120.f);
		}

		Duration = CameraSpline->Duration - KINDA_SMALL_NUMBER;
	}

	// Add in our handicam shake
	FVector ShakeFrequency = HandicamShakeFrequency * (1.f + FMath::FRand() * HandicamShakeFrequenceRandomizer);
	ShakeTime += FVector(DeltaTime + DeltaTime * ShakeFrequency.X, DeltaTime + DeltaTime * ShakeFrequency.Y, DeltaTime + DeltaTime * ShakeFrequency.Z);
	LocalPoint.Rotation.Add(
		LocalPoint.Scale.X * FMath::Sin(FMath::Fmod((ShakeTime.X * ShakeFrequency.X), 2 * PI)),
		LocalPoint.Scale.Y * FMath::Sin(FMath::Fmod((ShakeTime.Y * ShakeFrequency.Y), 2 * PI)),
		LocalPoint.Scale.Z * FMath::Sin(FMath::Fmod((ShakeTime.Z * ShakeFrequency.Z), 2 * PI))
	);

	// Set new location, rotation and FOV
	SetRelativeLocationAndRotation(LocalPoint.Position, LocalPoint.Rotation);
	SetFieldOfView(LocalPoint.FOV);

	if (bReturnCameraOnEnd && LoopMode == ESplineCameraLoopMode::SCL_None)
	{
		Duration -= BlendOutPreTime;
	}

	// If we've reached the end of the spline handle any looping or turn off the tick
	if (ActiveTime >= Duration || ActiveTime <= KINDA_SMALL_NUMBER)
	{
		switch(LoopMode)
		{
		case ESplineCameraLoopMode::SCL_Loop:
			ActiveTime = 0.f;
			break;

		case ESplineCameraLoopMode::SCL_PingPong:
			bPingPongForward = !bPingPongForward;
			break;

		case ESplineCameraLoopMode::SCL_None:
		default:
			SetComponentTickEnabled(false);
			ActiveFocalComponent = nullptr;
			OnCameraAnimationEnded.Broadcast();
			if (ReturnCameraTo != nullptr && bReturnCameraOnEnd)
			{
				ReturnCameraTo->ReturnCameraToCharacter(BlendOutPreTime, BlendBackType, 2.f, false, true);
				ReturnCameraTo = nullptr;
			}
		}
	}
}

void USCSplineCamera::ActivateCamera(USceneComponent* FocalComponent /* = nullptr */, class ASCCharacter* ReturnToCharacter /* = nullptr */)
{
	ActiveFocalComponent = FocalComponent;
	ActiveTime = 0.f;
	ReturnCameraTo = ReturnToCharacter;
	ShakeTime = FVector::ZeroVector;
	if (InitCameraSpline() || bAlwaysApplyHandicam)
	{
		SetComponentTickEnabled(true);
		bPingPongForward = true;
	}
}

void USCSplineCamera::DeactivateCamera()
{
	SetComponentTickEnabled(false);
}

bool USCSplineCamera::IsCameraValid(AActor* TraceIgnoreActor) const
{
	TArray<AActor*> ActorList;

	if (TraceIgnoreActor)
	{
		ActorList.Add(TraceIgnoreActor);
	}

	return IsCameraValidArray(ActorList);
}

bool USCSplineCamera::IsCameraValidArray(TArray<AActor*> TraceIgnoreActors) const
{
	if (!CameraSpline)
		return false;

	FCollisionQueryParams SC_TraceParams = FCollisionQueryParams(FName(TEXT("CameraTrace")), true, GetAttachmentRootActor());
	SC_TraceParams.bTraceComplex = false;
	SC_TraceParams.bTraceAsyncScene = true;
	SC_TraceParams.bReturnPhysicalMaterial = false;

	SC_TraceParams.AddIgnoredActors(TraceIgnoreActors);

	FHitResult CameraHit(ForceInit);

	float TraceTime = 0.f;
	int MaxIterations = FMath::CeilToInt(CameraSpline->Duration / ValidityTraceFidelity) * 2;
	while (TraceTime <= CameraSpline->Duration)
	{
		FSCCameraSplinePoint WorldPoint = CameraSpline->GetSplinePointAtTime(TraceTime, ESplineCoordinateSpace::World);
		GetWorld()->LineTraceSingleByChannel(CameraHit, WorldPoint.Position, WorldPoint.Position + WorldPoint.Rotation.Vector() * CameraMaxTraceDistance, ECC_AnimCameraBlocker, SC_TraceParams);

		if (CameraHit.bBlockingHit || --MaxIterations <= 0)
		{
			// Something is blocking the camera... shit
			return false;
		}

		// if we've reached the end break the loop.
		if (TraceTime >= CameraSpline->Duration)
			break;

		TraceTime = FMath::Clamp(TraceTime + ValidityTraceFidelity, 0.f, CameraSpline->Duration);
	}

	return true;
}

FRotator USCSplineCamera::GetFirstFrameRotation()
{
	if (!CameraSpline)
		return CachedDefaultTransform.Rotation;

	return CameraSpline->GetRotationAtSplineInputKey(0.f, ESplineCoordinateSpace::World);
}

#if WITH_EDITOR
void USCSplineCamera::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(USCSplineCamera, PreviewTime))
	{
		PreviewTime = FMath::Clamp(PreviewTime, KINDA_SMALL_NUMBER, SplineDuration - KINDA_SMALL_NUMBER);

		if (InitCameraSpline())
		{
			// Make sure the duration is updated properly.
			CameraSpline->UpdateDuration();

			if (CameraSpline->GetNumberOfSplinePoints() >= 2)
			{
				PreviewTime = FMath::Clamp(PreviewTime, KINDA_SMALL_NUMBER, CameraSpline->Duration - KINDA_SMALL_NUMBER);
				FSCCameraSplinePoint WorldPoint = CameraSpline->GetSplinePointAtTime(PreviewTime, ESplineCoordinateSpace::World);
				FSCCameraSplinePoint LocalPoint = CameraSpline->GetSplinePointAtTime(PreviewTime, ESplineCoordinateSpace::Local);

				if (ViewportPreview)
				{
					if (FLevelEditorViewportClient* Client = (FLevelEditorViewportClient*)GEditor->GetActiveViewport()->GetClient())
					{
						Client->SetViewLocation(WorldPoint.Position);
						Client->SetViewRotation(WorldPoint.Rotation);
					}
				}

				SetRelativeLocationAndRotation(LocalPoint.Position, LocalPoint.Rotation);
				SetFieldOfView(LocalPoint.FOV);
			}
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

bool USCSplineCamera::InitCameraSpline()
{
	CameraSpline = Cast<USCCameraSplineComponent>(GetAttachParent());

	return CameraSpline != nullptr;
}
