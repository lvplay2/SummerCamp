// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLHTTPRequestTask.h"

/**
 * @struct FILLBackendEndpointHandle
 */
struct ILLGAMEFRAMEWORK_API FILLBackendEndpointHandle
{
	// Index into UILLBackendRequestManager::EndpointList
	int32 EndpointIndex;

	FILLBackendEndpointHandle()
	: EndpointIndex(INDEX_NONE) {}

	FORCEINLINE operator bool() const { return (EndpointIndex != INDEX_NONE); }
};

/**
 * @struct FILLBackendEndpointResource
 * @brief Configuration structure for each backend endpoint.
 */
struct ILLGAMEFRAMEWORK_API FILLBackendEndpointResource
{
	// Full resource URL to make the request to
	const FString URL;

	// HTTP verb of the request type
	const EILLHTTPVerb Verb;

	// ClientKeyList index
	const int32 ClientKeyIndex;

	// Should we require an authorized server/player?
	bool bRequireUser;

	FILLBackendEndpointResource(const FString& InURL, const EILLHTTPVerb InVerb, const int32 InClientKeyIndex, const bool InRequireUser)
	: URL(InURL), Verb(InVerb), ClientKeyIndex(InClientKeyIndex), bRequireUser(InRequireUser) {}
};
