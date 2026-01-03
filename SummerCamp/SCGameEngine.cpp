// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGameEngine.h"

#include "SCGameSession.h"

USCGameEngine::USCGameEngine(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	DeferredDedicatedGameSessionClass = ASCGameSession::StaticClass();
}
