// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "SCRoadPoint.generated.h"

/**
 * @class USCRoadPointComponent
 * Component to use as the root for ASCRoadPoint. For access to CreateSceneProxy(), etc.
 */
UCLASS(ClassGroup = Utility, ShowCategories = (Mobility), HideCategories = (Physics, Collision, Lighting, Rendering, Mobile), meta=(BlueprintSpawnableComponent))
class SUMMERCAMP_API USCRoadPointComponent
: public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

public:
#if !UE_BUILD_SHIPPING
	// Begin UPrimitiveComponent interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	// End UPrimitiveComponent interface
#endif
};

/**
 * @class ASCRoadPoint
 * Points to markup roads for AI traversal
 */
UCLASS()
class SUMMERCAMP_API ASCRoadPoint
: public AActor
{
	GENERATED_UCLASS_BODY()

public:
	// Root component for the road. Handles scene proxy rendering schtuff.
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCRoadPointComponent* Root;

	// Road points that are considered "neighbors" when traversing the road
	UPROPERTY(EditAnywhere, Category = "Default")
	TArray<ASCRoadPoint*> Neighbors;

	// Whether or not this point is considered a goal location (should be with a vehicle escape volume)
	UPROPERTY(EditAnywhere, Category = "Default")
	bool bGoalLocation;

	/** Sets if this road point is a goal location */
	UFUNCTION(BlueprintCallable, Category="Default")
	void SetIsGoalLocation(bool bIsGoal) { bGoalLocation = bIsGoal; }

#if WITH_EDITOR
	// Begin AActor interface
	virtual void CheckForErrors() override;
	// End AActor interface

	// Begin UObject interface
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	// End UObject interface
#endif
};
