// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "BehaviorTree/BTTaskNode.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

#include "BTTask_FindWanderPoint.generated.h"

class AILLAIController;
class UBlackboardComponent;

struct FBlackboardKeySelector;

/**
 * @class UBTTask_FindWanderPoint
 * @brief Finds the most desirable spot to wander towards.
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UBTTask_FindWanderPoint
: public UBTTaskNode
{
	GENERATED_UCLASS_BODY()

	// Begin UBTTaskNode interface
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	// End UBTTaskNode interface

protected:
	/** @return Center point for wandering. */
	virtual FSphere BuildWanderCenterPoint(AILLAIController* OwnerController, UBlackboardComponent* OwnerBlackboard) const;

	/** @return Center point for wandering around the actor/vector at Key. */
	FSphere BuildWanderCenterFromKey(AILLAIController* OwnerController, UBlackboardComponent* OwnerBlackboard, const FBlackboardKeySelector& Key, const float Radius) const;

	// Navigation filter class
	UPROPERTY(EditAnywhere, Category="Condition")
	TSubclassOf<UNavigationQueryFilter> FilterClass;

	// Blackboard actor or vector key to wander near
	UPROPERTY(EditAnywhere, Category="Condition")
	FBlackboardKeySelector NearKey;

	// Minimum radius around NearKey to search for a point
	UPROPERTY(EditAnywhere, Category="Condition")
	float AcceptableRadiusMin;

	// Maximum radius around NearKey to search for a point
	UPROPERTY(EditAnywhere, Category="Condition")
	float AcceptableRadiusMax;

	// Blackboard vector to store the wander point into
	UPROPERTY(EditAnywhere, Category="Condition")
	FBlackboardKeySelector WanderPointKey;
};
