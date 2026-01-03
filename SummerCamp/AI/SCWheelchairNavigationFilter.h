// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCCounselorNavigationQueryFilter.h"
#include "SCWheelchairNavigationFilter.generated.h"

/**
 * @class USCWheelchairNavigationFilter
 * @brief Allows certain navigation areas (windows and Jason only areas) to be filtered out when considering a potential path
 */
UCLASS()
class SUMMERCAMP_API USCWheelchairNavigationFilter 
: public USCCounselorNavigationQueryFilter
{
	GENERATED_UCLASS_BODY()
};
