// Copyright (C) 2015-2018 IllFonic, LLC.

#ifndef __ILLGAMEFRAMEWORK_H__
#define __ILLGAMEFRAMEWORK_H__

// Engine includes
#include "Engine.h"
#include "SlateBasics.h"
#include "SlateExtras.h"
#include "ParticleDefinitions.h"
#include "SoundDefinitions.h"
#include "Net/UnrealNetwork.h"

// IGF includes
#include "ILLGameFrameworkClasses.h"
#include "ILLProjectSettings.h"

// IGF module setup
#include "ModuleInterface.h"

class IILLGameFrameworkModule
: public IModuleInterface
{
public:
};

// IGF log categories
ILLGAMEFRAMEWORK_API DECLARE_LOG_CATEGORY_EXTERN(LogIGF, Log, All);

#define SafeUniqueIdTChar(UniqueId) ((UniqueId).IsValid() ? *(UniqueId).ToString() : TEXT("INVALID"))

// Network encryption constants for AES
#define ILLNETENCRYPTION_AESKEY_BYTES 32
#define ILLNETENCRYPTION_AESIV_BYTES 16
#define ILLNETENCRYPTION_HMACKEY_BYTES 32

#endif // __ILLGAMEFRAMEWORK_H__
