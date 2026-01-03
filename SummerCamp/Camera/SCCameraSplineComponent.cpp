// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

/*=============================================================================
 * RIPPED FROM Runtime/Engine/Private/Components/SplineComponent.cpp
 SCCameraSplineComponent.cpp
 =============================================================================*/

#include "SummerCamp.h"
#include "SCCameraSplineComponent.h"

// UE4
#include "MessageLog.h"
#include "UObjectToken.h"

//#include "EnginePrivate.h"
#include "ComponentInstanceDataCache.h"
#include "EditorObjectVersion.h"

#include "SCSplineCamera.h"

USCCameraSplineComponent::USCCameraSplineComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bAllowSplineEditingPerInstance_DEPRECATED(true)
	, ReparamStepsPerSegment(10)
	, Duration(1.0f)
	, bStationaryEndpoints(false)
	, bSplineHasBeenEdited(false)
	, bModifiedByConstructionScript(false)
	, bInputSplinePointsToConstructionScript(false)
	, bDrawDebug(true)
	, bClosedLoop(false)
	, DefaultUpVector(FVector::UpVector)
#if WITH_EDITORONLY_DATA
	, EditorUnselectedSplineSegmentColor(FLinearColor(1.0f, 1.0f, 1.0f))
	, EditorSelectedSplineSegmentColor(FLinearColor(1.0f, 0.0f, 0.0f))
	, bAllowDiscontinuousSpline(false)
	, bShouldVisualizeScale(false)
	, ScaleVisualizationWidth(30.0f)
#endif
{
	SplineCurves.Position.Points.Reset(10);
	SplineCurves.Rotation.Points.Reset(10);
	SplineCurves.Scale.Points.Reset(10);
	SplineCurves.FOVCurve.Points.Reset(10);
	SplineCurves.HoldTimeCurve.Points.Reset(10);
	SplineCurves.TimeToNextCurve.Points.Reset(10);
	SplineCurves.TimeAtPointCurve.Points.Reset(10);

	SplineCurves.Position.Points.Emplace(0.0f, FVector(0, 0, 0), FVector::ZeroVector, FVector::ZeroVector, CIM_CurveAuto);
	SplineCurves.Rotation.Points.Emplace(0.0f, FQuat::Identity, FQuat::Identity, FQuat::Identity, CIM_Linear);
	SplineCurves.Scale.Points.Emplace(0.0f, FVector(1.0f), FVector::ZeroVector, FVector::ZeroVector, CIM_CurveAuto);
	SplineCurves.FOVCurve.Points.Emplace(0.0f, 90.f, 0.f, 0.f, CIM_CurveAuto);
	SplineCurves.HoldTimeCurve.Points.Emplace(0.0f, 0.f, 0.f, 0.f, CIM_Constant);
	SplineCurves.TimeToNextCurve.Points.Emplace(0.0f, 1.f, 0.f, 0.f, CIM_Constant);
	SplineCurves.TimeAtPointCurve.Points.Emplace(0.0f, 0.f, 0.f, 0.f, CIM_Constant);

	SplineCurves.Position.Points.Emplace(1.0f, FVector(100, 0, 0), FVector::ZeroVector, FVector::ZeroVector, CIM_CurveAuto);
	SplineCurves.Rotation.Points.Emplace(1.0f, FQuat::Identity, FQuat::Identity, FQuat::Identity, CIM_Linear);
	SplineCurves.Scale.Points.Emplace(1.0f, FVector(1.0f), FVector::ZeroVector, FVector::ZeroVector, CIM_CurveAuto);
	SplineCurves.FOVCurve.Points.Emplace(1.0f, 90.f, 0.f, 0.f, CIM_CurveAuto);
	SplineCurves.HoldTimeCurve.Points.Emplace(1.0f, 0.f, 0.f, 0.f, CIM_Constant);
	SplineCurves.TimeToNextCurve.Points.Emplace(1.0f, 1.f, 0.f, 0.f, CIM_Constant);
	SplineCurves.TimeAtPointCurve.Points.Emplace(1.0f, 1.f, 0.f, 0.f, CIM_Constant);

	UpdateSpline();

	// Set these deprecated values up so that old assets with default values load correctly (and are subsequently upgraded during Serialize)
	SplineInfo_DEPRECATED = SplineCurves.Position;
	SplineRotInfo_DEPRECATED = SplineCurves.Rotation;
	SplineScaleInfo_DEPRECATED = SplineCurves.Scale;
	SplineReparamTable_DEPRECATED = SplineCurves.ReparamTable;
}

void USCCameraSplineComponent::BeginPlay()
{
	// Make sure the duration is up to date when the game starts.
	UpdateDuration();
	Super::BeginPlay();
}

/*
EInterpCurveMode ConvertSplinePointTypeToInterpCurveMode(ESplinePointType::Type SplinePointType)
{
	switch (SplinePointType)
	{
	case ESplinePointType::Linear:				return CIM_Linear;
	case ESplinePointType::Curve:				return CIM_CurveAuto;
	case ESplinePointType::Constant:			return CIM_Constant;
	case ESplinePointType::CurveCustomTangent:	return CIM_CurveUser;
	case ESplinePointType::CurveClamped:		return CIM_CurveAutoClamped;

	default:									return CIM_Unknown;
	}
}

ESplinePointType::Type ConvertInterpCurveModeToSplinePointType(EInterpCurveMode InterpCurveMode)
{
	switch (InterpCurveMode)
	{
	case CIM_Linear:			return ESplinePointType::Linear;
	case CIM_CurveAuto:			return ESplinePointType::Curve;
	case CIM_Constant:			return ESplinePointType::Constant;
	case CIM_CurveUser:			return ESplinePointType::CurveCustomTangent;
	case CIM_CurveAutoClamped:	return ESplinePointType::CurveClamped;

	default:					return ESplinePointType::Constant;
	}
}
*/

void USCCameraSplineComponent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FEditorObjectVersion::GUID);

	// Move points to new properties
	if (Ar.CustomVer(FEditorObjectVersion::GUID) < FEditorObjectVersion::SplineComponentCurvesInStruct &&
		Ar.IsLoading())
	{
		SplineCurves.Position = SplineInfo_DEPRECATED;
		SplineCurves.Rotation = SplineRotInfo_DEPRECATED;
		SplineCurves.Scale = SplineScaleInfo_DEPRECATED;
		SplineCurves.ReparamTable = SplineReparamTable_DEPRECATED;
	}

	// Support old resources which don't have the rotation and scale splines present
	const int32 ArchiveUE4Version = Ar.UE4Ver();
	if (ArchiveUE4Version < VER_UE4_INTERPCURVE_SUPPORTS_LOOPING)
	{
		int32 NumPoints = SplineCurves.Position.Points.Num();

		// The start point is no longer cloned as the endpoint when the spline is looped, so remove the extra endpoint if present
		const bool bHasExtraEndpoint = bClosedLoop && (SplineCurves.Position.Points[0].OutVal == SplineCurves.Position.Points[NumPoints - 1].OutVal);

		if (bHasExtraEndpoint)
		{
			SplineCurves.Position.Points.RemoveAt(NumPoints - 1, 1, false);
			NumPoints--;
		}

		// Fill the other two splines with some defaults
		SplineCurves.Rotation.Points.Reset(NumPoints);
		SplineCurves.Scale.Points.Reset(NumPoints);
		for (int32 Count = 0; Count < NumPoints; Count++)
		{
			SplineCurves.Rotation.Points.Emplace(0.0f, FQuat::Identity, FQuat::Identity, FQuat::Identity, CIM_Linear);
			SplineCurves.Scale.Points.Emplace(0.0f, FVector(1.0f), FVector::ZeroVector, FVector::ZeroVector, CIM_CurveAuto);
		}

		UpdateSpline();
	}
}


void USCCameraSplineComponent::PostLoad()
{
	Super::PostLoad();
}


void USCCameraSplineComponent::PostEditImport()
{
	Super::PostEditImport();
}


void USCCameraSplineComponent::UpdateSpline()
{
	const int32 NumPoints = SplineCurves.Position.Points.Num();
	bool AllValidPoints = SplineCurves.Rotation.Points.Num() == NumPoints
						&& SplineCurves.Scale.Points.Num() == NumPoints
						&& SplineCurves.FOVCurve.Points.Num() == NumPoints
						&& SplineCurves.HoldTimeCurve.Points.Num() == NumPoints
						&& SplineCurves.TimeToNextCurve.Points.Num() == NumPoints
						&& SplineCurves.TimeAtPointCurve.Points.Num() == NumPoints;

	if (!AllValidPoints)
	{
		ResetSplineDefaults();
		return;
	}

#if DO_CHECK
	// Ensure input keys and TimeAtPoint keys are strictly ascending
	bool AllAscendingPoints = true;
	for (int32 Index = 1; Index < NumPoints; Index++)
	{
		AllAscendingPoints = SplineCurves.Position.Points[Index - 1].InVal < SplineCurves.Position.Points[Index].InVal;
		if (!AllAscendingPoints)
			break;

		AllAscendingPoints = SplineCurves.TimeAtPointCurve.Points[Index - 1].OutVal < SplineCurves.TimeAtPointCurve.Points[Index].OutVal;
		if (!AllAscendingPoints)
			break;
	}

	if (!AllAscendingPoints)
	{
		ResetSplineDefaults();
		return;
	}
#endif

	// Verify hold times and time to next with Time at point along the spline.
	for (int32 Index = 1; Index < NumPoints; Index++)
	{
		// Make sure we're adding a valid TimeAtPoint value based on previous and next points in the curve.
		float PrevTimeAtPoint = SplineCurves.TimeAtPointCurve.Points[Index - 1].OutVal;
		float TimeAtIndex = SplineCurves.TimeAtPointCurve.Points[Index].OutVal;

		float OldHoldTime = SplineCurves.HoldTimeCurve.Points[Index - 1].OutVal;
		float NewTimeToNext = (TimeAtIndex - PrevTimeAtPoint) - OldHoldTime;
		if (NewTimeToNext < 0)
		{
			FMath::Clamp(OldHoldTime + NewTimeToNext, 0.f, OldHoldTime);
			NewTimeToNext = 0.f;
		}

		SplineCurves.HoldTimeCurve.Points[Index - 1].OutVal = OldHoldTime;
		SplineCurves.TimeToNextCurve.Points[Index - 1].OutVal = NewTimeToNext;
	}

	// Ensure splines' looping status matches with that of the spline component
	if (bClosedLoop)
	{
		const float LoopKey = bLoopPositionOverride ? LoopPosition : SplineCurves.Position.Points.Last().InVal + 1.0f;
		SplineCurves.Position.SetLoopKey(LoopKey);
		SplineCurves.Rotation.SetLoopKey(LoopKey);
		SplineCurves.Scale.SetLoopKey(LoopKey);
		SplineCurves.FOVCurve.SetLoopKey(LoopKey);
		SplineCurves.HoldTimeCurve.SetLoopKey(LoopKey);
		SplineCurves.TimeToNextCurve.SetLoopKey(LoopKey);
		SplineCurves.TimeAtPointCurve.SetLoopKey(LoopKey);
	}
	else
	{
		SplineCurves.Position.ClearLoopKey();
		SplineCurves.Rotation.ClearLoopKey();
		SplineCurves.Scale.ClearLoopKey();
		SplineCurves.FOVCurve.ClearLoopKey();
		SplineCurves.HoldTimeCurve.ClearLoopKey();
		SplineCurves.TimeToNextCurve.ClearLoopKey();
		SplineCurves.TimeAtPointCurve.ClearLoopKey();
	}

	// Automatically set the tangents on any CurveAuto keys
	SplineCurves.Position.AutoSetTangents(0.0f, bStationaryEndpoints);
	//SplineCurves.Rotation.AutoSetTangents(0.0f, bStationaryEndpoints);
	SplineCurves.Scale.AutoSetTangents(0.0f, bStationaryEndpoints);
	SplineCurves.FOVCurve.AutoSetTangents(0.0f, bStationaryEndpoints);

	// Now initialize the spline reparam table
	const int32 NumSegments = bClosedLoop ? NumPoints : NumPoints - 1;

	// Start by clearing it
	SplineCurves.ReparamTable.Points.Reset(NumSegments * ReparamStepsPerSegment + 1);
	float AccumulatedLength = 0.0f;
	for (int32 SegmentIndex = 0; SegmentIndex < NumSegments; ++SegmentIndex)
	{
		for (int32 Step = 0; Step < ReparamStepsPerSegment; ++Step)
		{
			const float Param = static_cast<float>(Step) / ReparamStepsPerSegment;
			const float SegmentLength = (Step == 0) ? 0.0f : GetSegmentLength(SegmentIndex, Param);

			SplineCurves.ReparamTable.Points.Emplace(SegmentLength + AccumulatedLength, SegmentIndex + Param, 0.0f, 0.0f, CIM_Linear);
		}
		AccumulatedLength += GetSegmentLength(SegmentIndex, 1.0f);
	}

	SplineCurves.ReparamTable.Points.Emplace(AccumulatedLength, static_cast<float>(NumSegments), 0.0f, 0.0f, CIM_Linear);

#if !UE_BUILD_SHIPPING
	if (bDrawDebug)
	{
		MarkRenderStateDirty();
	}
#endif

	UpdateDuration();
}


