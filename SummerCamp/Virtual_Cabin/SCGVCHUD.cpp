// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGVCHUD.h"
#include "Virtual_Cabin/SCGVCCharacter.h"
#include "SCGameInstance.h"

ASCGVCHUD::ASCGVCHUD(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	PauseMenuClass = StaticLoadClass(USCMenuWidget::StaticClass(), nullptr, TEXT("/Game/Virtual_Cabins/Blueprints/VC_InterfaceBlueprints/BP_SCVCPauseMenu.BP_SCVCPauseMenu_C"));
}

void ASCGVCHUD::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (USCGameInstance* GameInstance = Cast<USCGameInstance>(World->GetGameInstance()))
		{
			// Hide the loading screen
			GameInstance->HideLoadingScreen();
		}

		if (m_InvWidgetBP != nullptr)
		{
			m_InvWidgetRef = CreateWidget<USCGVCInventoryWidget>(GetOwningPlayerController(), m_InvWidgetBP);
		}

		if (m_DebugMenuBP != nullptr)
		{
			m_DebugMenuWidgetRef = CreateWidget<USCGVCDebugMenuWidget>(GetOwningPlayerController(), m_DebugMenuBP);
		}

		if (m_InvWidgetRef->IsInViewport() == false)
			m_InvWidgetRef->AddToViewport();

		if (m_DebugMenuWidgetRef->IsInViewport() == false)
			m_DebugMenuWidgetRef->AddToViewport();

		m_IsDebugMenuOpen = false;
		m_IsInventoryOpen = false;
		m_IsDescriptionVisible = false;
	}
}

void ASCGVCHUD::HandleInventoryUp()
{
	if (m_InvWidgetRef != nullptr)
	{
		m_InvWidgetRef->OnPressUpButton();
	}
}

void ASCGVCHUD::HandleInventoryDown()
{
	if (m_InvWidgetRef != nullptr)
	{
		m_InvWidgetRef->OnPressDownButton();
	}
}

void ASCGVCHUD::HandleInventoryLeft()
{
	if (m_InvWidgetRef != nullptr)
	{
		m_InvWidgetRef->OnPressLeftButton();
	}
}

void ASCGVCHUD::HandleInventoryRight()
{
	if (m_InvWidgetRef != nullptr)
	{
		m_InvWidgetRef->OnPressRightButton();
	}
}

void ASCGVCHUD::HandleInventoryAction()
{
	if (m_InvWidgetRef != nullptr)
	{
		m_InvWidgetRef->OnPressActionButton();
	}
}

void ASCGVCHUD::HandleDebugMenuUp()
{
	if (m_DebugMenuWidgetRef != nullptr)
	{
		m_DebugMenuWidgetRef->OnPressUpButton();
	}
}

void ASCGVCHUD::HandleDebugMenuDown()
{
	if (m_DebugMenuWidgetRef != nullptr)
	{
		m_DebugMenuWidgetRef->OnPressDownButton();
	}
}

void ASCGVCHUD::HandleDebugMenuAction()
{
	if (m_DebugMenuWidgetRef != nullptr)
	{
		m_DebugMenuWidgetRef->OnPressActionButton();
	}
}

void ASCGVCHUD::ShowDescriptionText(const FString& newtitle, const FString& newtext)
{
	if (m_InvWidgetRef != nullptr && newtext.Len() >= 1)
	{
		m_IsDescriptionVisible = true;
		m_InvWidgetRef->OnShowDescriptionText(newtitle, newtext);
	}
}

void ASCGVCHUD::HideDescriptionText(bool instant)
{
	if (m_InvWidgetRef != nullptr)
	{
		if (m_IsDescriptionVisible)
			m_IsDescriptionVisible = false;
		else
			instant = true;
		m_InvWidgetRef->OnHideDescriptionText(instant);
	}
}

bool ASCGVCHUD::IsDescriptionVisible()
{
	return m_IsDescriptionVisible;
}

