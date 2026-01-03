// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "BlueprintGraphDefinitions.h" // For UK2Node_BaseAsyncTask and friends
#include "K2Node_LatentILLBackendCall.generated.h"

/**
 * @class UK2Node_LatentILLBackendCall
 * @brief This is more or less just a clone of UK2Node_LatentOnlineCall for latent callback proxy registration.
 */
UCLASS()
class ILLEDITORFRAMEWORK_API UK2Node_LatentILLBackendCall
: public UK2Node_BaseAsyncTask
{
	GENERATED_UCLASS_BODY()
	
	// UEdGraphNode interface
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	// End of UEdGraphNode interface
};
