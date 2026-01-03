// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_FindDoorToBarricade.generated.h"

/**
 * @class UBTTask_FindDoorToBarricade
 * @brief Finds a barricadeable door in the cabin the counselor is currently in
 */
UCLASS()
class SUMMERCAMP_API UBTTask_FindDoorToBarricade 
: public UBTTask_BlackboardBase
{
	GENERATED_UCLASS_BODY()

	// Begin UBTTaskNode interface
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	// End UBTTaskNode interface

private:
	// Minimum distance from the door to consider it for barricading, so we ignore the door we just entered through
	float MinimumDistance; // FIXME: pjackson: This sucks. Try and ensure the counselor moves to a point inside and faces outside instead.
};