float USCCameraSplineComponent::GetSegmentLength(const int32 Index, const float Param) const
{
	const int32 NumPoints = SplineCurves.Position.Points.Num();
	const int32 LastPoint = NumPoints - 1;

	check(Index >= 0 && ((bClosedLoop && Index < NumPoints) || (!bClosedLoop && Index < LastPoint)));
	check(Param >= 0.0f && Param <= 1.0f);

	// Evaluate the length of a Hermite spline segment.
	// This calculates the integral of |dP/dt| dt, where P(t) is the spline equation with components (x(t), y(t), z(t)).
	// This isn't solvable analytically, so we use a numerical method (Legendre-Gauss quadrature) which performs very well
	// with functions of this type, even with very few samples.  In this case, just 5 samples is sufficient to yield a
	// reasonable result.

	struct FLegendreGaussCoefficient
	{
		float Abscissa;
		float Weight;
	};

	static const FLegendreGaussCoefficient LegendreGaussCoefficients[] =
	{
		{ 0.0f, 0.5688889f },
		{ -0.5384693f, 0.47862867f },
		{ 0.5384693f, 0.47862867f },
		{ -0.90617985f, 0.23692688f },
		{ 0.90617985f, 0.23692688f }
	};

	const auto& StartPoint = SplineCurves.Position.Points[Index];
	const auto& EndPoint = SplineCurves.Position.Points[Index == LastPoint ? 0 : Index + 1];

	const FVector Scale3D = GetComponentToWorld().GetScale3D();

	const auto& P0 = StartPoint.OutVal;
	const auto& T0 = StartPoint.LeaveTangent;
	const auto& P1 = EndPoint.OutVal;
	const auto& T1 = EndPoint.ArriveTangent;

	// Special cases for linear or constant segments
	if (StartPoint.InterpMode == CIM_Linear)
	{
		return ((P1 - P0) * Scale3D).Size() * Param;
	}
	else if (StartPoint.InterpMode == CIM_Constant)
	{
		return 0.0f;
	}

	// Cache the coefficients to be fed into the function to calculate the spline derivative at each sample point as they are constant.
	const FVector Coeff1 = ((P0 - P1) * 2.0f + T0 + T1) * 3.0f;
	const FVector Coeff2 = (P1 - P0) * 6.0f - T0 * 4.0f - T1 * 2.0f;
	const FVector Coeff3 = T0;

	const float HalfParam = Param * 0.5f;

	float Length = 0.0f;
	for (const auto& LegendreGaussCoefficient : LegendreGaussCoefficients)
	{
		// Calculate derivative at each Legendre-Gauss sample, and perform a weighted sum
		const float Alpha = HalfParam * (1.0f + LegendreGaussCoefficient.Abscissa);
		const FVector Derivative = ((Coeff1 * Alpha + Coeff2) * Alpha + Coeff3) * Scale3D;
		Length += Derivative.Size() * LegendreGaussCoefficient.Weight;
	}
	Length *= HalfParam;

	return Length;
}


float USCCameraSplineComponent::GetSegmentParamFromLength(const int32 Index, const float Length, const float SegmentLength) const
{
	if (SegmentLength == 0.0f)
	{
		return 0.0f;
	}

	// Given a function P(x) which yields points along a spline with x = 0...1, we can define a function L(t) to be the
	// Euclidian length of the spline from P(0) to P(t):
	//
	//    L(t) = integral of |dP/dt| dt
	//         = integral of sqrt((dx/dt)^2 + (dy/dt)^2 + (dz/dt)^2) dt
	//
	// This method evaluates the inverse of this function, i.e. given a length d, it obtains a suitable value for t such that:
	//    L(t) - d = 0
	//
	// We use Newton-Raphson to iteratively converge on the result:
	//
	//    t' = t - f(t) / (df/dt)
	//
	// where: t is an initial estimate of the result, obtained through basic linear interpolation,
	//        f(t) is the function whose root we wish to find = L(t) - d,
	//        (df/dt) = d(L(t))/dt = |dP/dt|

	// TODO: check if this works OK with delta InVal != 1.0f

	const int32 NumPoints = SplineCurves.Position.Points.Num();
	const int32 LastPoint = NumPoints - 1;

	check(Index >= 0 && ((bClosedLoop && Index < NumPoints) || (!bClosedLoop && Index < LastPoint)));
	check(Length >= 0.0f && Length <= SegmentLength);

	float Param = Length / SegmentLength;  // initial estimate for t

										   // two iterations of Newton-Raphson is enough
	for (int32 Iteration = 0; Iteration < 2; ++Iteration)
	{
		float TangentMagnitude = SplineCurves.Position.EvalDerivative(Index + Param, FVector::ZeroVector).Size();
		if (TangentMagnitude > 0.0f)
		{
			Param -= (GetSegmentLength(Index, Param) - Length) / TangentMagnitude;
			Param = FMath::Clamp(Param, 0.0f, 1.0f);
		}
	}

	return Param;
}


FVector USCCameraSplineComponent::GetLocationAtSplineInputKey(float InKey, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	FVector Location = SplineCurves.Position.Eval(InKey, FVector::ZeroVector);

	if (CoordinateSpace == ESplineCoordinateSpace::World)
	{
		Location = GetComponentToWorld().TransformPosition(Location);
	}

	return Location;
}


FVector USCCameraSplineComponent::GetTangentAtSplineInputKey(float InKey, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	FVector Tangent = SplineCurves.Position.EvalDerivative(InKey, FVector::ZeroVector);

	if (CoordinateSpace == ESplineCoordinateSpace::World)
	{
		Tangent = GetComponentToWorld().TransformVector(Tangent);
	}

	return Tangent;
}


FVector USCCameraSplineComponent::GetDirectionAtSplineInputKey(float InKey, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	FVector Direction = SplineCurves.Position.EvalDerivative(InKey, FVector::ZeroVector).GetSafeNormal();

	if (CoordinateSpace == ESplineCoordinateSpace::World)
	{
		Direction = GetComponentToWorld().TransformVectorNoScale(Direction);
	}

	return Direction;
}


FRotator USCCameraSplineComponent::GetRotationAtSplineInputKey(float InKey, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	return GetQuaternionAtSplineInputKey(InKey, CoordinateSpace).Rotator();
}


FQuat USCCameraSplineComponent::GetQuaternionAtSplineInputKey(float InKey, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	FQuat Quat = SplineCurves.Rotation.Eval(InKey, FQuat::Identity);
	Quat.Normalize();

	if (CoordinateSpace == ESplineCoordinateSpace::World)
	{
		Quat = GetComponentToWorld().GetRotation() * Quat;
	}

	return Quat;

	/* Why would we rotate it for get direction along spline... Use get direction.
	const FVector Direction = SplineCurves.Position.EvalDerivative(InKey, FVector::ZeroVector).GetSafeNormal();
	const FVector UpVector = Quat.RotateVector(DefaultUpVector);

	FQuat Rot = (FRotationMatrix::MakeFromXZ(Direction, UpVector)).ToQuat();

	return Rot;
	*/
}


FVector USCCameraSplineComponent::GetUpVectorAtSplineInputKey(float InKey, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const FQuat Quat = GetQuaternionAtSplineInputKey(InKey, ESplineCoordinateSpace::Local);
	FVector UpVector = Quat.RotateVector(FVector::UpVector);

	if (CoordinateSpace == ESplineCoordinateSpace::World)
	{
		UpVector = GetComponentToWorld().TransformVectorNoScale(UpVector);
	}

	return UpVector;
}


FVector USCCameraSplineComponent::GetRightVectorAtSplineInputKey(float InKey, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const FQuat Quat = GetQuaternionAtSplineInputKey(InKey, ESplineCoordinateSpace::Local);
	FVector RightVector = Quat.RotateVector(FVector::RightVector);

	if (CoordinateSpace == ESplineCoordinateSpace::World)
	{
		RightVector = GetComponentToWorld().TransformVectorNoScale(RightVector);
	}

	return RightVector;
}


