// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_FindSearchableCabinet.generated.h"

/**
 * @class UBTTask_FindSearchableCabinet
 * @brief Find a searchable cabinet inside of the cabin the AI is in.
 */
UCLASS()
class SUMMERCAMP_API UBTTask_FindSearchableCabinet 
: public UBTTask_BlackboardBase
{
	GENERATED_UCLASS_BODY()
	
	// Begin UBTTaskNode interface
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	// End UBTTaskNode interface
	
protected:
	// Navigation filter class
	UPROPERTY(EditAnywhere, Category="Condition")
	TSubclassOf<UNavigationQueryFilter> FilterClass;
};
