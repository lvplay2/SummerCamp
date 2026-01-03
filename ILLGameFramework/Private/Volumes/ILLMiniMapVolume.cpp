// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "ILLGameFramework.h"
#include "ILLMiniMapVolume.h"

AILLMiniMapVolume::AILLMiniMapVolume(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetActorEnableCollision(false);
	GetBrushComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}


