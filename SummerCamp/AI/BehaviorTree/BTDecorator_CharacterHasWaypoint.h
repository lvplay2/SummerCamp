// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BehaviorTree/Decorators/BTDecorator_BlackboardBase.h"
#include "BTDecorator_CharacterHasWaypoint.generated.h"

/**
 * @class UBTDecorator_CharacterHasWaypoint
 */
UCLASS()
class SUMMERCAMP_API UBTDecorator_CharacterHasWaypoint 
: public UBTDecorator
{
	GENERATED_UCLASS_BODY()
	
	// Begin UBTDecorator interface
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	// End UBTDecorator interface
	
protected:

	// Blackboard actor to store the next waypoint
	UPROPERTY(EditAnywhere, Category="Condition")
	FBlackboardKeySelector WaypointKey;
};
