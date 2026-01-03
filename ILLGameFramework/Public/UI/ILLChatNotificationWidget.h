// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLUserWidget.h"
#include "ILLChatNotificationWidget.generated.h"

/**
 * @class UILLChatNotificationWidget
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLChatNotificationWidget
: public UILLUserWidget
{
	GENERATED_UCLASS_BODY()

	// Begin UUserWidget interface
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	// End UUserWidget interface

	/** Called when the first player "lookup" succeeds with the list of existing notifications. */
	UFUNCTION(BlueprintImplementableEvent, Category="Notifications")
	void OnPopulatePlayerNotifications(const TArray<UILLPlayerNotificationMessage*>& Notification);

	/** Called when a new player notification is received. */
	UFUNCTION(BlueprintImplementableEvent, Category="Notifications")
	void OnReceivePlayerNotification(UILLPlayerNotificationMessage* Notification);

protected:
	/** Delegate fired when our owning player receives a notification. */
	UFUNCTION()
	virtual void OnPlayerNotificationReceived(UILLPlayerNotificationMessage* Notification);

	/** Checks if we have our player owner yet, and when we do calls to OnPopulatePlayerNotifications. */
	virtual void AttemptToPopulate();

	// Have we synced up with our player owner?
	bool bPopulated;
};
