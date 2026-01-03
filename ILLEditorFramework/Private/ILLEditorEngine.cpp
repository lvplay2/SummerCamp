// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLEditorFramework.h"
#include "ILLEditorEngine.h"

// IEF
#include "ILLInteractableComponentVisualizer.h"

// IGF
#include "ILLInteractableComponent.h"

UILLEditorEngine::UILLEditorEngine(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void UILLEditorEngine::Init(class IEngineLoop* InEngineLoop)
{
	Super::Init(InEngineLoop);

	// Register visualizers
	GUnrealEd->RegisterComponentVisualizer(UILLInteractableComponent::StaticClass()->GetFName(), MakeShareable(new FILLInteractableComponentVisualizer));
}
