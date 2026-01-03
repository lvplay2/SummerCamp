// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Blueprint/UserWidget.h"

#include "SCGVCInventorySlotWidget.generated.h"

/**
 * 
 */
UCLASS()
class SUMMERCAMP_API USCGVCInventorySlotWidget
: public UUserWidget
{
	GENERATED_BODY()

protected:
	//Holds a reference to the item icon

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UTexture2D* m_ItemIcon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	ASCGVCItem* m_Item;

	UFUNCTION(BlueprintCallable, Category = "VC UI")
	void SetEquippedItemOnPC();

	UFUNCTION(BlueprintCallable, Category = "VC UI")
	void SendActionOnEquippedItemOnPC();

	UFUNCTION(BlueprintNativeEvent, Category = "VC UI")
	void OnClearedItem();

	UFUNCTION(BlueprintNativeEvent, Category = "VC UI")
	void OnSetItem();

public:
	UFUNCTION(BlueprintCallable, Category = "VC UI")
	void SetSlotItem(ASCGVCItem* Item);

	UFUNCTION(BlueprintCallable, Category = "VC UI")
	void ClearSlotItem();
};
