// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLBehaviorTreeComponent.h"

UILLBehaviorTreeComponent::UILLBehaviorTreeComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void UILLBehaviorTreeComponent::SetPaused(const bool bPaused, const FString& Reason/* = TEXT("")*/)
{
	if (bPaused)
	{
		PauseLogic(Reason);
	}
	else
	{
		ResumeLogic(Reason);
	}
}
