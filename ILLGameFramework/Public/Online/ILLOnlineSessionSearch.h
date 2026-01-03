// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "OnlineSessionSettings.h"

/**
 * @class FILLOnlineSessionSearch
 */
class ILLGAMEFRAMEWORK_API FILLOnlineSessionSearch
: public FOnlineSessionSearch
{
public:
	typedef FOnlineSessionSearch Super;

	FILLOnlineSessionSearch();

	virtual ~FILLOnlineSessionSearch() {}

	// Begin FOnlineSessionSearch interface
	virtual void SortSearchResults() override;
	// End FOnlineSessionSearch interface

	// Should we skip the "paranoid" QuerySettings double check in SortSearchResults?
	bool bSkipQuerySettingsEvaluation;
};
