// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLChatNotificationWidget.h"

#include "ILLPlayerController.h"

UILLChatNotificationWidget::UILLChatNotificationWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void UILLChatNotificationWidget::NativeConstruct()
{
	Super::NativeConstruct();

	AttemptToPopulate();
}

void UILLChatNotificationWidget::NativeDestruct()
{
	Super::NativeDestruct();

	if (AILLPlayerController* OwningPlayer = GetOwningILLPlayer())
	{
		OwningPlayer->OnPlayerReceivedNotification.RemoveAll(this);
	}
}

void UILLChatNotificationWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bPopulated)
	{
		AttemptToPopulate();
	}
}

void UILLChatNotificationWidget::OnPlayerNotificationReceived(UILLPlayerNotificationMessage* Notification)
{
	OnReceivePlayerNotification(Notification);
}

void UILLChatNotificationWidget::AttemptToPopulate()
{
	AILLPlayerController* OwningPlayer = GetOwningILLPlayer();
	if (!OwningPlayer)
		return;

	OnPopulatePlayerNotifications(OwningPlayer->GetNotificationMessages());
	OwningPlayer->OnPlayerReceivedNotification.AddDynamic(this, &ThisClass::OnPlayerNotificationReceived);
	bPopulated = true;
}
