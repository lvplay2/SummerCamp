// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "AI/Navigation/NavFilters/NavigationQueryFilter.h"
#include "SCCounselorNavigationQueryFilter.generated.h"

/**
 * @class USCCounselorNavigationQueryFilter
 * @brief Excludes nav areas that Counselors should not try to path through
 */
UCLASS()
class SUMMERCAMP_API USCCounselorNavigationQueryFilter
: public UNavigationQueryFilter
{
	GENERATED_UCLASS_BODY()
};
