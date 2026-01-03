// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLMantleVaultInteractableComponent.h"

#include "ILLMantleVaultEdge.h"

UILLMantleVaultInteractableComponent::UILLMantleVaultInteractableComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

FBox2D UILLMantleVaultInteractableComponent::GetInteractionAABB() const
{
	if (AILLMantleVaultEdge* Edge = Cast<AILLMantleVaultEdge>(GetOwner()))
	{
		FBox2D Result;
		for (const FMantleVaultEdgePoint& Point : Edge->EdgePoints)
		{
			const float Radius = DistanceLimit;
			const FVector2D Location(GetComponentLocation());
			const FVector2D Offset(Radius, Radius);
			const FBox2D AABB(Location - Offset, Location + Offset);
			Result += AABB;
		}

		return Result;
	}

	return Super::GetInteractionAABB();
}