void ASCGVCHUD::HideInventory()
{
	if (m_InvWidgetRef == nullptr)
		return;

	m_IsInventoryOpen = false;
	m_IsDescriptionVisible = false;
	m_InvWidgetRef->OnHide();

	for (int k = 0; k < m_InvWidgetRef->m_ItemsArray.Num(); k++)
	{
		if (m_InvWidgetRef->m_ItemsArray[k]->IsValidLowLevel())
			m_InvWidgetRef->m_ItemsArray[k] = nullptr;
	}

	if (m_InvWidgetRef->m_ItemsArray.Num() > 0)
		m_InvWidgetRef->m_ItemsArray.Empty();
}

void ASCGVCHUD::ShowInventory()
{
	if (m_InvWidgetRef == nullptr)
		return;

	ASCGVCCharacter* VCPC = Cast<ASCGVCCharacter>(GetOwningPlayerController()->GetPawn());

	if (VCPC == nullptr)
		return;

	m_IsInventoryOpen = true;

	m_InvWidgetRef->m_ItemsArray = VCPC->GetInventory();

	m_InvWidgetRef->OnShow();
}

void ASCGVCHUD::ShowButtonInfo(bool showActionButton, bool showRotationButton, bool showInfoButton, bool showBackButton)
{
	if (m_InvWidgetRef == nullptr)
		return;

	m_InvWidgetRef->OnShowButtonPanel(showActionButton, showRotationButton, showInfoButton, showBackButton);
}

void ASCGVCHUD::HideButtoninfo()
{
	if (m_InvWidgetRef == nullptr)
		return;

	m_InvWidgetRef->OnHideButtonPanel();
}

void ASCGVCHUD::ShowInventoryButtonInfo()
{
	if (m_InvWidgetRef == nullptr)
		return;

	m_InvWidgetRef->OnShowInventoryButtonPanel();
}

void ASCGVCHUD::HideInventoryButtoninfo()
{
	if (m_InvWidgetRef == nullptr)
		return;

	m_InvWidgetRef->OnHideInventoryButtonPanel();
}

void ASCGVCHUD::SetUseAndBackInfoButtonsText(FText usetext, FText backtext)
{
	if (m_InvWidgetRef == nullptr)
		return;

	m_InvWidgetRef->m_UseButtonInfoText = usetext;
	m_InvWidgetRef->m_BackButtonInfoText = backtext;
}

void ASCGVCHUD::ResetUseAndBackInfoButtonsText()
{
	if (m_InvWidgetRef == nullptr)
		return;

	m_InvWidgetRef->m_UseButtonInfoText = m_DefaultUseButtonInfoText;
	m_InvWidgetRef->m_BackButtonInfoText = m_DefaultBackButtonInfoText;
}

void ASCGVCHUD::HideDebugMenu()
{
	if (m_DebugMenuWidgetRef == nullptr)
		return;

	m_IsDebugMenuOpen = false;
	m_DebugMenuWidgetRef->OnHide();
}

void ASCGVCHUD::ShowDebugMenu()
{
	if (m_DebugMenuWidgetRef == nullptr)
		return;

	m_IsDebugMenuOpen = true;

	m_DebugMenuWidgetRef->OnShow();
}

void ASCGVCHUD::ShowPauseMenu()
{
	// Close everything else
	HideDescriptionText();
	HideInventory();

	// Do not push the PauseMenuClass twice, or when any other menu is up for that matter...
	if (!GetCurrentMenu())
	{
		PushMenu(PauseMenuClass);
	}
}

void ASCGVCHUD::HidePauseMenu()
{
	if (!GetCurrentMenu())
	{
		PopMenu();
	}
}

void ASCGVCHUD::PlayUISaveAnimation()
{
	if (m_InvWidgetRef == nullptr)
		return;

	m_InvWidgetRef->OnPlaySaveAnimation();
}