FTransform USCCameraSplineComponent::GetTransformAtSplineInputKey(float InKey, ESplineCoordinateSpace::Type CoordinateSpace, bool bUseScale) const
{
	const FVector Location(GetLocationAtSplineInputKey(InKey, ESplineCoordinateSpace::Local));
	const FQuat Rotation(GetQuaternionAtSplineInputKey(InKey, ESplineCoordinateSpace::Local));
	const FVector Scale = bUseScale ? GetScaleAtSplineInputKey(InKey) : FVector(1.0f);

	FTransform Transform(Rotation, Location, Scale);

	if (CoordinateSpace == ESplineCoordinateSpace::World)
	{
		Transform = Transform * GetComponentToWorld();
	}

	return Transform;
}


float USCCameraSplineComponent::GetRollAtSplineInputKey(float InKey, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	return GetRotationAtSplineInputKey(InKey, CoordinateSpace).Roll;
}


FVector USCCameraSplineComponent::GetScaleAtSplineInputKey(float InKey) const
{
	const FVector Scale = SplineCurves.Scale.Eval(InKey, FVector(1.0f));
	return Scale;
}

float USCCameraSplineComponent::GetFOVAtSplineInputKey(float InKey) const
{
	const float FOV = SplineCurves.FOVCurve.Eval(InKey, 90.f);
	return FOV;
}

float USCCameraSplineComponent::GetHoldTimeAtSplineInputKey(float InKey) const
{
	const float HoldTime = SplineCurves.HoldTimeCurve.Eval(InKey, 0.f);
	return HoldTime;
}

float USCCameraSplineComponent::GetTimeToNextAtSplineInputKey(float InKey) const
{
	const float TimeToNext = SplineCurves.TimeToNextCurve.Eval(InKey, 1.f);
	return TimeToNext;
}

void USCCameraSplineComponent::SetClosedLoop(bool bInClosedLoop, bool bUpdateSpline)
{
	bClosedLoop = bInClosedLoop;
	bLoopPositionOverride = false;
	if (bUpdateSpline)
	{
		UpdateSpline();
	}
}


void USCCameraSplineComponent::SetClosedLoopAtPosition(bool bInClosedLoop, float Key, bool bUpdateSpline)
{
	bClosedLoop = bInClosedLoop;
	bLoopPositionOverride = bInClosedLoop;
	LoopPosition = Key;

	if (bUpdateSpline)
	{
		UpdateSpline();
	}
}


bool USCCameraSplineComponent::IsClosedLoop() const
{
	return bClosedLoop;
}


void USCCameraSplineComponent::SetUnselectedSplineSegmentColor(const FLinearColor& Color)
{
#if WITH_EDITORONLY_DATA
	EditorUnselectedSplineSegmentColor = Color;
#endif
}


void USCCameraSplineComponent::SetSelectedSplineSegmentColor(const FLinearColor& Color)
{
#if WITH_EDITORONLY_DATA
	EditorSelectedSplineSegmentColor = Color;
#endif
}


void USCCameraSplineComponent::SetDrawDebug(bool bShow)
{
	bDrawDebug = bShow;
	MarkRenderStateDirty();
}

void USCCameraSplineComponent::ResetSplineDefaults()
{
	ClearSplinePoints(false);

	SplineCurves.Position.Points.Emplace(0.0f, FVector(0, 0, 0), FVector::ZeroVector, FVector::ZeroVector, CIM_CurveAuto);
	SplineCurves.Rotation.Points.Emplace(0.0f, FQuat::Identity, FQuat::Identity, FQuat::Identity, CIM_Linear);
	SplineCurves.Scale.Points.Emplace(0.0f, FVector(1.0f), FVector::ZeroVector, FVector::ZeroVector, CIM_CurveAuto);
	SplineCurves.FOVCurve.Points.Emplace(0.0f, 90.f, 0.f, 0.f, CIM_CurveAuto);
	SplineCurves.HoldTimeCurve.Points.Emplace(0.0f, 0.f, 0.f, 0.f, CIM_Constant);
	SplineCurves.TimeToNextCurve.Points.Emplace(0.0f, 1.f, 0.f, 0.f, CIM_Constant);
	SplineCurves.TimeAtPointCurve.Points.Emplace(0.0f, 0.f, 0.f, 0.f, CIM_Constant);

	SplineCurves.Position.Points.Emplace(1.0f, FVector(100, 0, 0), FVector::ZeroVector, FVector::ZeroVector, CIM_CurveAuto);
	SplineCurves.Rotation.Points.Emplace(1.0f, FQuat::Identity, FQuat::Identity, FQuat::Identity, CIM_Linear);
	SplineCurves.Scale.Points.Emplace(1.0f, FVector(1.0f), FVector::ZeroVector, FVector::ZeroVector, CIM_CurveAuto);
	SplineCurves.FOVCurve.Points.Emplace(1.0f, 90.f, 0.f, 0.f, CIM_CurveAuto);
	SplineCurves.HoldTimeCurve.Points.Emplace(1.0f, 0.f, 0.f, 0.f, CIM_Constant);
	SplineCurves.TimeToNextCurve.Points.Emplace(1.0f, 1.f, 0.f, 0.f, CIM_Constant);
	SplineCurves.TimeAtPointCurve.Points.Emplace(1.0f, 1.f, 0.f, 0.f, CIM_Constant);

	UpdateSpline();
}

void USCCameraSplineComponent::ClearSplinePoints(bool bUpdateSpline)
{
	SplineCurves.Position.Points.Reset();
	SplineCurves.Rotation.Points.Reset();
	SplineCurves.Scale.Points.Reset();
	SplineCurves.FOVCurve.Points.Reset();
	SplineCurves.HoldTimeCurve.Points.Reset();
	SplineCurves.TimeToNextCurve.Points.Reset();
	SplineCurves.TimeAtPointCurve.Points.Reset();
	if (bUpdateSpline)
	{
		UpdateSpline();
	}
}


static int32 UpperBound(const TArray<FInterpCurvePoint<FVector>>& SplinePoints, float Value)
{
	int32 Count = SplinePoints.Num();
	int32 First = 0;

	while (Count > 0)
	{
		const int32 Middle = Count / 2;
		if (Value >= SplinePoints[First + Middle].InVal)
		{
			First += Middle + 1;
			Count -= Middle + 1;
		}
		else
		{
			Count = Middle;
		}
	}

	return First;
}


void USCCameraSplineComponent::AddPoint(const FSCCameraSplinePoint& InSplinePoint, bool bUpdateSpline)
{
	const int32 Index = UpperBound(SplineCurves.Position.Points, InSplinePoint.InputKey);

	SplineCurves.Position.Points.Insert(FInterpCurvePoint<FVector>(
		InSplinePoint.InputKey,
		InSplinePoint.Position,
		InSplinePoint.ArriveTangent,
		InSplinePoint.LeaveTangent,
		ConvertSplinePointTypeToInterpCurveMode(InSplinePoint.Type)
		),
		Index);

	SplineCurves.Rotation.Points.Insert(FInterpCurvePoint<FQuat>(
		InSplinePoint.InputKey,
		InSplinePoint.Rotation.Quaternion(),
		FQuat::Identity,
		FQuat::Identity,
		CIM_Linear
		),
		Index);

	SplineCurves.Scale.Points.Insert(FInterpCurvePoint<FVector>(
		InSplinePoint.InputKey,
		InSplinePoint.Scale,
		FVector::ZeroVector,
		FVector::ZeroVector,
		CIM_CurveAuto
		),
		Index);

	SplineCurves.FOVCurve.Points.Insert(FInterpCurvePoint<float>(
		InSplinePoint.InputKey,
		InSplinePoint.FOV,
		0.f,
		0.f,
		CIM_CurveAuto
		),
		Index);

	SplineCurves.HoldTimeCurve.Points.Insert(FInterpCurvePoint<float>(
		InSplinePoint.InputKey,
		InSplinePoint.HoldTime,
		0.f,
		0.f,
		CIM_Constant
		),
		Index);

	SplineCurves.TimeToNextCurve.Points.Insert(FInterpCurvePoint<float>(
		InSplinePoint.InputKey,
		InSplinePoint.TimeToNext,
		0.f,
		0.f,
		CIM_Constant
		),
		Index);

	SplineCurves.TimeAtPointCurve.Points.Insert(FInterpCurvePoint<float>(
		InSplinePoint.InputKey,
		InSplinePoint.TimeAtPoint,
		0.f,
		0.f,
		CIM_Constant
		),
		Index);

	if (bLoopPositionOverride && LoopPosition <= SplineCurves.Position.Points.Last().InVal)
	{
		bLoopPositionOverride = false;
	}

	if (bUpdateSpline)
	{
		UpdateSpline();
	}
}


void USCCameraSplineComponent::AddPoints(const TArray<FSCCameraSplinePoint>& InSplinePoints, bool bUpdateSpline)
{
	const int32 NumPoints = SplineCurves.Position.Points.Num();
	SplineCurves.Position.Points.Reserve(NumPoints + InSplinePoints.Num());

	for (const auto& SplinePoint : InSplinePoints)
	{
		AddPoint(SplinePoint, false);
	}

	if (bUpdateSpline)
	{
		UpdateSpline();
	}
}


void USCCameraSplineComponent::AddSplinePoint(const FVector& Position, ESplineCoordinateSpace::Type CoordinateSpace, bool bUpdateSpline)
{
	const FVector TransformedPosition = (CoordinateSpace == ESplineCoordinateSpace::World) ?
		GetComponentToWorld().InverseTransformPosition(Position) : Position;

	// Add the spline point at the end of the array, adding 1.0 to the current last input key.
	// This continues the former behavior in which spline points had to be separated by an interval of 1.0.
	const float InKey = SplineCurves.Position.Points.Num() ? SplineCurves.Position.Points.Last().InVal + 1.0f : 0.0f;

	SplineCurves.Position.Points.Emplace(InKey, TransformedPosition, FVector::ZeroVector, FVector::ZeroVector, CIM_CurveAuto);
	SplineCurves.Rotation.Points.Emplace(InKey, FQuat::Identity, FQuat::Identity, FQuat::Identity, CIM_Linear);
	SplineCurves.Scale.Points.Emplace(InKey, FVector(1.0f), FVector::ZeroVector, FVector::ZeroVector, CIM_CurveAuto);
	SplineCurves.FOVCurve.Points.Emplace(InKey, 90.f, 0.f, 0.f, CIM_CurveAuto);
	SplineCurves.HoldTimeCurve.Points.Emplace(InKey, 0.f, 0.f, 0.f, CIM_Constant);
	SplineCurves.TimeToNextCurve.Points.Emplace(InKey, 1.f, 0.f, 0.f, CIM_Constant);

	// We're adding a point at the end so just add 1 to the total time me thinks.
	float TimeAtPoint = SplineCurves.TimeAtPointCurve.Points[InKey - 1].OutVal;
	SplineCurves.TimeAtPointCurve.Points.Emplace(InKey, TimeAtPoint + 1.f, 0.f, 0.f, CIM_Constant);

	if (bLoopPositionOverride)
	{
		LoopPosition += 1.0f;
	}

	if (bUpdateSpline)
	{
		UpdateSpline();
	}
}

