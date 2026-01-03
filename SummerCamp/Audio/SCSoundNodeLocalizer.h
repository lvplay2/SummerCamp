// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

 
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Sound/SoundNode.h"
#include "SCSoundNodeLocalizer.generated.h"

class FAudioDevice;
struct FActiveSound;
struct FSoundParseParameters;
struct FWaveInstance;

/** 
 * Defines how concurrent sounds are mixed together
 */
UCLASS(hidecategories=Object, editinlinenew, MinimalAPI, meta=( DisplayName="SC Localizer" ))
class USCSoundNodeLocalizer : public USoundNode
{
	GENERATED_UCLASS_BODY()

public:
	//~ Begin USoundNode Interface.
	virtual void ParseNodes( FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances ) override;
	virtual int32 GetMaxChildNodes() const override
	{
		return 2;
	}
	virtual void CreateStartingConnectors( void ) override;
	//~ End USoundNode Interface.
};

