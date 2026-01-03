// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Http.h"
#include "ILLBackendUser.generated.h"

/**
 * @class UILLBackendUser
 */
UCLASS(Abstract, Transient, Config=Engine)
class ILLGAMEFRAMEWORK_API UILLBackendUser
: public UObject
{
	GENERATED_UCLASS_BODY()

	/**
	 * Allows this backend user to adjust HTTP requests being created before being added to the pending request list.
	 *
	 * @param Request HTTP request being formed.
	 */
	virtual void SetupRequest(FHttpRequestPtr HttpRequest) {}
};
