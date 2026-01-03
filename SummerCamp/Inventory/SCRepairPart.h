// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCItem.h"
#include "SCRepairPart.generated.h"

class USCInteractComponent;

/**
 * @class ASCRepairPart
 */
UCLASS()
class SUMMERCAMP_API ASCRepairPart
: public ASCItem
{
	GENERATED_UCLASS_BODY()

public:
	// Begin ASCItem interface
	virtual bool Use(bool bFromInput) override;
	virtual bool CanUse(bool bFromInput) override;
	// End ASCItem interface

	////////////////////////////////////////////////////////////////////////////////////
	// AI
public:
	/** @return Closest AI repair goal distance squared. */
	FORCEINLINE float GetClosestRepairGoalDistanceSquared() const
	{
		ConditionalUpdateGoalDistance();
		return ClosestRepairGoalDistanceSquared;
	}

	/** @return Repair part goal cache for AI. */
	FORCEINLINE const TMap<USCInteractComponent*, FVector>& GetRepairGoalCache() const
	{
		ConditionalUpdateGoalCache();
		return RepairGoalCache;
	}

	/** Calls UpdateGoalCache if we have not called it recently. */
	virtual void ConditionalUpdateGoalCache() const;

	/** Updates ClosestRepairGoalDistanceSquared if we have not recently. */
	virtual void ConditionalUpdateGoalDistance() const;

	/** Rebuilds RepairGoalCache and updates ClosestRepairGoalDistanceSquared. */
	virtual void UpdateGoalCache() const;

protected:
	// Cache of known repair goals relevant to this repair part
	UPROPERTY(Transient)
	mutable TMap<USCInteractComponent*, FVector> RepairGoalCache;

	// Closest AI repair goal distance squared
	mutable float ClosestRepairGoalDistanceSquared;

	// Last time that the goal cache was updated
	mutable float LastGoalCacheUpdateTime;

	// Last time that the goal distance was calculated
	mutable float LastGoalDistanceUpdateTime;
};
