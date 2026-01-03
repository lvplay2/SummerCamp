// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLEditorFramework.h"

/**
 * @class FILLEditorFrameworkModule
 */
class FILLEditorFrameworkModule
: public IILLEditorFrameworkModule
{
public:
	// Begin IModuleInterface interface
	virtual void StartupModule() override {}
	virtual bool IsGameModule() const override { return true; }
	// End IModuleInterface interface
};

IMPLEMENT_GAME_MODULE(FILLEditorFrameworkModule, ILLEditorFramework);
