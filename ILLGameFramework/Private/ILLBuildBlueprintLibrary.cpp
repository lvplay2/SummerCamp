// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLBuildBlueprintLibrary.h"

#include "Resources/Version.h"

FString UILLBuildBlueprintLibrary::GetBuildBranch()
{
	return FString(TEXT(BRANCH_NAME));
}
