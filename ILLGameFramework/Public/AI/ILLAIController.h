// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "AIController.h"
#include "ILLAIController.generated.h"

class AILLAIWaypoint;

ILLGAMEFRAMEWORK_API DECLARE_LOG_CATEGORY_EXTERN(LogILLAI, Log, All);

/**
 * @class AILLAIController
 */
UCLASS(Config=Game)
class ILLGAMEFRAMEWORK_API AILLAIController
: public AAIController
{
	GENERATED_UCLASS_BODY()

	// Begin UObject interface
	virtual void BeginDestroy() override;
	// End UObject interface

	// Begin AActor interface
	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
	// End AActor interface

	// Begin AController interface
	virtual void InitPlayerState() override;
	// End AController interface

	// Overridden to call ChangeBehaviorTree in some cases
	virtual bool RunBehaviorTree(UBehaviorTree* BTAsset) override;

	// Change out our behavior tree at the end of the next tick
	UFUNCTION(BlueprintCallable, Category = "AI")
	void ChangeBehaviorTree(UBehaviorTree* InBehaviorTree);

private:
	// Our Pending behavior tree to be changed at the end of Tick
	UPROPERTY(Transient)
	class UBehaviorTree* PendingBehaviorTree;

	/////////////////////////////////////////////////////////////////////////////
	// Waypoints
public:
	/** Set this character's StartingWaypoint */
	UFUNCTION(BlueprintCallable, Category="Waypoints")
	void SetStartingWaypoint(AILLAIWaypoint* Waypoint) { StartingWaypoint = Waypoint; }

	/** Sets this character's OverrideNextWaypoint. This Waypoint will be used instead of their current Waypoint's NextWaypoint when BTTask_FindNextWaypoint is executed. */
	UFUNCTION(BlueprintCallable, Category="Waypoints")
	void SetOverrideNextWaypoint(AILLAIWaypoint* Waypoint) { OverrideNextWaypoint = Waypoint; }

	/** @return This character's OverrideNextWaypoint */
	UFUNCTION(BlueprintPure, Category="Waypoints")
	AILLAIWaypoint* GetOverrideNextWaypoint() const { return OverrideNextWaypoint; }

	// Waypoint the AI should start pathing from
	UPROPERTY(EditAnywhere, Category="Waypoints")
	AILLAIWaypoint* StartingWaypoint;

	// Has this character been assigned their StartingWaypoint?
	bool bAssignedStartingWaypoint;

	// Can this character execute synchronized way point actions?
	bool bCanExecuteSynchronizedWaypointActions;

protected:
	// The next Waypoint this character should move to (overrides their current way point's NextWaypoint)
	UPROPERTY(Transient)
	AILLAIWaypoint* OverrideNextWaypoint;
};

