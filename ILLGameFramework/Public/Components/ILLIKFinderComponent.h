// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Components/ActorComponent.h"
#include "ILLIKFinderComponent.generated.h"

/**
 * @enum EILLIKSearchBias
 */
UENUM()
enum class EILLIKSearchBias
{
	// Use the closest point to our bone
	Close,
	// Use the furthest point from our bone
	Far,
	// Use the point that's the furthest forward (in component space)
	Forward,
	// Use the point that's the furthest back (in component space)
	Back,
	// Use the point that's the furthest right (in component space)
	Right,
	// Use the point that's the furthest left (in component space)
	Left,
	// Use the point that's the furthest up (in component space)
	Up,
	// Use the point that's the furthest down (in component space)
	Down,

	MAX UMETA(Hidden)
};

/**
 * @struct FILLIKPoint
 */
USTRUCT(BlueprintType)
struct ILLGAMEFRAMEWORK_API FILLIKPoint
{
	GENERATED_BODY()

public:
	// CTORs
	FILLIKPoint();
	FILLIKPoint(const FHitResult& HitResult, const float InTimestamp);

	// Position in world space of point
	UPROPERTY(BlueprintReadOnly, Category = "ILLIKPoint")
	FVector Position;

	// Normal of the surface this point resides on
	UPROPERTY(BlueprintReadOnly, Category = "ILLIKPoint")
	FVector Normal;

	// Material of the surface this point resides on
	UPROPERTY(BlueprintReadOnly, Category = "ILLIKPoint")
	TEnumAsByte<EPhysicalSurface> PhysicalMaterial;

	// Timestamp of when this point was last found, or when it was first used (best point only)
	UPROPERTY(BlueprintReadOnly, Category = "ILLIKPoint")
	float Timestamp;

	// Actor hit by ray trace
	UPROPERTY(BlueprintReadOnly, Category = "ILLIKPoint")
	TWeakObjectPtr<AActor> Actor;

	// Component in actor hit by ray trace
	UPROPERTY(BlueprintReadOnly, Category = "ILLIKPoint")
	TWeakObjectPtr<UPrimitiveComponent> Component;
};

/**
 * @class UILLIKFinderComponent
 */
