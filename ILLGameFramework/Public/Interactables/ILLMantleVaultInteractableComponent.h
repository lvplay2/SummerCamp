// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLInteractableComponent.h"
#include "ILLMantleVaultInteractableComponent.generated.h"

/**
 * @class UILLMantleVaultInteractableComponent
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLMantleVaultInteractableComponent
: public UILLInteractableComponent
{
	GENERATED_UCLASS_BODY()

	// Begin UILLInteractableComponent interface
	virtual FBox2D GetInteractionAABB() const override;
	// End UILLInteractableComponent interface
};
