// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLPlayerStateLocalMessage.h"

#include "ILLPlayerController.h"
#include "ILLPlayerState.h"

#define LOCTEXT_NAMESPACE "ILLFonic.IGF.ILLPlayerStateLocalMessage"

UILLPlayerStateNotificationMessage::UILLPlayerStateNotificationMessage(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

UILLPlayerStateLocalMessage::UILLPlayerStateLocalMessage(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void UILLPlayerStateLocalMessage::ClientReceive(const FClientReceiveData& ClientData) const
{
	FText LocalizedMessage;
	if (AILLPlayerState* RelatedPS1 = Cast<AILLPlayerState>(ClientData.RelatedPlayerState_1))
	{
		switch (static_cast<EMessage>(ClientData.MessageIndex))
		{
		case EMessage::RegisteredWithGame: LocalizedMessage = FText::Format(LOCTEXT("RegisteredWithGame", "{0} joined"), FText::FromString(RelatedPS1->PlayerName)); break;
		case EMessage::UnregisteredWithGame: LocalizedMessage = FText::Format(LOCTEXT("UnregisteredWithGame", "{0} left"), FText::FromString(RelatedPS1->PlayerName)); break;
		}

		if (!LocalizedMessage.IsEmpty())
		{
			if (AILLPlayerController* PC = Cast<AILLPlayerController>(ClientData.LocalPC))
			{
				UILLPlayerStateNotificationMessage* Message = NewObject<UILLPlayerStateNotificationMessage>();
				Message->NotificationText = LocalizedMessage;
				Message->PlayerState = RelatedPS1;
				PC->HandleLocalizedNotification(Message);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
