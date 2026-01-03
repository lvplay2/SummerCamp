// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCWheelchairNavigationFilter.h"

#include "NavAreas/SCNavArea_Window.h"


USCWheelchairNavigationFilter::USCWheelchairNavigationFilter(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	AddExcludedArea(USCNavArea_Window::StaticClass());
}
