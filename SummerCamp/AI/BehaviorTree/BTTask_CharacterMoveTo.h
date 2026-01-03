// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BTTask_MoveToExtended.h"
#include "SCCharacterAIController.h"
#include "BTTask_CharacterMoveTo.generated.h"

/**
 * @class UBTTask_CharacterMoveTo
 */
UCLASS(Config=Game)
class UBTTask_CharacterMoveTo
: public UBTTask_MoveToExtended
{
	GENERATED_UCLASS_BODY()

	// Begin UBTTaskNode interface
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	// End UBTTaskNode interface

	// Begin UBTTask_MoveToExtended interface
protected:
	virtual EBTNodeResult::Type PerformMoveTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	// End UBTTask_MoveToExtended interface

	// Automatically determine BreadCrumbDistance from the owning ASCCharacterAIController?
	UPROPERTY(Category="BreadCrumbing", EditAnywhere)
	uint32 bAutomaticBreadCrumbDistance : 1;
};
