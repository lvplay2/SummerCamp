// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCNameplateWidget.h"

#include "SCCharacter.h"
#include "SCPlayerState.h"

USCNameplateWidget::USCNameplateWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

ASCCharacter* USCNameplateWidget::GetOwningCharacter()
{
	return Cast<ASCCharacter>(ActorContext);
}

ASCPlayerState* USCNameplateWidget::GetOwningPlayerState()
{
	if (ASCCharacter* OwningCharacter = GetOwningCharacter())
	{
		return Cast<ASCPlayerState>(OwningCharacter->PlayerState);
	}

	return nullptr;
}
