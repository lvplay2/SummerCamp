// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BTTask_CharacterMoveTo.h"
#include "SCCounselorAIController.h"
#include "BTTask_CounselorMoveTo.generated.h"

/**
 * @class UBTTask_CounselorMoveTo
 */
UCLASS(Config=Game)
class UBTTask_CounselorMoveTo
: public UBTTask_CharacterMoveTo
{
	GENERATED_UCLASS_BODY()

	// Begin UBTTaskNode interface
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	// End UBTTaskNode interface

	// Begin UBTTask_MoveToExtended interface
protected:
	virtual EBTNodeResult::Type PerformMoveTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	// End UBTTask_MoveToExtended interface

protected:
	// Running movement preference
	UPROPERTY(EditAnywhere, Category="Counselor")
	ESCCounselorMovementPreference RunPreference;

	// Sprinting movement preference
	UPROPERTY(EditAnywhere, Category="Counselor")
	ESCCounselorMovementPreference SprintPreference;
};
