// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCMenuWidget.h"

#include "Camera/CameraActor.h"

#include "SCHUD.h"
#include "SCGameInstance.h"
#include "SCGameState.h"
#include "SCGameState_Lobby.h"
#include "SCHUD_Lobby.h"
#include "SCOnlineSessionClient.h"

USCMenuWidget::USCMenuWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bBlockInputForTransitionOut = true;
}

UILLUserWidget* USCMenuWidget::ChangeRootMenu(TSubclassOf<UILLUserWidget> MenuClass, const bool bForce/* = false*/)
{
	UILLUserWidget* NewRootMenu = nullptr;
	if (APlayerController* OwningPlayer = GetOwningPlayer())
	{
		if (ASCHUD* HUD = Cast<ASCHUD>(OwningPlayer->GetHUD()))
		{
			NewRootMenu = HUD->ChangeRootMenu(MenuClass, bForce);
		}
		else if (ASCHUD_Lobby* LobbyHUD = Cast<ASCHUD_Lobby>(OwningPlayer->GetHUD()))
		{
			NewRootMenu = LobbyHUD->ChangeRootMenu(MenuClass, bForce);
		}

		if (USCGameInstance* GI = Cast<USCGameInstance>(OwningPlayer->GetGameInstance()))
		{
			GI->CheckControllerPairings();
		}
	}

	return NewRootMenu;
}

UILLUserWidget* USCMenuWidget::PushMenu(TSubclassOf<UILLUserWidget> MenuClass, const bool bSkipTransitionOut/* = false*/)
{
	if (APlayerController* OwningPlayer = GetOwningPlayer())
	{
		if (ASCHUD* HUD = Cast<ASCHUD>(OwningPlayer->GetHUD()))
		{
			return HUD->PushMenu(MenuClass, bSkipTransitionOut);
		}
		else if (ASCHUD_Lobby* LobbyHUD = Cast<ASCHUD_Lobby>(OwningPlayer->GetHUD()))
		{
			return LobbyHUD->PushMenu(MenuClass, bSkipTransitionOut);
		}
	}

	return nullptr;
}

bool USCMenuWidget::PopMenu(FName WidgetToFocus/* = NAME_None*/, const bool bForce/* = false*/)
{
	if (APlayerController* OwningPlayer = GetOwningPlayer())
	{
		if (ASCHUD* HUD = Cast<ASCHUD>(OwningPlayer->GetHUD()))
		{
			return HUD->PopMenu(WidgetToFocus, bForce);
		}
		else if (ASCHUD_Lobby* LobbyHUD = Cast<ASCHUD_Lobby>(OwningPlayer->GetHUD()))
		{
			return LobbyHUD->PopMenu(WidgetToFocus, bForce);
		}
	}

	return false;
}

bool USCMenuWidget::RemoveMenuByClass(TSubclassOf<UILLUserWidget> MenuClass)
{
	if (APlayerController* OwningPlayer = GetOwningPlayer())
	{
		if (ASCHUD* HUD = Cast<ASCHUD>(OwningPlayer->GetHUD()))
		{
			return HUD->RemoveMenuByClass(MenuClass);
		}
		else if (ASCHUD_Lobby* LobbyHUD = Cast<ASCHUD_Lobby>(OwningPlayer->GetHUD()))
		{
			return LobbyHUD->RemoveMenuByClass(MenuClass);
		}
	}

	return false;
}

void USCMenuWidget::ChangeCamera(const FString& CameraName, const float TransitionTime)
{
	TArray<AActor*> CameraActorList;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACameraActor::StaticClass(), CameraActorList);

	for (AActor* CameraActor : CameraActorList)
	{
		if (CameraActor->GetName() == CameraName)
		{
			if (APlayerController* OwningPlayer = GetOwningPlayer())
			{
				OwningPlayer->SetViewTargetWithBlend(CameraActor, TransitionTime);
			}
			break;
		}
	}
}

bool USCMenuWidget::IsViewingMenuInLobby() const
{
	if (UWorld* World = GetWorld())
	{
		return (World->GetGameState() && World->GetGameState()->IsA(ASCGameState_Lobby::StaticClass()));
	}

	return false;
}

bool USCMenuWidget::IsViewingMenuInGame() const
{
	if (UWorld* World = GetWorld())
	{
		return (World->GetGameState() && World->GetGameState()->IsA(ASCGameState::StaticClass()));
	}

	return false;
}
