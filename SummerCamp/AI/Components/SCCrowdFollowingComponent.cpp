// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCCrowdFollowingComponent.h"

#include "SCCharacter.h"


USCCrowdFollowingComponent::USCCrowdFollowingComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

bool USCCrowdFollowingComponent::HasReachedInternal(const FVector& GoalLocation, float GoalRadius, float GoalHalfHeight, const FVector& AgentLocation, float RadiusThreshold, float AgentRadiusMultiplier) const
{
	// This is a copy of UPathfollowingComponent::HasReachedInternal() with in water checks

	if (MovementComp == NULL)
	{
		return false;
	}

	// get cylinder of moving agent
	float AgentRadius = 0.0f;
	float AgentHalfHeight = 0.0f;
	AActor* MovingAgent = MovementComp->GetOwner();
	MovingAgent->GetSimpleCollisionCylinder(AgentRadius, AgentHalfHeight);

	// check if they overlap (with added AcceptanceRadius)
	const FVector ToGoal = GoalLocation - AgentLocation;

	const float Dist2DSq = ToGoal.SizeSquared2D();
	const float UseRadius = FMath::Max(RadiusThreshold, GoalRadius + (AgentRadius * AgentRadiusMultiplier));
	if (Dist2DSq > FMath::Square(UseRadius))
	{
		return false;
	}

	// Ignore Z distance if we are in water
	const bool bInWater = MovingAgent->IsA<ASCCharacter>() ? Cast<ASCCharacter>(MovingAgent)->bInWater : false;
	if (!bInWater)
	{
		const float ZDiff = FMath::Abs(ToGoal.Z);
		const float UseHeight = GoalHalfHeight + (AgentHalfHeight * MinAgentHalfHeightPct);
		if (ZDiff > UseHeight)
		{
			return false;
		}
	}

	return true;
}
