// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BehaviorTree/BTTaskNode.h"
#include "SCCounselorAIController.h"
#include "BTTask_SetCounselorFlashlightMode.generated.h"

/**
 * @class UBTTask_SetCounselorFlashlightMode
 */
UCLASS()
class SUMMERCAMP_API UBTTask_SetCounselorFlashlightMode 
: public UBTTaskNode
{
	GENERATED_UCLASS_BODY()

	// Begin UBTTaskNode interface
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;
	// End UBTTaskNode interface

	// Flashlight mode to apply
	UPROPERTY(EditAnywhere, Category="Task")
	ESCCounselorFlashlightMode FlashlightMode;
};
