// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCJasonOnlyNavigationQueryFilter.h"

#include "NavAreas/SCNavArea_JasonOnly.h"


USCJasonOnlyNavigationQueryFilter::USCJasonOnlyNavigationQueryFilter(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	AddExcludedArea(USCNavArea_JasonOnly::StaticClass());
}