void USCCameraSplineComponent::AddSplinePointAtIndex(const FVector& Position, int32 Index, ESplineCoordinateSpace::Type CoordinateSpace, bool bUpdateSpline)
{
	const FVector TransformedPosition = (CoordinateSpace == ESplineCoordinateSpace::World) ?
		GetComponentToWorld().InverseTransformPosition(Position) : Position;

	int32 NumPoints = SplineCurves.Position.Points.Num();

	if (Index >= 0 && Index <= NumPoints)
	{
		const float InKey = (Index == 0) ? 0.0f : SplineCurves.Position.Points[Index - 1].InVal + 1.0f;

		SplineCurves.Position.Points.Insert(FInterpCurvePoint<FVector>(InKey, TransformedPosition, FVector::ZeroVector, FVector::ZeroVector, CIM_CurveAuto), Index);
		SplineCurves.Rotation.Points.Insert(FInterpCurvePoint<FQuat>(InKey, FQuat::Identity, FQuat::Identity, FQuat::Identity, CIM_Linear), Index);
		SplineCurves.Scale.Points.Insert(FInterpCurvePoint<FVector>(InKey, FVector(1.0f), FVector::ZeroVector, FVector::ZeroVector, CIM_CurveAuto), Index);
		SplineCurves.FOVCurve.Points.Insert(FInterpCurvePoint<float>(InKey, 90.f, 0.f, 0.f, CIM_CurveAuto), Index);
		SplineCurves.HoldTimeCurve.Points.Insert(FInterpCurvePoint<float>(InKey, 0.f, 0.f, 0.f, CIM_CurveAuto), Index);
		SplineCurves.TimeToNextCurve.Points.Insert(FInterpCurvePoint<float>(InKey, 1.f, 0.f, 0.f, CIM_CurveAuto), Index);

		// Make sure we're adding a valid TimeAtPoint value based on previous and next points in the curve.
		float PrevTimeAtPoint = 0.f;
		float NextTimeAtPoint = SplineCurves.TimeAtPointCurve.Points[Index].OutVal;
		if (Index > 0)
			PrevTimeAtPoint = SplineCurves.TimeAtPointCurve.Points[Index - 1].OutVal;

		SplineCurves.TimeAtPointCurve.Points[Index].OutVal = PrevTimeAtPoint + ((NextTimeAtPoint - PrevTimeAtPoint) * 0.5f);

		SplineCurves.TimeAtPointCurve.Points.Insert(FInterpCurvePoint<float>(InKey, NextTimeAtPoint, 0.f, 0.f, CIM_Constant), Index);

		NumPoints++;

		// Adjust subsequent points' input keys to make room for the value just added
		for (int I = Index + 1; I < NumPoints; ++I)
		{
			SplineCurves.Position.Points[I].InVal += 1.0f;
			SplineCurves.Rotation.Points[I].InVal += 1.0f;
			SplineCurves.Scale.Points[I].InVal += 1.0f;
			SplineCurves.FOVCurve.Points[I].InVal += 1.0f;
			SplineCurves.HoldTimeCurve.Points[I].InVal += 1.0f;
			SplineCurves.TimeToNextCurve.Points[I].InVal += 1.0f;
			SplineCurves.TimeAtPointCurve.Points[I].InVal += 1.0f;
		}

		if (bLoopPositionOverride)
		{
			LoopPosition += 1.0f;
		}
	}

	if (bUpdateSpline)
	{
		UpdateSpline();
	}
}

void USCCameraSplineComponent::RemoveSplinePoint(int32 Index, bool bUpdateSpline)
{
	int32 NumPoints = SplineCurves.Position.Points.Num();

	if (Index >= 0 && Index < NumPoints)
	{
		SplineCurves.Position.Points.RemoveAt(Index, 1, false);
		SplineCurves.Rotation.Points.RemoveAt(Index, 1, false);
		SplineCurves.Scale.Points.RemoveAt(Index, 1, false);
		SplineCurves.FOVCurve.Points.RemoveAt(Index, 1, false);
		SplineCurves.HoldTimeCurve.Points.RemoveAt(Index, 1, false);
		SplineCurves.TimeToNextCurve.Points.RemoveAt(Index, 1, false);
		SplineCurves.TimeAtPointCurve.Points.RemoveAt(Index, 1, false);
		NumPoints--;

		// Adjust all following spline point input keys to close the gap left by the removed point
		while (Index < NumPoints)
		{
			SplineCurves.Position.Points[Index].InVal -= 1.0f;
			SplineCurves.Rotation.Points[Index].InVal -= 1.0f;
			SplineCurves.Scale.Points[Index].InVal -= 1.0f;
			SplineCurves.FOVCurve.Points[Index].InVal -= 1.0f;
			SplineCurves.HoldTimeCurve.Points[Index].InVal -= 1.0f;
			SplineCurves.TimeToNextCurve.Points[Index].InVal -= 1.0f;
			SplineCurves.TimeAtPointCurve.Points[Index].InVal -= 1.0f;
			Index++;
		}

		if (bLoopPositionOverride)
		{
			LoopPosition -= 1.0f;
		}
	}

	if (bUpdateSpline)
	{
		UpdateSpline();
	}
}

void USCCameraSplineComponent::SetSplinePoints(const TArray<FVector>& Points, ESplineCoordinateSpace::Type CoordinateSpace, bool bUpdateSpline)
{
	const int32 NumPoints = Points.Num();
	SplineCurves.Position.Points.Reset(NumPoints);
	SplineCurves.Rotation.Points.Reset(NumPoints);
	SplineCurves.Scale.Points.Reset(NumPoints);
	SplineCurves.FOVCurve.Points.Reset(NumPoints);
	SplineCurves.HoldTimeCurve.Points.Reset(NumPoints);
	SplineCurves.TimeToNextCurve.Points.Reset(NumPoints);
	SplineCurves.TimeAtPointCurve.Points.Reset(NumPoints);

	float InputKey = 0.0f;
	for (const auto& Point : Points)
	{
		const FVector TransformedPoint = (CoordinateSpace == ESplineCoordinateSpace::World) ?
			GetComponentToWorld().InverseTransformPosition(Point) : Point;

		SplineCurves.Position.Points.Emplace(InputKey, TransformedPoint, FVector::ZeroVector, FVector::ZeroVector, CIM_CurveAuto);
		SplineCurves.Rotation.Points.Emplace(InputKey, FQuat::Identity, FQuat::Identity, FQuat::Identity, CIM_Linear);
		SplineCurves.Scale.Points.Emplace(InputKey, FVector(1.0f), FVector::ZeroVector, FVector::ZeroVector, CIM_CurveAuto);
		SplineCurves.FOVCurve.Points.Emplace(InputKey, 90.f, 0.f, 0.f, CIM_CurveAuto);
		SplineCurves.HoldTimeCurve.Points.Emplace(InputKey, 0.f, 0.f, 0.f, CIM_Constant);
		SplineCurves.TimeToNextCurve.Points.Emplace(InputKey, 1.f, 0.f, 0.f, CIM_Constant);
		SplineCurves.TimeAtPointCurve.Points.Emplace(InputKey, InputKey, 0.f, 0.f, CIM_Constant);

		InputKey += 1.0f;
	}

	bLoopPositionOverride = false;

	if (bUpdateSpline)
	{
		UpdateSpline();
	}
}


void USCCameraSplineComponent::SetLocationAtSplinePoint(int32 PointIndex, const FVector& InLocation, ESplineCoordinateSpace::Type CoordinateSpace, bool bUpdateSpline)
{
	const int32 NumPoints = SplineCurves.Position.Points.Num();

	if ((PointIndex >= 0) && (PointIndex < NumPoints))
	{
		const FVector TransformedLocation = (CoordinateSpace == ESplineCoordinateSpace::World) ?
			GetComponentToWorld().InverseTransformPosition(InLocation) : InLocation;

		SplineCurves.Position.Points[PointIndex].OutVal = TransformedLocation;

		if (bUpdateSpline)
		{
			UpdateSpline();
		}
	}
}


void USCCameraSplineComponent::SetTangentAtSplinePoint(int32 PointIndex, const FVector& InTangent, ESplineCoordinateSpace::Type CoordinateSpace, bool bUpdateSpline)
{
	const int32 NumPoints = SplineCurves.Position.Points.Num();

	if ((PointIndex >= 0) && (PointIndex < NumPoints))
	{
		const FVector TransformedTangent = (CoordinateSpace == ESplineCoordinateSpace::World) ?
			GetComponentToWorld().InverseTransformVector(InTangent) : InTangent;

		SplineCurves.Position.Points[PointIndex].LeaveTangent = TransformedTangent;
		SplineCurves.Position.Points[PointIndex].ArriveTangent = TransformedTangent;
		SplineCurves.Position.Points[PointIndex].InterpMode = CIM_CurveUser;

		if (bUpdateSpline)
		{
			UpdateSpline();
		}
	}
}


void USCCameraSplineComponent::SetTangentsAtSplinePoint(int32 PointIndex, const FVector& InArriveTangent, const FVector& InLeaveTangent, ESplineCoordinateSpace::Type CoordinateSpace, bool bUpdateSpline)
{
	const int32 NumPoints = SplineCurves.Position.Points.Num();

	if ((PointIndex >= 0) && (PointIndex < NumPoints))
	{
		const FVector TransformedArriveTangent = (CoordinateSpace == ESplineCoordinateSpace::World) ?
			GetComponentToWorld().InverseTransformVector(InArriveTangent) : InArriveTangent;
		const FVector TransformedLeaveTangent = (CoordinateSpace == ESplineCoordinateSpace::World) ?
			GetComponentToWorld().InverseTransformVector(InLeaveTangent) : InLeaveTangent;

		SplineCurves.Position.Points[PointIndex].ArriveTangent = TransformedArriveTangent;
		SplineCurves.Position.Points[PointIndex].LeaveTangent = TransformedLeaveTangent;
		SplineCurves.Position.Points[PointIndex].InterpMode = CIM_CurveUser;

		if (bUpdateSpline)
		{
			UpdateSpline();
		}
	}
}


