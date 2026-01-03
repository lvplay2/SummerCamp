// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "AI/Navigation/NavAreas/NavAreaMeta_SwitchByAgent.h"
#include "SCNavArea_LockedDoorMultiAgent.generated.h"

/**
 * @class USCNavArea_LockedDoorMultiAgent
 * @brief Allows for separate NavAreas based on the NavAgent for locked doors.
 */
UCLASS()
class SUMMERCAMP_API USCNavArea_LockedDoorMultiAgent 
: public UNavAreaMeta_SwitchByAgent
{
	GENERATED_UCLASS_BODY()
};
