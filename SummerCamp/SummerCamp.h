// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Engine.h"

#include "SoundDefinitions.h"
#include "Net/UnrealNetwork.h"

#include "ILLGameFramework/Public/ILLGameFramework.h"

#include "SCTypes.h"

#if WITH_EDITOR
# include "ILLEditorFramework/Public/ILLEditorFramework.h"
# include "SCEditor/Public/SCEditorModule.h"
#endif

DECLARE_LOG_CATEGORY_EXTERN(LogSC, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogSCWeapon, Log, All);

/** Used to control seamless travel between all modes. */
#define SC_SEAMLESS_TRAVEL true
