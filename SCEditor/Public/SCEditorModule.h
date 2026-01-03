// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Engine.h"
#include "ModuleManager.h"
#include "UnrealEd.h"

/**
 * @class FSCEditorModule
 */
class FSCEditorModule
: public IModuleInterface
{
public:
	// Begin IModuleInterface interface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	// End IModuleInterface interface
};
