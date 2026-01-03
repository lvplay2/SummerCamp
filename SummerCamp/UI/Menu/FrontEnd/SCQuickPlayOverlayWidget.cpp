// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCQuickPlayOverlayWidget.h"

#include "SCGameInstance.h"
#include "SCOnlineMatchmakingClient.h"
#include "SCWidgetBlueprintLibrary.h"

USCQuickPlayOverlayWidget::USCQuickPlayOverlayWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void USCQuickPlayOverlayWidget::StopMatchmaking()
{
	UWorld* World = GetWorld();
	USCOnlineMatchmakingClient* OMC = Cast<USCOnlineMatchmakingClient>(Cast<USCGameInstance>(World->GetGameInstance())->GetOnlineMatchmaking());

	// Cancel matchmaking
	OMC->CancelMatchmaking();
}
