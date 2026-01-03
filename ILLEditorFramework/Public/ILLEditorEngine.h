// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "UnrealEd.h"
#include "ILLEditorEngine.generated.h"

/**
 * @class UILLEditorEngine
 */
UCLASS()
class ILLEDITORFRAMEWORK_API UILLEditorEngine
: public UUnrealEdEngine
{
	GENERATED_UCLASS_BODY()

	// Begin UEngine interface
	virtual void Init(class IEngineLoop* InEngineLoop) override;
	// End UEngine interface
};
