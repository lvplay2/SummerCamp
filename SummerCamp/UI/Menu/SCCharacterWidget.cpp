// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCCharacterWidget.h"

#include "SCBackendRequestManager.h"
#include "SCCharacter.h"
#include "SCGameInstance.h"
#include "SCLocalPlayer.h"
#include "SCPlayerState.h"

USCCharacterWidget::USCCharacterWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

USCBackendInventory* USCCharacterWidget::GetInventoryManager()
{
	if (!GetPlayerContext().IsValid())
		return nullptr;

	// Setup the Backend Inventory
	if (USCLocalPlayer* const LocalPlayer = Cast<USCLocalPlayer>(GetPlayerContext().GetLocalPlayer()))
	{
		USCGameInstance* GI = Cast<USCGameInstance>(GetWorld()->GetGameInstance());
		USCBackendRequestManager* SCBackend = GI->GetBackendRequestManager<USCBackendRequestManager>();
		USCBackendPlayer* BackendPlayer = LocalPlayer->GetBackendPlayer<USCBackendPlayer>();
		if (BackendPlayer)
		{
			return BackendPlayer->GetBackendInventory();
		}
	}

	return nullptr;
}

bool USCCharacterWidget::AreSettingsDirty()
{
	if (USCBackendInventory* BackendInventory = GetInventoryManager())
	{
		return BackendInventory->bIsDirty;
	}

	return false;
}

void USCCharacterWidget::ClearDirty()
{
	if (USCBackendInventory* BackendInventory = GetInventoryManager())
	{
		BackendInventory->bIsDirty = false;
	}
}
