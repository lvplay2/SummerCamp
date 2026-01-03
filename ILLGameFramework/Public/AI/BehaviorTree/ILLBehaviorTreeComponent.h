// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "BehaviorTree/BehaviorTreeComponent.h"
#include "ILLBehaviorTreeComponent.generated.h"

/**
 * @class UILLBehaviorTreeComponent
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLBehaviorTreeComponent
: public UBehaviorTreeComponent
{
	GENERATED_UCLASS_BODY()

public:
	/** 
	 * Pauses or unpauses the behavior tree logic
	 * @param bPaused Weather to pause (true) or unpause (false) the behavior tree
	 * @param Reason Optional string used for AI debugging purposes
	 */
	UFUNCTION(BlueprintCallable, Category="AI", meta=(AdvancedDisplay="Reason"))
	void SetPaused(const bool bPaused, const FString& Reason = TEXT(""));
};
