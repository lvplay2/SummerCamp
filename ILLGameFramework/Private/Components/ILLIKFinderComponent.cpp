// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "Components/ILLIKFinderComponent.h"

#include "MessageLog.h"
#include "UObjectToken.h"

#define MIN_DIFF_SQ (10.f * 10.f)

TAutoConsoleVariable<int32> CVarILLDebugIKFinder(
	TEXT("ill.DebugIKFinder"),
	0,
	TEXT("Displays debug information for the local IKFinder\n")
	TEXT(" 0: Off\n")
	TEXT(" 1: On\n")
	TEXT(" 2: Text output\n")
);

FILLIKPoint::FILLIKPoint()
: Position(FVector::ZeroVector)
, Normal(FVector::ZeroVector)
, PhysicalMaterial(EPhysicalSurface::SurfaceType_Default)
, Timestamp(-1.f)
, Actor(nullptr)
, Component(nullptr)
{
}

FILLIKPoint::FILLIKPoint(const FHitResult& HitResult, const float InTimestamp)
: Position(HitResult.ImpactPoint)
, Normal(HitResult.ImpactNormal)
, Timestamp(InTimestamp)
, Actor(HitResult.Actor)
, Component(HitResult.Component)
{
	PhysicalMaterial = UGameplayStatics::GetSurfaceType(HitResult);
}

UILLIKFinderComponent::UILLIKFinderComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, SkeletalMeshName(NAME_None)
, TargetBone(NAME_None)
, EndBone(NAME_None)
, IKArmLength(50.f)
, MinYaw(0.f)
, MaxYaw(45.f)
, MinPitch(-65.f)
, MaxPitch(35.f)
, BaseDirection(FVector::ForwardVector)
, SearchDistance(75.f)
, SearchRows(2)
, RowPitchMaxOffset(15.f)
, SearchCollumns(3)
, CollumnYawMaxOffset(25.f)
, TraceChannel(ECollisionChannel::ECC_Visibility)
, SearchDelay(30.f)
, bUseAsyncWorld(true)
, bUseComplexSearch(false)
, bIgnoreCharacters(true)
, PointLifespan(2.5f)
, BestPointBias(EILLIKSearchBias::Close)
, BestPointLifespan(15.f)
, IKAlpha(0.f)
, BlendInDelay(0.25f)
, BlendInTime(0.5f)
, BlendOutTime(0.25f)
, LastUsed_Timestamp(-FLT_MAX)
, IKArmLengthSq(0.f)
{
	bAllowConcurrentTick = true;
	bAutoActivate = true;
	bCanEverAffectNavigation = false;

	PrimaryComponentTick.bAllowTickOnDedicatedServer = false; // Client-side only
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bRunOnAnyThread = false;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.bTickEvenWhenPaused = false;
}

void UILLIKFinderComponent::SetActive(bool bNewActive, bool bReset /*= false*/)
{
	Super::SetActive(bNewActive, bReset);

	if (bReset)
	{
		LastUsed_Timestamp = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		BestPoint = FILLIKPoint();
		ValidPoints.Empty();
		IKAlpha = 0.f;
	}
}