void USCCameraSplineComponent::SetUpVectorAtSplinePoint(int32 PointIndex, const FVector& InUpVector, ESplineCoordinateSpace::Type CoordinateSpace, bool bUpdateSpline)
{
	const int32 NumPoints = SplineCurves.Position.Points.Num();

	if ((PointIndex >= 0) && (PointIndex < NumPoints))
	{
		const FVector TransformedUpVector = (CoordinateSpace == ESplineCoordinateSpace::World) ?
			GetComponentToWorld().InverseTransformVector(InUpVector.GetSafeNormal()) : InUpVector.GetSafeNormal();

		FQuat Quat = FQuat::FindBetween(DefaultUpVector, TransformedUpVector);
		SplineCurves.Rotation.Points[PointIndex].OutVal = Quat;

		if (bUpdateSpline)
		{
			UpdateSpline();
		}
	}
}


ESplinePointType::Type USCCameraSplineComponent::GetSplinePointType(int32 PointIndex) const
{
	if ((PointIndex >= 0) && (PointIndex < SplineCurves.Position.Points.Num()))
	{
		return ConvertInterpCurveModeToSplinePointType(SplineCurves.Position.Points[PointIndex].InterpMode);
	}

	return ESplinePointType::Constant;
}


void USCCameraSplineComponent::SetSplinePointType(int32 PointIndex, ESplinePointType::Type Type, bool bUpdateSpline)
{
	if ((PointIndex >= 0) && (PointIndex < SplineCurves.Position.Points.Num()))
	{
		SplineCurves.Position.Points[PointIndex].InterpMode = ConvertSplinePointTypeToInterpCurveMode(Type);
		if (bUpdateSpline)
		{
			UpdateSpline();
		}
	}
}


int32 USCCameraSplineComponent::GetNumberOfSplinePoints() const
{
	// No longer returns an imaginary extra endpoint if closed loop is set
	const int32 NumPoints = SplineCurves.Position.Points.Num();
	return NumPoints;
}

FVector USCCameraSplineComponent::GetLocationAtSplinePoint(int32 PointIndex, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const int32 NumPoints = SplineCurves.Position.Points.Num();
	const int32 ClampedIndex = (bClosedLoop && PointIndex >= NumPoints) ? 0 : FMath::Clamp(PointIndex, 0, NumPoints - 1);
	const FVector Location = SplineCurves.Position.Points[ClampedIndex].OutVal;
	return (CoordinateSpace == ESplineCoordinateSpace::World) ? GetComponentToWorld().TransformPosition(Location) : Location;
}


FVector USCCameraSplineComponent::GetDirectionAtSplinePoint(int32 PointIndex, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const int32 NumPoints = SplineCurves.Position.Points.Num();
	const int32 ClampedIndex = (bClosedLoop && PointIndex >= NumPoints) ? 0 : FMath::Clamp(PointIndex, 0, NumPoints - 1);
	const FVector Direction = SplineCurves.Position.Points[ClampedIndex].LeaveTangent.GetSafeNormal();
	return (CoordinateSpace == ESplineCoordinateSpace::World) ? GetComponentToWorld().TransformVector(Direction) : Direction;
}


FVector USCCameraSplineComponent::GetTangentAtSplinePoint(int32 PointIndex, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const int32 NumPoints = SplineCurves.Position.Points.Num();
	const int32 ClampedIndex = (bClosedLoop && PointIndex >= NumPoints) ? 0 : FMath::Clamp(PointIndex, 0, NumPoints - 1);
	const FVector Direction = SplineCurves.Position.Points[ClampedIndex].LeaveTangent;
	return (CoordinateSpace == ESplineCoordinateSpace::World) ? GetComponentToWorld().TransformVector(Direction) : Direction;
}


FVector USCCameraSplineComponent::GetArriveTangentAtSplinePoint(int32 PointIndex, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const int32 NumPoints = SplineCurves.Position.Points.Num();
	const int32 ClampedIndex = (bClosedLoop && PointIndex >= NumPoints) ? 0 : FMath::Clamp(PointIndex, 0, NumPoints - 1);
	const FVector Direction = SplineCurves.Position.Points[ClampedIndex].ArriveTangent;
	return (CoordinateSpace == ESplineCoordinateSpace::World) ? GetComponentToWorld().TransformVector(Direction) : Direction;
}


FVector USCCameraSplineComponent::GetLeaveTangentAtSplinePoint(int32 PointIndex, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const int32 NumPoints = SplineCurves.Position.Points.Num();
	const int32 ClampedIndex = (bClosedLoop && PointIndex >= NumPoints) ? 0 : FMath::Clamp(PointIndex, 0, NumPoints - 1);
	const FVector Direction = SplineCurves.Position.Points[ClampedIndex].LeaveTangent;
	return (CoordinateSpace == ESplineCoordinateSpace::World) ? GetComponentToWorld().TransformVector(Direction) : Direction;
}


FQuat USCCameraSplineComponent::GetQuaternionAtSplinePoint(int32 PointIndex, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const int32 NumPoints = SplineCurves.Position.Points.Num();
	const int32 ClampedIndex = (bClosedLoop && PointIndex >= NumPoints) ? 0 : FMath::Clamp(PointIndex, 0, NumPoints - 1);
	return GetQuaternionAtSplineInputKey(SplineCurves.Rotation.Points[ClampedIndex].InVal, CoordinateSpace);
}


FRotator USCCameraSplineComponent::GetRotationAtSplinePoint(int32 PointIndex, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const int32 NumPoints = SplineCurves.Position.Points.Num();
	const int32 ClampedIndex = (bClosedLoop && PointIndex >= NumPoints) ? 0 : FMath::Clamp(PointIndex, 0, NumPoints - 1);
	return GetRotationAtSplineInputKey(SplineCurves.Rotation.Points[ClampedIndex].InVal, CoordinateSpace);
}


FVector USCCameraSplineComponent::GetUpVectorAtSplinePoint(int32 PointIndex, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const int32 NumPoints = SplineCurves.Position.Points.Num();
	const int32 ClampedIndex = (bClosedLoop && PointIndex >= NumPoints) ? 0 : FMath::Clamp(PointIndex, 0, NumPoints - 1);
	return GetUpVectorAtSplineInputKey(SplineCurves.Rotation.Points[ClampedIndex].InVal, CoordinateSpace);
}


FVector USCCameraSplineComponent::GetRightVectorAtSplinePoint(int32 PointIndex, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const int32 NumPoints = SplineCurves.Position.Points.Num();
	const int32 ClampedIndex = (bClosedLoop && PointIndex >= NumPoints) ? 0 : FMath::Clamp(PointIndex, 0, NumPoints - 1);
	return GetRightVectorAtSplineInputKey(SplineCurves.Rotation.Points[ClampedIndex].InVal, CoordinateSpace);
}


float USCCameraSplineComponent::GetRollAtSplinePoint(int32 PointIndex, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const int32 NumPoints = SplineCurves.Position.Points.Num();
	const int32 ClampedIndex = (bClosedLoop && PointIndex >= NumPoints) ? 0 : FMath::Clamp(PointIndex, 0, NumPoints - 1);
	return GetRollAtSplineInputKey(SplineCurves.Rotation.Points[ClampedIndex].InVal, CoordinateSpace);
}


FVector USCCameraSplineComponent::GetScaleAtSplinePoint(int32 PointIndex) const
{
	const int32 NumPoints = SplineCurves.Position.Points.Num();
	const int32 ClampedIndex = (bClosedLoop && PointIndex >= NumPoints) ? 0 : FMath::Clamp(PointIndex, 0, NumPoints - 1);
	return SplineCurves.Scale.Points[ClampedIndex].OutVal;
}

float USCCameraSplineComponent::GetFOVAtSplinePoint(int32 PointIndex) const
{
	const int32 NumPoints = SplineCurves.Position.Points.Num();
	const int32 ClampedIndex = (bClosedLoop && PointIndex >= NumPoints) ? 0 : FMath::Clamp(PointIndex, 0, NumPoints - 1);
	return SplineCurves.FOVCurve.Points[ClampedIndex].OutVal;
}

float USCCameraSplineComponent::GetHoldTimeAtSplinePoint(int32 PointIndex) const
{
	const int32 NumPoints = SplineCurves.Position.Points.Num();
	const int32 ClampedIndex = (bClosedLoop && PointIndex >= NumPoints) ? 0 : FMath::Clamp(PointIndex, 0, NumPoints - 1);
	return SplineCurves.HoldTimeCurve.Points[ClampedIndex].OutVal;
}

float USCCameraSplineComponent::GetTimeToNextAtSplinePoint(int32 PointIndex) const
{
	const int32 NumPoints = SplineCurves.Position.Points.Num();
	const int32 ClampedIndex = (bClosedLoop && PointIndex >= NumPoints) ? 0 : FMath::Clamp(PointIndex, 0, NumPoints - 1);
	return SplineCurves.TimeToNextCurve.Points[ClampedIndex].OutVal;
}

FTransform USCCameraSplineComponent::GetTransformAtSplinePoint(int32 PointIndex, ESplineCoordinateSpace::Type CoordinateSpace, bool bUseScale) const
{
	const int32 NumPoints = SplineCurves.Position.Points.Num();
	const int32 ClampedIndex = (bClosedLoop && PointIndex >= NumPoints) ? 0 : FMath::Clamp(PointIndex, 0, NumPoints - 1);
	return GetTransformAtSplineInputKey(SplineCurves.Rotation.Points[ClampedIndex].InVal, CoordinateSpace, bUseScale);
}


void USCCameraSplineComponent::GetLocationAndTangentAtSplinePoint(int32 PointIndex, FVector& Location, FVector& Tangent, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const int32 NumPoints = SplineCurves.Position.Points.Num();
	const int32 ClampedIndex = (bClosedLoop && PointIndex >= NumPoints) ? 0 : FMath::Clamp(PointIndex, 0, NumPoints - 1);
	float InputKey = SplineCurves.Rotation.Points[ClampedIndex].InVal;
	Location = GetLocationAtSplineInputKey(InputKey, CoordinateSpace);
	Tangent = GetTangentAtSplineInputKey(InputKey, CoordinateSpace);
}


float USCCameraSplineComponent::GetDistanceAlongSplineAtSplinePoint(int32 PointIndex) const
{
	const int32 NumPoints = SplineCurves.Position.Points.Num();
	const int32 NumSegments = bClosedLoop ? NumPoints : NumPoints - 1;

	if ((PointIndex >= 0) && (PointIndex < NumSegments + 1))
	{
		return SplineCurves.ReparamTable.Points[PointIndex * ReparamStepsPerSegment].InVal;
	}

	return 0.0f;
}


float USCCameraSplineComponent::GetSplineLength() const
{
	const int32 NumPoints = SplineCurves.ReparamTable.Points.Num();

	// This is given by the input of the last entry in the remap table
	if (NumPoints > 0)
	{
		return SplineCurves.ReparamTable.Points.Last().InVal;
	}

	return 0.0f;
}


