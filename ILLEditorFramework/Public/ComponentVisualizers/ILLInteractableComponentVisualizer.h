// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ComponentVisualizer.h"

class ILLEDITORFRAMEWORK_API FILLInteractableComponentVisualizer
	: public FComponentVisualizer
{
public:
	// Begin FComponentVisualizer Interface
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	// End FComponentVisualizer Interface
};
