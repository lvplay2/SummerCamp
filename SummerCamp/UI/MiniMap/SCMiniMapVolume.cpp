// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCMiniMapVolume.h"

ASCMiniMapVolume::ASCMiniMapVolume(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetActorEnableCollision(false);
	GetBrushComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}