void USCCameraSplineComponent::SetDefaultUpVector(const FVector& UpVector, ESplineCoordinateSpace::Type CoordinateSpace)
{
	if (CoordinateSpace == ESplineCoordinateSpace::World)
	{
		DefaultUpVector = GetComponentToWorld().InverseTransformVector(UpVector);
	}
	else
	{
		DefaultUpVector = UpVector;
	}

	UpdateSpline();
}


FVector USCCameraSplineComponent::GetDefaultUpVector(ESplineCoordinateSpace::Type CoordinateSpace) const
{
	if (CoordinateSpace == ESplineCoordinateSpace::World)
	{
		return GetComponentToWorld().TransformVector(DefaultUpVector);
	}
	else
	{
		return DefaultUpVector;
	}
}


float USCCameraSplineComponent::GetInputKeyAtDistanceAlongSpline(float Distance) const
{
	const int32 NumPoints = SplineCurves.Position.Points.Num();

	if (NumPoints < 2)
	{
		return 0.0f;
	}

	const float TimeMultiplier = Duration / (bClosedLoop ? NumPoints : (NumPoints - 1.0f));
	return SplineCurves.ReparamTable.Eval(Distance, 0.0f) * TimeMultiplier;
}


FVector USCCameraSplineComponent::GetLocationAtDistanceAlongSpline(float Distance, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const float Param = SplineCurves.ReparamTable.Eval(Distance, 0.0f);
	return GetLocationAtSplineInputKey(Param, CoordinateSpace);
}


FVector USCCameraSplineComponent::GetTangentAtDistanceAlongSpline(float Distance, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const float Param = SplineCurves.ReparamTable.Eval(Distance, 0.0f);
	return GetTangentAtSplineInputKey(Param, CoordinateSpace);
}


FVector USCCameraSplineComponent::GetDirectionAtDistanceAlongSpline(float Distance, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const float Param = SplineCurves.ReparamTable.Eval(Distance, 0.0f);
	return GetDirectionAtSplineInputKey(Param, CoordinateSpace);
}


FQuat USCCameraSplineComponent::GetQuaternionAtDistanceAlongSpline(float Distance, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const float Param = SplineCurves.ReparamTable.Eval(Distance, 0.0f);
	return GetQuaternionAtSplineInputKey(Param, CoordinateSpace);
}


FRotator USCCameraSplineComponent::GetRotationAtDistanceAlongSpline(float Distance, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const float Param = SplineCurves.ReparamTable.Eval(Distance, 0.0f);
	return GetRotationAtSplineInputKey(Param, CoordinateSpace);
}


FVector USCCameraSplineComponent::GetUpVectorAtDistanceAlongSpline(float Distance, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const float Param = SplineCurves.ReparamTable.Eval(Distance, 0.0f);
	return GetUpVectorAtSplineInputKey(Param, CoordinateSpace);
}


FVector USCCameraSplineComponent::GetRightVectorAtDistanceAlongSpline(float Distance, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const float Param = SplineCurves.ReparamTable.Eval(Distance, 0.0f);
	return GetRightVectorAtSplineInputKey(Param, CoordinateSpace);
}


float USCCameraSplineComponent::GetRollAtDistanceAlongSpline(float Distance, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const float Param = SplineCurves.ReparamTable.Eval(Distance, 0.0f);
	return GetRollAtSplineInputKey(Param, CoordinateSpace);
}


FVector USCCameraSplineComponent::GetScaleAtDistanceAlongSpline(float Distance) const
{
	const float Param = SplineCurves.ReparamTable.Eval(Distance, 0.0f);
	return GetScaleAtSplineInputKey(Param);
}

float USCCameraSplineComponent::GetFOVAtDistanceAlongSpline(float Distance) const
{
	const float Param = SplineCurves.ReparamTable.Eval(Distance, 0.0f);
	return GetFOVAtSplineInputKey(Param);
}

float USCCameraSplineComponent::GetHoldTimeAtDistanceAlongSpline(float Distance) const
{
	const float Param = SplineCurves.ReparamTable.Eval(Distance, 0.0f);
	return GetHoldTimeAtSplineInputKey(Param);
}

float USCCameraSplineComponent::GetTimeToNextAtDistanceAlongSpline(float Distance) const
{
	const float Param = SplineCurves.ReparamTable.Eval(Distance, 0.0f);
	return GetTimeToNextAtSplineInputKey(Param);
}

FTransform USCCameraSplineComponent::GetTransformAtDistanceAlongSpline(float Distance, ESplineCoordinateSpace::Type CoordinateSpace, bool bUseScale) const
{
	const float Param = SplineCurves.ReparamTable.Eval(Distance, 0.0f);
	return GetTransformAtSplineInputKey(Param, CoordinateSpace, bUseScale);
}


FVector USCCameraSplineComponent::GetLocationAtTime(float Time, ESplineCoordinateSpace::Type CoordinateSpace, bool bUseConstantVelocity) const
{
	if (Duration == 0.0f)
	{
		return FVector::ZeroVector;
	}

	if (bUseConstantVelocity)
	{
		return GetLocationAtDistanceAlongSpline(Time / Duration * GetSplineLength(), CoordinateSpace);
	}
	else
	{
		float InputKey = 0;
		while (Time > 0)
		{
			const float HoldTime = GetHoldTimeAtSplineInputKey(InputKey);
			const float TimeToNext = FMath::Max(GetTimeToNextAtSplineInputKey(InputKey), 0.001f);
			if((Time -= HoldTime) <= 0.f)
			{
				return GetLocationAtSplineInputKey(InputKey, CoordinateSpace);
			}

			if (Time >= TimeToNext)
			{
				Time -= TimeToNext;
			}
			else
			{
				InputKey += Time / TimeToNext;
				return GetLocationAtSplineInputKey(InputKey, CoordinateSpace);
			}

			++InputKey;
		}
	}

	return FVector::ZeroVector;
}


FVector USCCameraSplineComponent::GetDirectionAtTime(float Time, ESplineCoordinateSpace::Type CoordinateSpace, bool bUseConstantVelocity) const
{
	if (Duration == 0.0f)
	{
		return FVector::ZeroVector;
	}

	if (bUseConstantVelocity)
	{
		return GetDirectionAtDistanceAlongSpline(Time / Duration * GetSplineLength(), CoordinateSpace);
	}
	else
	{
		float InputKey = 0;
		while (Time > 0)
		{
			const float HoldTime = GetHoldTimeAtSplineInputKey(InputKey);
			const float TimeToNext = FMath::Max(GetTimeToNextAtSplineInputKey(InputKey), 0.001f);
			if((Time -= HoldTime) <= 0.f)
			{
				return GetDirectionAtSplineInputKey(InputKey, CoordinateSpace);
			}

			if (Time >= TimeToNext)
			{
				Time -= TimeToNext;
			}
			else
			{
				InputKey += Time / TimeToNext;
				return GetDirectionAtSplineInputKey(InputKey, CoordinateSpace);
			}

			++InputKey;
		}
	}

	return FVector::ZeroVector;
}


FVector USCCameraSplineComponent::GetTangentAtTime(float Time, ESplineCoordinateSpace::Type CoordinateSpace, bool bUseConstantVelocity) const
{
	if (Duration == 0.0f)
	{
		return FVector::ZeroVector;
	}

	if (bUseConstantVelocity)
	{
		return GetTangentAtDistanceAlongSpline(Time / Duration * GetSplineLength(), CoordinateSpace);
	}
	else
	{
		float InputKey = 0;
		while (Time > 0)
		{
			const float HoldTime = GetHoldTimeAtSplineInputKey(InputKey);
			const float TimeToNext = FMath::Max(GetTimeToNextAtSplineInputKey(InputKey), 0.001f);
			if((Time -= HoldTime) <= 0.f)
			{
				return GetTangentAtSplineInputKey(InputKey, CoordinateSpace);
			}

			if (Time >= TimeToNext)
			{
				Time -= TimeToNext;
			}
			else
			{
				InputKey += Time / TimeToNext;
				return GetTangentAtSplineInputKey(InputKey, CoordinateSpace);
			}

			++InputKey;
		}
	}

	return FVector::ZeroVector;
}


FRotator USCCameraSplineComponent::GetRotationAtTime(float Time, ESplineCoordinateSpace::Type CoordinateSpace, bool bUseConstantVelocity) const
{
	if (Duration == 0.0f)
	{
		return FRotator::ZeroRotator;
	}

	if (bUseConstantVelocity)
	{
		return GetRotationAtDistanceAlongSpline(Time / Duration * GetSplineLength(), CoordinateSpace);
	}
	else
	{
		float InputKey = 0;
		while (Time > 0)
		{
			const float HoldTime = GetHoldTimeAtSplineInputKey(InputKey);
			const float TimeToNext = FMath::Max(GetTimeToNextAtSplineInputKey(InputKey), 0.001f);
			if((Time -= HoldTime) <= 0.f)
			{
				return GetRotationAtSplineInputKey(InputKey, CoordinateSpace);
			}

			if (Time >= TimeToNext)
			{
				Time -= TimeToNext;
			}
			else
			{
				InputKey += Time / TimeToNext;
				return GetRotationAtSplineInputKey(InputKey, CoordinateSpace);
			}

			++InputKey;
		}
	}

	return FRotator::ZeroRotator;
}


FQuat USCCameraSplineComponent::GetQuaternionAtTime(float Time, ESplineCoordinateSpace::Type CoordinateSpace, bool bUseConstantVelocity) const
{
	if (Duration == 0.0f)
	{
		return FQuat::Identity;
	}

	if (bUseConstantVelocity)
	{
		return GetQuaternionAtDistanceAlongSpline(Time / Duration * GetSplineLength(), CoordinateSpace);
	}
	else
	{
		float InputKey = 0;
		while (Time > 0)
		{
			const float HoldTime = GetHoldTimeAtSplineInputKey(InputKey);
			const float TimeToNext = FMath::Max(GetTimeToNextAtSplineInputKey(InputKey), 0.001f);
			if((Time -= HoldTime) <= 0.f)
			{
				return GetQuaternionAtSplineInputKey(InputKey, CoordinateSpace);
			}

			if (Time >= TimeToNext)
			{
				Time -= TimeToNext;
			}
			else
			{
				InputKey += Time / TimeToNext;
				return GetQuaternionAtSplineInputKey(InputKey, CoordinateSpace);
			}

			++InputKey;
		}
	}

	return FQuat::Identity;
}