void UILLIKFinderComponent::BeginPlay()
{
	Super::BeginPlay();

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	AActor* OwningActor = GetOwner();
	TArray<UActorComponent*> SkeletalComponents = OwningActor->GetComponentsByClass(USkeletalMeshComponent::StaticClass());

	// Start by finding the matching skeletal mesh
	for (UActorComponent* Mesh : SkeletalComponents)
	{
		if (SkeletalMeshName == Mesh->GetFName())
		{
			if (SkeletalMesh.Get() != nullptr)
			{
#if WITH_EDITOR
				// Multiple skeletal meshes found, unlikely and very odd
				FMessageLog("PIE").Error(FText::FromString(FString::Printf(TEXT("Found a duplicate skeletal mesh with the name: %s From: "), *SkeletalMeshName.ToString())))
					->AddToken(FUObjectToken::Create(this));
#endif
			}
			else
			{
				SkeletalMesh = Cast<USkeletalMeshComponent>(Mesh);
			}
		}
	}

	if (SkeletalMesh.Get() == nullptr)
	{
#if WITH_EDITOR
		FString PlausableSkeletals;
		for (UActorComponent* Mesh : SkeletalComponents)
		{
			PlausableSkeletals.Append(FString::Printf(TEXT("%s, "), *Mesh->GetName()));
		}

		PlausableSkeletals.RemoveAt(PlausableSkeletals.Len() - 2, 2);

		// Couldn't find a matching skeletal mesh, and won't be searching
		FMessageLog("PIE").Error(FText::FromString(FString::Printf(TEXT("Couldn't find a skeletal mesh named: %s. Possible options: %s. Searching will be disabled. From: "), *SkeletalMeshName.ToString(), *PlausableSkeletals)))
			->AddToken(FUObjectToken::Create(this));
#endif

		SetActive(false);
		return;
	}

	// Setup our ray traces
	if (SearchRows * SearchCollumns <= 0)
	{
#if WITH_EDITOR
		// We have a bad number of rows/collumns, and won't be searching
		FMessageLog("PIE").Error(FText::FromString(FString::Printf(TEXT("Rows (%d) or collumns (%d) were 0 or less, gotta have some rays to trace with! Searching will be disabled. From: "), SearchRows, SearchCollumns)))
			->AddToken(FUObjectToken::Create(this));
#endif

		SetActive(false);
		return;
	}

	// Setup all ray trace directions
	BaseDirection.Normalize();

	const FRotator RowMin(0.f, -RowPitchMaxOffset, 0.f),		RowMax(0.f, RowPitchMaxOffset, 0.f);
	const FRotator CollumnMin(0.f, -CollumnYawMaxOffset, 0.f),	CollumnMax(0.f, CollumnYawMaxOffset, 0.f);
	for (int32 Row = 0; Row < SearchRows; ++Row)
	{
		const float RowAlpha = (float)Row / (float)(SearchRows - 1);
		const float Pitch(FMath::Lerp(-RowPitchMaxOffset, RowPitchMaxOffset, RowAlpha));
		for (int32 Col = 0; Col < SearchCollumns; ++Col)
		{
			const float CollumnAlpha = (float)Col / (float)(SearchCollumns - 1);
			const float Yaw(FMath::Lerp(-CollumnYawMaxOffset, CollumnYawMaxOffset, CollumnAlpha));

			SearchDirections.Add(FRotator(Pitch, Yaw, 0.f).RotateVector(BaseDirection).GetSafeNormal());
		}
	}

	// Setup our trace params
	TraceParams.AddIgnoredActor(OwningActor);
	TraceParams.TraceTag = TEXT("IKFinder");
	TraceParams.bReturnPhysicalMaterial = true;
	TraceParams.bTraceAsyncScene = bUseAsyncWorld;
	TraceParams.bTraceComplex = bUseComplexSearch;

	LastUsed_Timestamp = CurrentTime;

	IKArmLengthSq = FMath::Square(IKArmLength);

	SetComponentTickEnabled(IsActive());
}

void UILLIKFinderComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const UWorld* World = GetWorld();
	const float CurrentTime = World->GetTimeSeconds();

	const float OldAlpha = IKAlpha;

	// If we're using our best point, don't do any more ray tracing
	bool bIsUsingBestPoint = BestPoint.Timestamp > 0.f;
	const FVector BonePosition = SkeletalMesh->GetBoneLocation(TargetBone);
	const FRotator Orientation = SkeletalMesh->GetComponentRotation().RotateVector(BaseDirection).Rotation();

	// Update our currently blended point
	if (bIsUsingBestPoint)
	{
		// Check if we're blending in
		bool bBlendingIn = false;
		const float BestPointLifetime = CurrentTime - BestPoint.Timestamp;
		if (BestPointLifespan <= 0.f || BestPointLifetime < BestPointLifespan + BlendInDelay)
			bBlendingIn = IsPointValidBestPoint(BonePosition, Orientation, BestPoint);

		if (bBlendingIn)
		{
			if (BestPointLifetime > BlendInDelay)
				IKAlpha += DeltaTime / BlendInTime;
		}
		else
		{
			IKAlpha -= DeltaTime / BlendOutTime;
		}

		IKAlpha = FMath::Clamp(IKAlpha, 0.f, 1.f);

		// Are we fully blended out?
		bIsUsingBestPoint = bBlendingIn || IKAlpha > 0.f;

		if (bIsUsingBestPoint)
		{
			if (IKAlpha > 0.f)
				LastUsed_Timestamp = CurrentTime;
		}
		else
		{
			// Clear our best point
			BestPoint = FILLIKPoint();
		}
	}

	// Trim expired and out of range valid points
	const float SearchDistanceSq = FMath::Square(SearchDistance);
	for (int32 iPoint = 0; iPoint < ValidPoints.Num(); ++iPoint)
	{
		FILLIKPoint& Point = ValidPoints[iPoint];
		if (CurrentTime - Point.Timestamp > PointLifespan)
		{
			ValidPoints.RemoveAt(iPoint);
			--iPoint;
			continue;
		}
		else if ((BonePosition - Point.Position).SizeSquared() > SearchDistanceSq)
		{
			ValidPoints.RemoveAt(iPoint);
			--iPoint;
			continue;
		}
	}

	// See if we should raytrace
	if (!bIsUsingBestPoint && CurrentTime - LastUsed_Timestamp >= SearchDelay)
	{
		// Raytrace!
		for (const FVector& Dir : SearchDirections)
		{
			FHitResult Hit;
			const FVector WorldDir = SkeletalMesh->GetComponentRotation().RotateVector(Dir);
			const bool FoundHit = World->LineTraceSingleByChannel(Hit, BonePosition, BonePosition + WorldDir * SearchDistance, TraceChannel, TraceParams);

			if (FoundHit == false)
				continue;

			// Let's maybe not put our hands on people
			if (bIgnoreCharacters && Hit.Actor.Get() && Cast<ACharacter>(Hit.Actor.Get()))
				continue;

			// Any new points should be compared against our list of valid points
			bool NewHit = true;
			for (FILLIKPoint& Point : ValidPoints)
			{
				// Anything within 10 cm of a point just refreshes that points lifespan
				if (FVector::DistSquared(Hit.ImpactPoint, Point.Position) <= MIN_DIFF_SQ)
				{
					Point.Timestamp = CurrentTime;
					NewHit = false;
					break;
				}
			}

			// This is a new point, add it
			if (NewHit)
			{
				ValidPoints.Add(FILLIKPoint(Hit, CurrentTime));
			}
		}

		// We've found a bunch of points, pick a BEST point
		FILLIKPoint* PossibleBestPoint = nullptr;
		for (FILLIKPoint& Point : ValidPoints)
		{
			if (IsPointValidBestPoint(BonePosition, Orientation, Point))
			{
				// Is this new point better than our current one?
				if (PossibleBestPoint == nullptr || PointCompare(BonePosition, Orientation, *PossibleBestPoint, Point) > 0)
				{
					PossibleBestPoint = &Point;
				}
			}
		}

		// Do we have a valid BEST point?
		if (PossibleBestPoint)
		{
			// Start blending it in!
			BestPoint = *PossibleBestPoint;
			BestPoint.Timestamp = CurrentTime;
			IKAlpha = 0.f;
		}
	}

	// See if we fully blended in a point this frame
	if (OldAlpha < 1.f && IKAlpha >= 1.f)
	{
		OnPointFullyBlendedIn(SkeletalMeshName, TargetBone, BestPoint);
	}

#if WITH_EDITOR
	// Debug draw
	if (CVarILLDebugIKFinder.GetValueOnGameThread() > 0)
	{
		if (CVarILLDebugIKFinder.GetValueOnGameThread() > 1)
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Cyan, FString::Printf(TEXT("IK Searcher %s : %s (%d x %d)"), *SkeletalMeshName.ToString(), *TargetBone.ToString(), SearchRows, SearchCollumns), false);

		for (const FVector& Dir : SearchDirections)
		{
			const FVector WorldDir = SkeletalMesh->GetComponentRotation().RotateVector(Dir);
			DrawDebugDirectionalArrow(World, BonePosition, BonePosition + WorldDir * SearchDistance, 25.f, FColor::Red);
			if (CVarILLDebugIKFinder.GetValueOnGameThread() > 1)
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Cyan, FString::Printf(TEXT(" - %s"), *Dir.ToString()), false);
		}

		for (const FILLIKPoint& Point : ValidPoints)
		{
			const float Alpha = 1.f - ((CurrentTime - Point.Timestamp) / PointLifespan);
			const FColor Color((uint8)(Alpha * 255.f), 0, 0);
			DrawDebugSphere(World, Point.Position, 10.f, 16, Color);
		}

		if (CVarILLDebugIKFinder.GetValueOnGameThread() > 1)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Cyan, FString::Printf(TEXT(" - %d points"), ValidPoints.Num()), false);
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Cyan, FString::Printf(TEXT(" - best point "), BestPoint.Timestamp > 0.f ? *BestPoint.Position.ToString() : TEXT("not found")), false);
		}

		if (IKAlpha > 0.f)
			DrawDebugSphere(World, BestPoint.Position, 15.f, 16, FColor(0, (uint8)(IKAlpha * 255.f), 0));
	}
