// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCSpecialItem.h"

#define LOCTEXT_NAMESPACE "ILLFonic.F13.SCSpecialItem"

ASCSpecialItem::ASCSpecialItem(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	FriendlyName = LOCTEXT("FriendlyName", "Special Item");
	bIsSpecial = true;
}

#undef LOCTEXT_NAMESPACE
