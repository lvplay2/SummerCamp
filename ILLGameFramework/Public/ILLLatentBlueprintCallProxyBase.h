// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLLatentBlueprintCallProxyBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEmptyILLLatentDelegate);

/**
 * @class UILLLatentBlueprintCallProxyBase
 * @brief Blueprint node that more or less clones UOnlineBlueprintCallProxyBase. Use FEmptyILLLatentDelegate for latent pins.
 */
UCLASS(MinimalAPI)
class UILLLatentBlueprintCallProxyBase
: public UObject
{
	GENERATED_UCLASS_BODY()

	// Called to trigger the actual latent action once the delegates have been bound
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true"), Category="Latent")
	virtual void Activate();
};