FVector USCCameraSplineComponent::GetUpVectorAtTime(float Time, ESplineCoordinateSpace::Type CoordinateSpace, bool bUseConstantVelocity) const
{
	if (Duration == 0.0f)
	{
		return FVector::ZeroVector;
	}

	if (bUseConstantVelocity)
	{
		return GetUpVectorAtDistanceAlongSpline(Time / Duration * GetSplineLength(), CoordinateSpace);
	}
	else
	{
		float InputKey = 0;
		while (Time > 0)
		{
			const float HoldTime = GetHoldTimeAtSplineInputKey(InputKey);
			const float TimeToNext = FMath::Max(GetTimeToNextAtSplineInputKey(InputKey), 0.001f);
			if((Time -= HoldTime) <= 0.f)
			{
				return GetUpVectorAtSplineInputKey(InputKey, CoordinateSpace);
			}

			if (Time >= TimeToNext)
			{
				Time -= TimeToNext;
			}
			else
			{
				InputKey += Time / TimeToNext;
				return GetUpVectorAtSplineInputKey(InputKey, CoordinateSpace);
			}

			++InputKey;
		}
	}

	return FVector::ZeroVector;
}


FVector USCCameraSplineComponent::GetRightVectorAtTime(float Time, ESplineCoordinateSpace::Type CoordinateSpace, bool bUseConstantVelocity) const
{
	if (Duration == 0.0f)
	{
		return FVector::ZeroVector;
	}

	if (bUseConstantVelocity)
	{
		return GetRightVectorAtDistanceAlongSpline(Time / Duration * GetSplineLength(), CoordinateSpace);
	}
	else
	{
		float InputKey = 0;
		while (Time > 0)
		{
			const float HoldTime = GetHoldTimeAtSplineInputKey(InputKey);
			const float TimeToNext = FMath::Max(GetTimeToNextAtSplineInputKey(InputKey), 0.001f);
			if((Time -= HoldTime) <= 0.f)
			{
				return GetRightVectorAtSplineInputKey(InputKey, CoordinateSpace);
			}

			if (Time >= TimeToNext)
			{
				Time -= TimeToNext;
			}
			else
			{
				InputKey += Time / TimeToNext;
				return GetRightVectorAtSplineInputKey(InputKey, CoordinateSpace);
			}

			++InputKey;
		}
	}

	return FVector::ZeroVector;
}


float USCCameraSplineComponent::GetRollAtTime(float Time, ESplineCoordinateSpace::Type CoordinateSpace, bool bUseConstantVelocity) const
{
	if (Duration == 0.0f)
	{
		return 0.0f;
	}

	if (bUseConstantVelocity)
	{
		return GetRollAtDistanceAlongSpline(Time / Duration * GetSplineLength(), CoordinateSpace);
	}
	else
	{
		float InputKey = 0;
		while (Time > 0)
		{
			const float HoldTime = GetHoldTimeAtSplineInputKey(InputKey);
			const float TimeToNext = FMath::Max(GetTimeToNextAtSplineInputKey(InputKey), 0.001f);
			if((Time -= HoldTime) <= 0.f)
			{
				return GetRollAtSplineInputKey(InputKey, CoordinateSpace);
			}

			if (Time >= TimeToNext)
			{
				Time -= TimeToNext;
			}
			else
			{
				InputKey += Time / TimeToNext;
				return GetRollAtSplineInputKey(InputKey, CoordinateSpace);
			}

			++InputKey;
		}
	}

	return 0.0f;
}


FTransform USCCameraSplineComponent::GetTransformAtTime(float Time, ESplineCoordinateSpace::Type CoordinateSpace, bool bUseConstantVelocity, bool bUseScale) const
{
	if (Duration == 0.0f)
	{
		return FTransform::Identity;
	}


	if (bUseConstantVelocity)
	{
		return GetTransformAtDistanceAlongSpline(Time / Duration * GetSplineLength(), CoordinateSpace, bUseScale);
	}
	else
	{
		float InputKey = 0;
		while (Time > 0)
		{
			const float HoldTime = GetHoldTimeAtSplineInputKey(InputKey);
			const float TimeToNext = FMath::Max(GetTimeToNextAtSplineInputKey(InputKey), 0.001f);
			if((Time -= HoldTime) <= 0.f)
			{
				return GetTransformAtSplineInputKey(InputKey, CoordinateSpace, bUseScale);
			}

			if (Time >= TimeToNext)
			{
				Time -= TimeToNext;
			}
			else
			{
				InputKey += Time / TimeToNext;
				return GetTransformAtSplineInputKey(InputKey, CoordinateSpace, bUseScale);
			}

			++InputKey;
		}
	}

	return FTransform::Identity;
}


FVector USCCameraSplineComponent::GetScaleAtTime(float Time, bool bUseConstantVelocity) const
{
	if (Duration == 0.0f)
	{
		return FVector(1.0f);
	}

	float InputKey = 0;
	while (Time > 0)
	{
		const float HoldTime = GetHoldTimeAtSplineInputKey(InputKey);
		const float TimeToNext = FMath::Max(GetTimeToNextAtSplineInputKey(InputKey), 0.001f);
		if((Time -= HoldTime) <= 0.f)
		{
			return GetScaleAtSplineInputKey(InputKey);
		}

		if (Time >= TimeToNext)
		{
			Time -= TimeToNext;
		}
		else
		{
			InputKey += Time / TimeToNext;
			return GetScaleAtSplineInputKey(InputKey);
		}

		++InputKey;
	}

	return FVector(1.0f);
}

float USCCameraSplineComponent::GetFOVAtTime(float Time) const
{
	if (Duration == 0.0f)
	{
		return 90.0f;
	}

	float InputKey = 0;
	while (Time > 0)
	{
		const float HoldTime = GetHoldTimeAtSplineInputKey(InputKey);
		const float TimeToNext = FMath::Max(GetTimeToNextAtSplineInputKey(InputKey), 0.001f);
		if((Time -= HoldTime) <= 0.f)
		{
			return GetFOVAtSplineInputKey(InputKey);
		}

		if (Time >= TimeToNext)
		{
			Time -= TimeToNext;
		}
		else
		{
			InputKey += Time / TimeToNext;
			return GetFOVAtSplineInputKey(InputKey);
		}

		++InputKey;
	}

	return 0.f;
}

float USCCameraSplineComponent::GetHoldTimeAtTime(float Time) const
{
	if (Duration == 0.0f)
	{
		return 0.0f;
	}

	float InputKey = 0;
	while (Time > 0)
	{
		const float HoldTime = GetHoldTimeAtSplineInputKey(InputKey);
		const float TimeToNext = FMath::Max(GetTimeToNextAtSplineInputKey(InputKey), 0.001f);
		if((Time -= HoldTime) <= 0.f)
		{
			return GetHoldTimeAtSplineInputKey(InputKey);
		}

		if (Time >= TimeToNext)
		{
			Time -= TimeToNext;
		}
		else
		{
			//return the input key time, we don't want to interpolate these values.
			//InputKey += Time / TimeToNext;
			return GetHoldTimeAtSplineInputKey(InputKey);
		}

		++InputKey;
	}

	return 0.f;
}

float USCCameraSplineComponent::GetTimeToNextAtTime(float Time) const
{
	if (Duration == 0.0f)
	{
		return 0.0f;
	}

	float InputKey = 0;
	while (Time > 0)
	{
		const float HoldTime = GetHoldTimeAtSplineInputKey(InputKey);
		const float TimeToNext = FMath::Max(GetTimeToNextAtSplineInputKey(InputKey), 0.001f);
		if((Time -= HoldTime) <= 0.f)
		{
			return GetTimeToNextAtSplineInputKey(InputKey);
		}

		if (Time >= TimeToNext)
		{
			Time -= TimeToNext;
		}
		else
		{
			//return the input key time, we don't want to interpolate these values.
			//InputKey += Time / TimeToNext;
			return GetTimeToNextAtSplineInputKey(InputKey);
		}

		++InputKey;
	}

	return 0.f;
}


float USCCameraSplineComponent::FindInputKeyClosestToWorldLocation(const FVector& WorldLocation) const
{
	const FVector LocalLocation = GetComponentToWorld().InverseTransformPosition(WorldLocation);
	float Dummy;
	return SplineCurves.Position.InaccurateFindNearest(LocalLocation, Dummy);
}


FVector USCCameraSplineComponent::FindLocationClosestToWorldLocation(const FVector& WorldLocation, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const float Param = FindInputKeyClosestToWorldLocation(WorldLocation);
	return GetLocationAtSplineInputKey(Param, CoordinateSpace);
}


FVector USCCameraSplineComponent::FindDirectionClosestToWorldLocation(const FVector& WorldLocation, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const float Param = FindInputKeyClosestToWorldLocation(WorldLocation);
	return GetDirectionAtSplineInputKey(Param, CoordinateSpace);
}


FVector USCCameraSplineComponent::FindTangentClosestToWorldLocation(const FVector& WorldLocation, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const float Param = FindInputKeyClosestToWorldLocation(WorldLocation);
	return GetTangentAtSplineInputKey(Param, CoordinateSpace);
}


FQuat USCCameraSplineComponent::FindQuaternionClosestToWorldLocation(const FVector& WorldLocation, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const float Param = FindInputKeyClosestToWorldLocation(WorldLocation);
	return GetQuaternionAtSplineInputKey(Param, CoordinateSpace);
}


FRotator USCCameraSplineComponent::FindRotationClosestToWorldLocation(const FVector& WorldLocation, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const float Param = FindInputKeyClosestToWorldLocation(WorldLocation);
	return GetRotationAtSplineInputKey(Param, CoordinateSpace);
}


FVector USCCameraSplineComponent::FindUpVectorClosestToWorldLocation(const FVector& WorldLocation, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const float Param = FindInputKeyClosestToWorldLocation(WorldLocation);
	return GetUpVectorAtSplineInputKey(Param, CoordinateSpace);
}


FVector USCCameraSplineComponent::FindRightVectorClosestToWorldLocation(const FVector& WorldLocation, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const float Param = FindInputKeyClosestToWorldLocation(WorldLocation);
	return GetRightVectorAtSplineInputKey(Param, CoordinateSpace);
}


float USCCameraSplineComponent::FindRollClosestToWorldLocation(const FVector& WorldLocation, ESplineCoordinateSpace::Type CoordinateSpace) const
{
	const float Param = FindInputKeyClosestToWorldLocation(WorldLocation);
	return GetRollAtSplineInputKey(Param, CoordinateSpace);
}


FVector USCCameraSplineComponent::FindScaleClosestToWorldLocation(const FVector& WorldLocation) const
{
	const float Param = FindInputKeyClosestToWorldLocation(WorldLocation);
	return GetScaleAtSplineInputKey(Param);
}


FTransform USCCameraSplineComponent::FindTransformClosestToWorldLocation(const FVector& WorldLocation, ESplineCoordinateSpace::Type CoordinateSpace, bool bUseScale) const
{
	const float Param = FindInputKeyClosestToWorldLocation(WorldLocation);
	return GetTransformAtSplineInputKey(Param, CoordinateSpace, bUseScale);
}