UCLASS(ClassGroup="ILLIKFinder", BlueprintType, Blueprintable, HideCategories=(LOD, Rendering), meta=(BlueprintSpawnableComponent, ShortTooltip="Finds valid IK points in the world to attach limbs to, handles blend in/out"))
class ILLGAMEFRAMEWORK_API UILLIKFinderComponent
: public UActorComponent
{
	GENERATED_BODY()

public:
	UILLIKFinderComponent(const FObjectInitializer& ObjectInitializer);

	// Begin UActorComponent Interface
	virtual void SetActive(bool bNewActive, bool bReset = false) override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	// End UActorComponent Interface

	/** Called when we're fully using this IK point */
	UFUNCTION(BlueprintImplementableEvent, Category = "ILLIKFinder")
	void OnPointFullyBlendedIn(const FName& Mesh, const FName& Bone, const FILLIKPoint& Point) const;

	/**
	 * Gets the relevant data for IKing a limb all in one place with appropriate modifications
	 * @param Offset - Offset (in cm) to apply from the base location along the normal of the collision
	 * @param Alpha - How blended in is this IK position?
	 * @param Location - Location (in world space) with offset applied, only filled out if Alpha > 0
	 * @param Rotation - Rotation (in component space) based on the normal of the collision with the mesh rotation applied, only filled out if Alpha > 0
	 * @return If false, we're not active and Alpha is set to 0
	 */
	UFUNCTION(BlueprintPure, Category = "ILLIKFinder")
	bool GetBestPointInfo(const float Offset, float& Alpha, FVector& Location, FRotator& Rotation) const;

	// Name of the skeletal mesh we'll be using as our search platform
	UPROPERTY(EditDefaultsOnly, Category = "ILLIKFinder")
	FName SkeletalMeshName;

	// Bone on the skeletal mesh to search from
	UPROPERTY(EditDefaultsOnly, Category = "ILLIKFinder")
	FName TargetBone;

	// Bone that will actually IK to the point (used to interpolate rotator on the best point)
	UPROPERTY(EditDefaultsOnly, Category = "ILLIKFinder")
	FName EndBone;

	// Use to decide when we should blend in/out our IK values
	UPROPERTY(EditDefaultsOnly, Category = "ILLIKFinder")
	float IKArmLength;
	
	// Min yaw to start blending IK point out
	UPROPERTY(EditDefaultsOnly, Category = "ILLIKFinder", meta=(UIMin = 0.0, UIMax = 180.0, ClampMin = 0.0, ClampMax = 180.0))
	float MinYaw;

	// Max yaw to start blending IK point in
	UPROPERTY(EditDefaultsOnly, Category = "ILLIKFinder", meta=(UIMin = 0.0, UIMax = 180.0, ClampMin = 0.0, ClampMax = 180.0))
	float MaxYaw;

	// Min pitch to start blending IK point out
	UPROPERTY(EditDefaultsOnly, Category = "ILLIKFinder", meta=(UIMin = -90.0, UIMax = 90.0, ClampMin = -90.0, ClampMax = 90.0))
	float MinPitch;

	// Max pitch to start blending IK point in
	UPROPERTY(EditDefaultsOnly, Category = "ILLIKFinder", meta=(UIMin = -90.0, UIMax = 90.0, ClampMin = -90.0, ClampMax = 90.0))
	float MaxPitch;

	// Direction (in component space) to base our search off of
	UPROPERTY(EditDefaultsOnly, Category = "ILLIKFinder")
	FVector BaseDirection;

	// Distance (in cm) to search from bone
	UPROPERTY(EditDefaultsOnly, Category = "ILLIKFinder")
	float SearchDistance;

	// Number of rows of traces to perform
	UPROPERTY(EditDefaultsOnly, Category = "ILLIKFinder")
	int32 SearchRows;

	// Min/Max value of pitch adjustment to apply for rows
	UPROPERTY(EditDefaultsOnly, Category = "ILLIKFinder")
	float RowPitchMaxOffset;

	// Number of collumns of traces to perform
	UPROPERTY(EditDefaultsOnly, Category = "ILLIKFinder")
	int32 SearchCollumns;

	// Min/Max value of yaw adjustment to apply for collumns
	UPROPERTY(EditDefaultsOnly, Category = "ILLIKFinder")
	float CollumnYawMaxOffset;

	// Channel to perform ray traces on
	UPROPERTY(EditDefaultsOnly, Category = "ILLIKFinder")
	TEnumAsByte<ECollisionChannel> TraceChannel;

	// Time in seconds to delay after we use a point, 0 will be immediately, < 0 will disable this search
	UPROPERTY(EditDefaultsOnly, Category = "ILLIKFinder")
	float SearchDelay;

	// Turning this off will provide better results on moving objects at a heafty cost per trace (Rows * Collumns)
	UPROPERTY(EditDefaultsOnly, Category = "ILLIKFinder")
	bool bUseAsyncWorld;

	// If true, we will search actual geometry rather than collision geometry
	UPROPERTY(EditDefaultsOnly, Category = "ILLIKFinder")
	bool bUseComplexSearch;

	// If true, will ignore characters when looking for a place to IK to
	UPROPERTY(EditDefaultsOnly, Category = "ILLIKFinder")
	bool bIgnoreCharacters;

	// List of all unique valid points, will be removed when out of SearchDistance range or lifespan expires, finding a near point will refresh the lifespan of a point
	UPROPERTY(BlueprintReadOnly, Category = "ILLIKFinder")
	TArray<FILLIKPoint> ValidPoints;

	// Max time (in seconds) a point remains valid
	UPROPERTY(EditDefaultsOnly, Category = "ILLIKFinder")
	float PointLifespan;

	// Defines which algorithim to use when picking the best point
	UPROPERTY(EditDefaultsOnly, Category = "ILLIKFinder")
	EILLIKSearchBias BestPointBias;

	// TODO: Add support for a blended stack of points
	// Stays the best point once it starts blending in, until it's fully blended out
	UPROPERTY(BlueprintReadOnly, Category = "ILLIKFinder")
	FILLIKPoint BestPoint;

	/**
	 * Max time (in seconds) the best point remains valid once blended starts
	 * 0 or less for no timelimit
	 */
	UPROPERTY(EditDefaultsOnly, Category = "ILLIKFinder")
	float BestPointLifespan;

	// Blend value for the best IK point, if this value is above 0, we will not be searching for a new point
	UPROPERTY(BlueprintReadOnly, Category = "ILLIKFinder")
	float IKAlpha;

	// Time (in seconds) to wait before blending in the IKAlpha
	UPROPERTY(EditDefaultsOnly, Category = "ILLIKFinder")
	float BlendInDelay;

	// Time (in seconds) it takes to blend in the IKAlpha
	UPROPERTY(EditDefaultsOnly, Category = "ILLIKFinder")
	float BlendInTime;

	// Time (in seconds) it takes to blend out IKAlpha
	UPROPERTY(EditDefaultsOnly, Category = "ILLIKFinder")
	float BlendOutTime;

protected:
	// Handle to our skeletal mesh
	UPROPERTY()
	TWeakObjectPtr<USkeletalMeshComponent> SkeletalMesh;

	// List of all directions to search in, generated in PostInitComponents
	TArray<FVector> SearchDirections;

	// Trace params to use for all searches, generated in PostInitComponents
	FCollisionQueryParams TraceParams;

	// Time when we started blending in our best point
	float LastUsed_Timestamp;

	// Pre calculated square of arm length to save math
	float IKArmLengthSq;

	/** Is this point a valid best point? */
	bool IsPointValidBestPoint(const FVector& Origin, const FRotator& Orientation, const FILLIKPoint& Point) const;

	/** Used to compare best points to find the best best point */
	int32 PointCompare(const FVector& Origin, const FRotator& Orientation, const FILLIKPoint& Left, const FILLIKPoint& Right) const;
};
