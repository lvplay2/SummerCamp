// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCAnimDriverComponent.h"

USCAnimDriverComponent::USCAnimDriverComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	SetIsReplicated(false);
}

bool USCAnimDriverComponent::BeginAnimDriver(AActor* InNotifier, FAnimNotifyData InNotifyData)
{
	if (InNotifier != nullptr && InNotifier == NotifyOwner)
	{
		BeginNotify.ExecuteIfBound(InNotifyData);
		return true;
	}
	return false;
}

void USCAnimDriverComponent::TickAnimDriver(FAnimNotifyData InNotifyData)
{
	TickNotify.ExecuteIfBound(InNotifyData);
}

void USCAnimDriverComponent::EndAnimDriver(FAnimNotifyData InNotifyData)
{
	EndNotify.ExecuteIfBound(InNotifyData);
	if (bAutoClearOwner)
		NotifyOwner = nullptr;
}

void USCAnimDriverComponent::InitializeAnimDriver(AActor* InNotifyOwner, FAnimNotifyEventDelegate InBeginDelegate, FAnimNotifyEventDelegate InTickDelegate, FAnimNotifyEventDelegate InEndDelegate)
{
	NotifyOwner = InNotifyOwner;
	BeginNotify = InBeginDelegate;
	TickNotify = InTickDelegate;
	EndNotify = InEndDelegate;
}

void USCAnimDriverComponent::InitializeNotifies(FAnimNotifyEventDelegate InBeginDelegate, FAnimNotifyEventDelegate InTickDelegate, FAnimNotifyEventDelegate InEndDelegate)
{
	BeginNotify = InBeginDelegate;
	TickNotify = InTickDelegate;
	EndNotify = InEndDelegate;
}

void USCAnimDriverComponent::InitializeBeginTick(FAnimNotifyEventDelegate InBeginDelegate, FAnimNotifyEventDelegate InTickDelegate)
{
	BeginNotify = InBeginDelegate;
	TickNotify = InTickDelegate;
	EndNotify.Clear();
}

void USCAnimDriverComponent::InitializeBeginEnd(FAnimNotifyEventDelegate InBeginDelegate, FAnimNotifyEventDelegate InEndDelegate)
{
	BeginNotify = InBeginDelegate;
	TickNotify.Clear();
	EndNotify = InEndDelegate;
}

void USCAnimDriverComponent::InitializeTickEnd(FAnimNotifyEventDelegate InTickDelegate, FAnimNotifyEventDelegate InEndDelegate)
{
	BeginNotify.Clear();
	TickNotify = InTickDelegate;
	EndNotify = InEndDelegate;
}

void USCAnimDriverComponent::InitializeBegin(FAnimNotifyEventDelegate InBeginDelegate)
{
	BeginNotify = InBeginDelegate;
	TickNotify.Clear();
	EndNotify.Clear();
}

void USCAnimDriverComponent::InitializeEnd(FAnimNotifyEventDelegate InEndDelegate)
{
	BeginNotify.Clear();
	TickNotify.Clear();
	EndNotify = InEndDelegate;
}

void USCAnimDriverComponent::InitializeTick(FAnimNotifyEventDelegate InTickDelegate)
{
	BeginNotify.Clear();
	TickNotify = InTickDelegate;
	EndNotify.Clear();
}