#if !UE_BUILD_SHIPPING
FPrimitiveSceneProxy* USCCameraSplineComponent::CreateSceneProxy()
{
	if (!bDrawDebug)
	{
		return Super::CreateSceneProxy();
	}

	class FSplineSceneProxy : public FPrimitiveSceneProxy
	{
	public:

		FSplineSceneProxy(const USCCameraSplineComponent* InComponent)
			: FPrimitiveSceneProxy(InComponent)
			, bDrawDebug(InComponent->bDrawDebug)
			, SplineInfo(InComponent->SplineCurves.Position)
#if WITH_EDITORONLY_DATA
			, LineColor(InComponent->EditorUnselectedSplineSegmentColor)
#else
			, LineColor(FLinearColor::White)
#endif
		{}

		virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
		{
			QUICK_SCOPE_CYCLE_COUNTER(STAT_SplineSceneProxy_GetDynamicMeshElements);

			if (IsSelected())
			{
				return;
			}

			for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
			{
				if (VisibilityMap & (1 << ViewIndex))
				{
					const FSceneView* View = Views[ViewIndex];
					FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

					const FMatrix& LocalToWorld = GetLocalToWorld();

					// Taking into account the min and maximum drawing distance
					const float DistanceSqr = (View->ViewMatrices.GetViewOrigin() - LocalToWorld.GetOrigin()).SizeSquared();
					if (DistanceSqr < FMath::Square(GetMinDrawDistance()) || DistanceSqr > FMath::Square(GetMaxDrawDistance()))
					{
						continue;
					}

					const int32 GrabHandleSize = 6;
					FVector OldKeyPos(0);

					const int32 NumPoints = SplineInfo.Points.Num();
					const int32 NumSegments = SplineInfo.bIsLooped ? NumPoints : NumPoints - 1;
					for (int32 KeyIdx = 0; KeyIdx < NumSegments + 1; KeyIdx++)
					{
						const FVector NewKeyPos = LocalToWorld.TransformPosition(SplineInfo.Eval(static_cast<float>(KeyIdx), FVector(0)));

						// Draw the keypoint
						if (KeyIdx < NumPoints)
						{
							PDI->DrawPoint(NewKeyPos, LineColor, GrabHandleSize, SDPG_World);
						}

						// If not the first keypoint, draw a line to the previous keypoint.
						if (KeyIdx > 0)
						{
							// For constant interpolation - don't draw ticks - just draw dotted line.
							if (SplineInfo.Points[KeyIdx - 1].InterpMode == CIM_Constant)
							{
								// Calculate dash length according to size on screen
								const float StartW = View->WorldToScreen(OldKeyPos).W;
								const float EndW = View->WorldToScreen(NewKeyPos).W;

								const float WLimit = 10.0f;
								if (StartW > WLimit || EndW > WLimit)
								{
									const float Scale = 0.03f;
									DrawDashedLine(PDI, OldKeyPos, NewKeyPos, LineColor, FMath::Max(StartW, EndW) * Scale, SDPG_World);
								}
							}
							else
							{
								// Find position on first keyframe.
								FVector OldPos = OldKeyPos;

								// Then draw a line for each substep.
								const int32 NumSteps = 20;

								for (int32 StepIdx = 1; StepIdx <= NumSteps; StepIdx++)
								{
									const float Key = (KeyIdx - 1) + (StepIdx / static_cast<float>(NumSteps));
									const FVector NewPos = LocalToWorld.TransformPosition(SplineInfo.Eval(Key, FVector(0)));
									PDI->DrawLine(OldPos, NewPos, LineColor, SDPG_World);
									OldPos = NewPos;
								}
							}
						}

						OldKeyPos = NewKeyPos;
					}
				}
			}
		}

		virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
		{
			FPrimitiveViewRelevance Result;
			Result.bDrawRelevance = bDrawDebug && !IsSelected() && IsShown(View) && View->Family->EngineShowFlags.Splines;
			Result.bDynamicRelevance = true;
			Result.bShadowRelevance = IsShadowCast(View);
			Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
			return Result;
		}

		virtual uint32 GetMemoryFootprint(void) const override { return sizeof *this + GetAllocatedSize(); }
		uint32 GetAllocatedSize(void) const { return FPrimitiveSceneProxy::GetAllocatedSize(); }

	private:
		bool bDrawDebug;
		FInterpCurveVector SplineInfo;
		FLinearColor LineColor;
	};

	return new FSplineSceneProxy(this);
}


FBoxSphereBounds USCCameraSplineComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	if (!bDrawDebug)
	{
		// Do as little as possible if not rendering anything
		return Super::CalcBounds(LocalToWorld);
	}

	const int32 NumPoints = SplineCurves.Position.Points.Num();
	const int32 NumSegments = bClosedLoop ? NumPoints : NumPoints - 1;

	FVector Min(WORLD_MAX);
	FVector Max(-WORLD_MAX);
	for (int32 Index = 0; Index < NumSegments; Index++)
	{
		const bool bLoopSegment = (Index == NumPoints - 1);
		const int32 NextIndex = bLoopSegment ? 0 : Index + 1;
		const FInterpCurvePoint<FVector>& ThisInterpPoint = SplineCurves.Position.Points[Index];
		FInterpCurvePoint<FVector> NextInterpPoint = SplineCurves.Position.Points[NextIndex];
		if (bLoopSegment)
		{
			NextInterpPoint.InVal = ThisInterpPoint.InVal + SplineCurves.Position.LoopKeyOffset;
		}

		CurveVectorFindIntervalBounds(ThisInterpPoint, NextInterpPoint, Min, Max);
	}

	return FBoxSphereBounds(FBox(Min, Max).TransformBy(LocalToWorld));
}
#endif


/** Used to store spline data during RerunConstructionScripts */
class FSCSplineInstanceData : public FSceneComponentInstanceData
{
public:
	explicit FSCSplineInstanceData(const USCCameraSplineComponent* SourceComponent)
		: FSceneComponentInstanceData(SourceComponent)
	{
	}

	virtual void ApplyToComponent(UActorComponent* Component, const ECacheApplyPhase CacheApplyPhase) override
	{
		FSceneComponentInstanceData::ApplyToComponent(Component, CacheApplyPhase);
		CastChecked<USCCameraSplineComponent>(Component)->ApplyComponentInstanceData(this, (CacheApplyPhase == ECacheApplyPhase::PostUserConstructionScript));
	}

	FSCCameraSplineCurves SplineCurves;
	FSCCameraSplineCurves SplineCurvesPreUCS;
	bool bSplineHasBeenEdited;
};


FActorComponentInstanceData* USCCameraSplineComponent::GetComponentInstanceData() const
{
	FSCSplineInstanceData* SplineInstanceData = new FSCSplineInstanceData(this);
	if (bSplineHasBeenEdited)
	{
		SplineInstanceData->SplineCurves = SplineCurves;
	}
	SplineInstanceData->bSplineHasBeenEdited = bSplineHasBeenEdited;

	return SplineInstanceData;
}


void USCCameraSplineComponent::ApplyComponentInstanceData(FSCSplineInstanceData* SplineInstanceData, const bool bPostUCS)
{
	check(SplineInstanceData);

	if (bPostUCS)
	{
		if (bInputSplinePointsToConstructionScript)
		{
			// Don't reapply the saved state after the UCS has run if we are inputting the points to it.
			// This allows the UCS to work on the edited points and make its own changes.
			return;
		}
		else
		{
			bModifiedByConstructionScript = (SplineInstanceData->SplineCurvesPreUCS != SplineCurves);

			// If we are restoring the saved state, unmark the SplineCurves property as 'modified'.
			// We don't want to consider that these changes have been made through the UCS.
			TArray<UProperty*> Properties;
			Properties.Emplace(FindField<UProperty>(USCCameraSplineComponent::StaticClass(), GET_MEMBER_NAME_CHECKED(USCCameraSplineComponent, SplineCurves)));
			RemoveUCSModifiedProperties(Properties);
		}
	}
	else
	{
		SplineInstanceData->SplineCurvesPreUCS = SplineCurves;
	}

	if (SplineInstanceData->bSplineHasBeenEdited)
	{
		SplineCurves = SplineInstanceData->SplineCurves;
		bModifiedByConstructionScript = false;
	}

	bSplineHasBeenEdited = SplineInstanceData->bSplineHasBeenEdited;

	UpdateSpline();
}

FSCCameraSplinePoint USCCameraSplineComponent::GetSplinePointAtTime(float Time, ESplineCoordinateSpace::Type CoordinateSpace)
{
	FSCCameraSplinePoint SplinePoint;
	SplinePoint.Position = GetLocationAtTime(Time, CoordinateSpace);
	SplinePoint.Rotation = GetRotationAtTime(Time, CoordinateSpace);
	SplinePoint.Scale = GetScaleAtTime(Time);
	SplinePoint.FOV = GetFOVAtTime(Time);
	SplinePoint.HoldTime = GetHoldTimeAtTime(Time);
	SplinePoint.TimeToNext = GetTimeToNextAtTime(Time);

	return SplinePoint;
}


#if WITH_EDITOR
void USCCameraSplineComponent::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property != nullptr)
	{
		static const FName ReparamStepsPerSegmentName = GET_MEMBER_NAME_CHECKED(USCCameraSplineComponent, ReparamStepsPerSegment);
		static const FName StationaryEndpointsName = GET_MEMBER_NAME_CHECKED(USCCameraSplineComponent, bStationaryEndpoints);
		static const FName DefaultUpVectorName = GET_MEMBER_NAME_CHECKED(USCCameraSplineComponent, DefaultUpVector);
		static const FName ClosedLoopName = GET_MEMBER_NAME_CHECKED(USCCameraSplineComponent, bClosedLoop);
		static const FName SplineHasBeenEditedName = GET_MEMBER_NAME_CHECKED(USCCameraSplineComponent, bSplineHasBeenEdited);

		const FName PropertyName(PropertyChangedEvent.Property->GetFName());
		if (PropertyName == ReparamStepsPerSegmentName ||
			PropertyName == StationaryEndpointsName ||
			PropertyName == DefaultUpVectorName ||
			PropertyName == ClosedLoopName)
		{
			UpdateSpline();
		}
	}

	Super::PostEditChangeChainProperty(PropertyChangedEvent);
}
#endif

void USCCameraSplineComponent::UpdateDuration()
{
	float TotalDuration = 0;
	const int32 NumPoints = GetNumberOfSplinePoints();
	for (int32 Index = 0; Index < NumPoints; ++Index)
	{
		TotalDuration += GetHoldTimeAtSplinePoint(Index);
		// We don't want to include time to next for the last point
		if (Index < NumPoints - 1 || bClosedLoop)
			TotalDuration += GetTimeToNextAtSplinePoint(Index);
	}

	Duration = TotalDuration;
}



