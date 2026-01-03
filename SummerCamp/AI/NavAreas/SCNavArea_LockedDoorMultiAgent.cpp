// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCNavArea_LockedDoorMultiAgent.h"

#include "AI/Navigation/NavAreas/NavArea_Null.h"

#include "SCNavArea_LockedDoor.h"

USCNavArea_LockedDoorMultiAgent::USCNavArea_LockedDoorMultiAgent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	Agent0Area = USCNavArea_LockedDoor::StaticClass(); // Counselors
	// Agent1Area = NavArea_Default // Jason

	// Cars can't use doors...
	Agent2Area = UNavArea_Null::StaticClass(); 
}
