// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCMiniMapBase.h"

// UE4
#include "Image.h"
#include "CanvasPanel.h"
#include "Overlay.h"
#include "CanvasPanelSlot.h"

// IGF
#include "ILLHUD.h"
#include "IllMinimapWidget.h"

// SC
#include "SCCharacter.h"
#include "SCGameSettingsSaveGame.h"
#include "SCLocalPlayer.h"
#include "SCMiniMapVolume.h"
#include "SCPlayerState.h"

USCMiniMapBase::USCMiniMapBase(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void USCMiniMapBase::InitializeMapMaterial(UMaterialInstanceDynamic* MID, AILLMiniMapVolume* Volume)
{
	USCLocalPlayer* LocalPlayer = CastChecked<USCLocalPlayer>(GetOwningLocalPlayer());
	if (USCGameSettingsSaveGame* GameSettings = LocalPlayer->GetLoadedSettingsSave<USCGameSettingsSaveGame>())
	{
		bRotateMap = (GameSettings->bRotateMinimapWithPlayer && bCanEverRotate);
	}

	if (ASCMiniMapVolume* SCVolume = Cast<ASCMiniMapVolume>(Volume))
	{
		if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(GetOwningPlayerPawn()))
		{
			MID->SetTextureParameterValue(TEXT("Texture"), SCVolume->KillerMapTexture.LoadSynchronous());
			return;
		}
	}

	Super::InitializeMapMaterial(MID, Volume);
}
