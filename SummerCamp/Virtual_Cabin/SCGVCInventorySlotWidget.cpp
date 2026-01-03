// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGVCInventorySlotWidget.h"
#include "Virtual_Cabin/SCGVCItem.h"
#include "Virtual_Cabin/SCGVCCharacter.h"

void USCGVCInventorySlotWidget::SetEquippedItemOnPC()
{
	ASCGVCCharacter* PC = Cast<ASCGVCCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

	if (PC != NULL)
	{
		PC->SetEquippedItem(m_Item);
	}
}

void USCGVCInventorySlotWidget::SendActionOnEquippedItemOnPC()
{
	ASCGVCCharacter* PC = Cast<ASCGVCCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

	if (PC != NULL)
	{
		PC->SendActionOnEquippedItem();
	}
}

void USCGVCInventorySlotWidget::SetSlotItem(ASCGVCItem* Item)
{
	if (Item != NULL)
	{
		m_Item = Item;
		m_ItemIcon = Item->GetItemIcon();
		OnSetItem();
	}
	else
	{
		m_Item = nullptr;
		m_ItemIcon = nullptr;
	}
}

void USCGVCInventorySlotWidget::ClearSlotItem()
{
	m_Item = nullptr;
	m_ItemIcon = nullptr;
	OnClearedItem();
}

void USCGVCInventorySlotWidget::OnClearedItem_Implementation()
{
}

void USCGVCInventorySlotWidget::OnSetItem_Implementation()
{
}