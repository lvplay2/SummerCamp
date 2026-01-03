// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCSpectatorStart.h"

ASCSpectatorStart::ASCSpectatorStart(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	GroundDistanceLimit = 0.f; // No need to check distance to ground since these are mostly cinematic starts for spectators/people joining the game
}
