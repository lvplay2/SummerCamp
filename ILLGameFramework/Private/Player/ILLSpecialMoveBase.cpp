// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLSpecialMoveBase.h"

#include "ILLCharacter.h"

UILLSpecialMoveBase::UILLSpecialMoveBase(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	MoveTimeLimit = 5.f;
}

void UILLSpecialMoveBase::BeginDestroy()
{
	// Notify listeners, Broadcast is always safe on multicast even when nothing is bound
	CompletionDelegate.Broadcast();
	// Clear any delegates so we don't leave anything hanging.
	CompletionDelegate.Clear();

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_TimeLimit);
	}

	Super::BeginDestroy();
}

UWorld* UILLSpecialMoveBase::GetWorld() const
{
	return GetOuterAILLCharacter() ? GetOuterAILLCharacter()->GetWorld() : NULL;
}

void UILLSpecialMoveBase::BeginMove(AActor* InTargetActor, FOnILLSpecialMoveCompleted::FDelegate InCompletionDelegate)
{
	TargetActor = InTargetActor;
	CompletionDelegate.Add(InCompletionDelegate);

	if (MoveTimeLimit > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_TimeLimit, this, &UILLSpecialMoveBase::MoveTimeLimitHit, MoveTimeLimit);
	}
}

void UILLSpecialMoveBase::ResetTimeout(const float NewTimeout)
{
	if (NewTimeout >= 0.f)
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_TimeLimit, this, &UILLSpecialMoveBase::MoveTimeLimitHit, NewTimeout);
	else
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_TimeLimit);
}

void UILLSpecialMoveBase::MoveTimeLimitHit()
{
	MarkPendingKill();
}

void UILLSpecialMoveBase::AddCompletionDelegate(FOnILLSpecialMoveCompleted::FDelegate InCompletionDelegate)
{
	CompletionDelegate.Add(InCompletionDelegate);
}

void UILLSpecialMoveBase::RemoveCompletionDelegates(UObject* ForObject)
{
	CompletionDelegate.RemoveAll(ForObject);
}

float UILLSpecialMoveBase::GetTimeRemaining() const
{
	if (!GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_TimeLimit))
		return -1.f;

	return GetWorld()->GetTimerManager().GetTimerRemaining(TimerHandle_TimeLimit);
}
