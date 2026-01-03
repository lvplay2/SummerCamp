// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "IllMinimapWidget.h"

#include "CanvasPanelSlot.h"

#include "ILLMinimap.h"

UILLMinimapWidget::UILLMinimapWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, ZOrder(0)
, bScaleWithMap(false)
, DefaultSize(FVector2D(64.f, 64.f))
{
}

void UILLMinimapWidget::InitializeWidget_Implementation(UObject* InOwner, AActor* InMiniMapOwner, UILLMinimap* InMinimap)
{
	Owner = InOwner;
	MiniMapOwner = InMiniMapOwner;
	Minimap = InMinimap;

	if (UCanvasPanelSlot* WidgetSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		if (bScaleWithMap)
		{
			WidgetSlot->SetSize(DefaultSize * Minimap->MinimapZoom);
		}
		WidgetSlot->SetSize(DefaultSize);
		WidgetSlot->ZOrder = ZOrder;
	}


}

bool UILLMinimapWidget::UpdateMapWidget_Implementation(FVector& CurrentPosition, FRotator& CurrentRotation)
{
	return true;
}
