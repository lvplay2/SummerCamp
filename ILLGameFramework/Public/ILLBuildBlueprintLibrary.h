// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ILLBuildDefines.h"
#include "ILLBuildBlueprintLibrary.generated.h"

/**
 * @class UILLBuildBlueprintLibrary
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLBuildBlueprintLibrary
: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** @return Current build branch. */
	UFUNCTION(BlueprintPure, Category="Build")
	static FString GetBuildBranch();

	/** @return Current build number. */
	UFUNCTION(BlueprintPure, Category="Build")
	static int32 GetBuildNumber() { return ILLBUILD_BUILD_NUMBER; }

	/** @return true if this is a console (PS4 or XboxOne) platform. */
	UFUNCTION(BlueprintPure, Category="Build")
	static bool IsPlatformConsole() { return (IsPlatformPlayStation4() || IsPlatformXboxOne()); }

	/** @return true if this is running on the PS4 platform. */
	UFUNCTION(BlueprintPure, Category="Build")
	static bool IsPlatformPlayStation4() { return PLATFORM_PS4 ? true : false; }

	/** @return true if this is running on the XboxOne platform. */
	UFUNCTION(BlueprintPure, Category="Build")
	static bool IsPlatformXboxOne() { return PLATFORM_XBOXONE ? true : false; }

	/** @return true if this is a desktop (Linux, Mac or Windows) platform. */
	UFUNCTION(BlueprintPure, Category="Build")
	static bool IsPlatformDesktop() { return (IsPlatformLinux() || IsPlatformMac() || IsPlatformWindows()); }

	/** @return true if this is running on the Linux platform. */
	UFUNCTION(BlueprintPure, Category="Build")
	static bool IsPlatformLinux() { return PLATFORM_LINUX ? true : false; }

	/** @return true if this is running on the Mac platform. */
	UFUNCTION(BlueprintPure, Category="Build")
	static bool IsPlatformMac() { return PLATFORM_MAC ? true : false; }

	/** @return true if this is running on the Windows platform. */
	UFUNCTION(BlueprintPure, Category="Build")
	static bool IsPlatformWindows() { return PLATFORM_WINDOWS ? true : false; }

	/** @return true if this is a demo build. */
	UFUNCTION(BlueprintPure, Category="Build")
	static bool IsDemoBuild() { return ILLBUILD_DEMO ? true : false; }

	/** @return true if this is an editor build. */
	UFUNCTION(BlueprintPure, Category="Build")
	static bool IsEditorBuild() { return WITH_EDITOR ? true : false; }

	/** @return true if this is a shipping build. */
	UFUNCTION(BlueprintPure, Category="Build")
	static bool IsShippingBuild() { return UE_BUILD_SHIPPING ? true : false; }
};
