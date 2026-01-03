// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCMusicObject_Counselor.h"

USCMusicObject_Counselor::USCMusicObject_Counselor()
{
	MinIdleMusicTime = 0.f;
	MaxIdleMusicTime = 0.f;

	ScarySoundsFearLevel = 70.0f;
	ScarySoundsHearingRadiusMin = -1000.f;
	ScarySoundsHearingRadiusMax = 1000.f;
	ScarySoundCooldownMin = 10.f;
	ScarySoundCooldownMax = 20.f;
}