#endif
}

bool UILLIKFinderComponent::GetBestPointInfo(const float Offset, float& Alpha, FVector& Location, FRotator& Rotation) const
{
	if (!IsActive())
	{
		Alpha = 0.f;
		return false;
	}

	Alpha = IKAlpha;
	if (Alpha > 0.f)
	{
		Location = BestPoint.Position + BestPoint.Normal * Offset;
		Rotation = BestPoint.Normal.Rotation() + SkeletalMesh->GetRelativeTransform().Rotator();

		if (!EndBone.IsNone())
		{
			const FRotator EndBoneRotation = SkeletalMesh->GetBoneQuaternion(EndBone).Rotator();
			Rotation = FMath::Lerp(EndBoneRotation, Rotation, Alpha);

#if WITH_EDITOR
			if (CVarILLDebugIKFinder.GetValueOnGameThread() > 0)
			{
				DrawDebugLine(GetWorld(), Location, Location + Rotation.Quaternion().GetForwardVector() * 20.f, FColor::Red, false, 0.f, 255);
				DrawDebugLine(GetWorld(), Location, Location + Rotation.Quaternion().GetRightVector() * 20.f, FColor::Green, false, 0.f, 255);
				DrawDebugLine(GetWorld(), Location, Location + Rotation.Quaternion().GetUpVector() * 20.f, FColor::Blue, false, 0.f, 255);
			}
#endif

			Rotation -= SkeletalMesh->GetComponentRotation();
			Rotation.Normalize();
		}
	}

	return true;
}

bool UILLIKFinderComponent::IsPointValidBestPoint(const FVector& Origin, const FRotator& Orientation, const FILLIKPoint& Point) const
{
	const FVector ToPoint(Point.Position - Origin);
	if (ToPoint.SizeSquared() > IKArmLengthSq)
		return false;

	const FRotator ToPointRot(ToPoint.Rotation());
	const float YawOffset = FMath::Abs(FRotator::NormalizeAxis(Orientation.Yaw - ToPointRot.Yaw));
	if (YawOffset > MaxYaw || YawOffset < MinYaw)
		return false;

	const float PitchOffset = FMath::Abs(FRotator::NormalizeAxis(Orientation.Pitch - ToPointRot.Pitch));
	if (PitchOffset > MaxPitch || PitchOffset < MinPitch)
		return false;

	return true;
}

int32 UILLIKFinderComponent::PointCompare(const FVector& Origin, const FRotator& Orientation, const FILLIKPoint& Left, const FILLIKPoint& Right) const
{
	int32 Invert = 1; // Used to flip responses so we don't have to write twice the code
	FVector Dir = FVector::ZeroVector;
	switch (BestPointBias)
	{
	case EILLIKSearchBias::Far:
		Invert = -1;
		//break; Fallthrough
	case EILLIKSearchBias::Close:
		{
			const float LeftDist = FVector::DistSquared(Origin, Left.Position);
			const float RightDist = FVector::DistSquared(Origin, Right.Position);
			if (LeftDist < RightDist)
				return -1 * Invert;
			else if (LeftDist > RightDist)
				return 1 * Invert;
			return 0;
		}
		break;

	case EILLIKSearchBias::Back:
		Invert = -1;
		//break; Fallthrough
	case EILLIKSearchBias::Forward:
		Dir = Orientation.RotateVector(FVector::ForwardVector);
		break;

	case EILLIKSearchBias::Left:
		Invert = -1;
		//break; Fallthrough
	case EILLIKSearchBias::Right:
		Dir = Orientation.RotateVector(FVector::RightVector);
		break;

	case EILLIKSearchBias::Down:
		Invert = -1;
		//break; Fallthrough
	case EILLIKSearchBias::Up:
		Dir = Orientation.RotateVector(FVector::UpVector);
		break;

	// Should not happen
	default:
		return 0;
	}
	
	const float LeftDot = Left.Position | Dir;
	const float RightDot = Right.Position | Dir;
	if (LeftDot > RightDot)
		return 1 * Invert;
	else if (LeftDot < RightDot)
		return -1 * Invert;
	return 0;
}
