// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "AI/Navigation/NavFilters/NavigationQueryFilter.h"
#include "SCJasonOnlyNavigationQueryFilter.generated.h"

/**
 * @class USCJasonOnlyNavigationQueryFilter
 * @brief Allows Jason only navigation areas to be filtered out when considering a potential path
 */
UCLASS()
class SUMMERCAMP_API USCJasonOnlyNavigationQueryFilter 
: public UNavigationQueryFilter
{
	GENERATED_UCLASS_BODY()
};
