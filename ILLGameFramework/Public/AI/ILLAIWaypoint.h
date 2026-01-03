// Copyright (C) 2015-2017 IllFonic, LLC.

#pragma once

#include "CoreMinimal.h"
#include "Engine/NavigationObjectBase.h"
#include "ILLAIWaypoint.generated.h"

class UArrowComponent;
class UBehaviorTree;
class UTextRenderComponent;

class AILLAIWaypoint;
class AILLCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorAtWaypoint, AActor*, ActorAtWaypoint);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorLeavingWaypoint, AActor*, ActorLeavingWaypoint);

/**
* @struct FSynchronizedWaypointInfo
*/
USTRUCT()
struct FSynchronizedWaypointInfo
{
	GENERATED_USTRUCT_BODY()

	// The waypoint to synchronize with
	UPROPERTY(EditAnywhere)
	AILLAIWaypoint* SynchronizedWayoint;

	// The character needed to be at the SynchronizedWayoint in order to execute the current waypoint's actions
	UPROPERTY(EditAnywhere)
	AILLCharacter* RequiredCharacter;

	/** @return true if this waypoint is synchronized */
	bool IsSynchronized() const;
};

/**
 * @class AILLWaypoint
 */
UCLASS(Blueprintable)
class ILLGAMEFRAMEWORK_API AILLAIWaypoint
: public ANavigationObjectBase
{
	GENERATED_UCLASS_BODY()

public:

	// Begin AActor Interface
	virtual void PostLoad() override;
	virtual void PostActorCreated() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
#endif
	// End AActor Interface

	/** Updates the debug text for this waypoint */
	void UpdateWaypointText();

	/** Set the next waypoint */
	UFUNCTION(BlueprintCallable, Category = "Default")
	void SetNextWaypoint(AILLAIWaypoint* Waypoint) { NextWaypoint = Waypoint; }

	/** Get the next waypoint */
	UFUNCTION(BlueprintPure, Category = "Default")
	AILLAIWaypoint* GetNextWaypoint() const { return NextWaypoint; }

	/** Get the behavior tree to execute */
	UFUNCTION(BlueprintPure, Category = "Default")
	UBehaviorTree* GetBehaviorTree() const { return BehaviorToExecute; }

	/** Gets the transform of the DirectionToFace component so's we can pass it along to the AI and position them more better */
	UFUNCTION(BlueprintPure, Category = "Default")
	FTransform GetArrowTransform() const;

	/** Delegate for when a character is at this waypoint */
	UPROPERTY(BlueprintAssignable, Category = "Default")
	FOnActorAtWaypoint OnActorAtWaypoint;

	/** Delegate for when a character is leaving this waypoint */
	UPROPERTY(BlueprintAssignable, Category = "Default")
	FOnActorLeavingWaypoint OnActorLeavingWaypoint;

	/** @return true if this waypoint has a behavior to execute, position and/or rotation settings, or is synchronized with another waypoint */
	FORCEINLINE bool HasActionsToExecute() const { return BehaviorToExecute != nullptr || bAlignBotWithArrow || bMoveBotToArrow || bSynchronizeWithOtherWaypoints; }

	/** Add a character that is currently at this waypoint */
	FORCEINLINE void AddOverlappingActor(AActor* Actor)
	{
		if (OnActorAtWaypoint.IsBound())
			OnActorAtWaypoint.Broadcast(Actor);
		ActorsAtWaypoint.AddUnique(Actor);
	}

	/** Remove a character that was at this waypoint */
	FORCEINLINE void RemoveOverlappingActor(AActor* Actor)
	{
		if (OnActorLeavingWaypoint.IsBound())
			OnActorLeavingWaypoint.Broadcast(Actor);
		ActorsAtWaypoint.Remove(Actor);
	}

	/** @return true if the specified character is at this waypoint */
	FORCEINLINE bool IsActorAtWaypoint(const AActor* Actor) const { return ActorsAtWaypoint.Contains(Actor); }

	/** Notfiy that the specified actor is passing through this waypoint */
	void NotifyPassingThroughWaypoint(AActor* Actor) const
	{
		if (OnActorAtWaypoint.IsBound())
			OnActorAtWaypoint.Broadcast(Actor);

		if (OnActorLeavingWaypoint.IsBound())
			OnActorLeavingWaypoint.Broadcast(Actor);
	}

	// The next waypoint to move to
	UPROPERTY(EditAnywhere)
	AILLAIWaypoint* NextWaypoint;

	// Behavior tree to execute before moving on to the NextWaypoint
	UPROPERTY(EditAnywhere)
	UBehaviorTree* BehaviorToExecute;

	// Text renderer to show that this waypoint has a BT to execute
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	UTextRenderComponent* WaypointTextComponent;

	// Should we ignore this waypoint being executed more than once while at it
	UPROPERTY(EditAnywhere)
	bool bIgnoreRestartWhileAtWaypoint;

	// Color for the arrow drawing to the NextWaypoint
	UPROPERTY(EditAnywhere, Category = "Rendering")
	FColor LinkColor;

	// Should this waypoint draw it's link arrow during gameplay (only works in editor)
	UPROPERTY(EditAnywhere, Category = "Rendering")
	bool bDrawLinkArrowDuringPlay;

	// Should AI align themselves with the DirectionToFace Arrow
	UPROPERTY(EditAnywhere, Category = "BotPositioning")
	bool bAlignBotWithArrow;

	// Should the AI's rotation snap to the DirectionToFace Arrow
	UPROPERTY(EditAnywhere, Category = "BotPositioning", meta = (EditCondition = "bAlignBotWithArrow"))
	bool bSnapRotation;

	// The rate at which the AI should rotate towards DirectionToFace Arrow
	UPROPERTY(EditAnywhere, Category = "BotPositioning", meta = (ClampMin = 0.1, UIMin = 0.1, EditCondition = "bAlignBotWithArrow"))
	float RotationRate;

	// Should AI position themselves with the DirectionToFace Arrow
	UPROPERTY(EditAnywhere, Category = "BotPositioning")
	bool bMoveBotToArrow;

	// Should the AI's position snap to the DirectionToFace Arrow
	UPROPERTY(EditAnywhere, Category = "BotPositioning", meta = (EditCondition = "bMoveBotToArrow"))
	bool bSnapPosition;

	// The rate at which the AI should move towards the DirectionToFace Arrow
	UPROPERTY(EditAnywhere, Category = "BotPositioning", meta = (ClampMin = 1, UIMin = 1, EditCondition = "bMoveBotToArrow"))
	float MoveRate;

	// How long should the AI attempt to move to the DirectionToFace Arrow (in seconds)
	UPROPERTY(EditAnywhere, Category = "BotPositioning", meta = (ClampMin = 0, UIMin = 0, EditCondition = "bMoveBotToArrow"))
	float MoveToArrowTimeout;

	// Should the AI's position snap to the DirectionToFace Arrow if the MoveToArrowTimeout time limit is reached
	UPROPERTY(EditAnywhere, Category = "BotPositioning", meta = (EditCondition = "bMoveBotToArrow"))
	bool bSnapPositionOnMoveTimeout;

	// Should the AI position snap back to this waypoint's position after tasks have been preformed
	UPROPERTY(EditAnywhere, Category = "BotPositioning", meta = (EditCondition = "bMoveBotToArrow"))
	bool bSnapToWaypointAfterTasks;

	// Direction the AI should face when at this waypoint
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "BotPositioning")
	UArrowComponent* DirectionToFace;

	// Should the AI synchronize with other waypoints
	UPROPERTY(EditAnywhere, Category = "WaypointSynchronization")
	bool bSynchronizeWithOtherWaypoints;

	// How long should the AI wait for waypoint synchronization (in seconds, <= 0 is indefinite wait)
	UPROPERTY(EditAnywhere, Category = "WaypointSynchronization")
	float SynchronizeTimeout;

	// The waypoint the AI should move to if synchronization times out (useful if you want to try and loop back to this waypoint, else the AI will move to the NextWaypoint)
	UPROPERTY(EditAnywhere, Category = "WaypointSynchronization")
	AILLAIWaypoint* TimeoutWaypoint;

	// List of waypoints to synchronize with
	UPROPERTY(EditAnywhere, Category = "WaypointSynchronization")
	TArray<FSynchronizedWaypointInfo> WaypointsToSynchronizeWith;

	// List of actors to ignore collision with while at this waypoint
	UPROPERTY(EditAnywhere, Category = "Collision")
	TArray<AActor*> IgnoreCollisionOnActors;

protected:

	// List of actors at this waypoint
	UPROPERTY()
	TArray<AActor*> ActorsAtWaypoint;
};
