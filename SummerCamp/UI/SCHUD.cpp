// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCHUD.h"

#include "SCErrorDialogWidget.h"
#include "SCMinimapIconComponent.h"
#include "SCMenuStackHUDComponent.h"

ASCHUD::ASCHUD(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	MenuStackComponent = CreateDefaultSubobject<USCMenuStackHUDComponent>(MenuStackCompName);
}

TSubclassOf<UILLMinimapWidget> ASCHUD::GetMinimapIcon(const UILLMinimapIconComponent* IconComponent) const
{
	if (const USCMinimapIconComponent* SCMapIcon = Cast<const USCMinimapIconComponent>(IconComponent))
		return bIsKiller ? SCMapIcon->KillerIcon : SCMapIcon->DefaultIcon;

	return Super::GetMinimapIcon(IconComponent);
}

bool ASCHUD::IsErrorDialogShown() const
{
	for (const FILLMenuStackElement& Menu : MenuStackComponent->GetFullStack())
	{
		if (Menu.Class->GetDefaultObject<UILLUserWidget>()->IsA(USCErrorDialogWidget::StaticClass()))
			return true;
	}

	return false;
}
