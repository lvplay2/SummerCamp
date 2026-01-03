// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Navigation/CrowdFollowingComponent.h"
#include "SCCrowdFollowingComponent.generated.h"

/**
 * @class USCCrowdFollowingComponent
 */
UCLASS()
class SUMMERCAMP_API USCCrowdFollowingComponent 
: public UCrowdFollowingComponent
{
	GENERATED_UCLASS_BODY()

	// Begin UPathFollowingComponent interface
protected:
	virtual bool HasReachedInternal(const FVector& GoalLocation, float GoalRadius, float GoalHalfHeight, const FVector& AgentLocation, float RadiusThreshold, float AgentRadiusMultiplier) const override;
	// End UPathFollowingComponent interface
};
