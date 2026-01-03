// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCCounselorNavigationQueryFilter.h"

#include "NavAreas/SCNavArea_CarOnly.h"
#include "NavAreas/SCNavArea_JasonOnly.h"

USCCounselorNavigationQueryFilter::USCCounselorNavigationQueryFilter(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	AddExcludedArea(USCNavArea_CarOnly::StaticClass());
	AddExcludedArea(USCNavArea_JasonOnly::StaticClass());
}
