// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCCounselorNavigationQueryFilter.h"
#include "SCCounselorAvoidTrapsQueryFilter.generated.h"

/**
 * @class USCCounselorAvoidTrapsQueryFilter
 * @brief Excludes nav areas that Counselors should not try to path through including traps
 */
UCLASS()
class SUMMERCAMP_API USCCounselorAvoidTrapsQueryFilter
: public USCCounselorNavigationQueryFilter
{
	GENERATED_UCLASS_BODY()
};
