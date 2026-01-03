// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLPartyLocalMessage.h"

#include "ILLPartyBeaconMemberState.h"
#include "ILLPlayerController.h"

#define LOCTEXT_NAMESPACE "ILLFonic.IGF.ILLPartyLocalMessage"

UILLPartyNotificationMessage::UILLPartyNotificationMessage(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

UILLPartyLocalMessage::UILLPartyLocalMessage(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void UILLPartyLocalMessage::ClientReceive(const FClientReceiveData& ClientData) const
{
	AILLPartyBeaconMemberState* MemberState = Cast<AILLPartyBeaconMemberState>(ClientData.OptionalObject);

	FText LocalizedMessage;
	switch (static_cast<EMessage>(ClientData.MessageIndex))
	{
	case EMessage::Created: LocalizedMessage = LOCTEXT("PartyCreated", "Created party"); break;
	case EMessage::Entered: LocalizedMessage = LOCTEXT("PartyJoined", "Joined party"); break;
	case EMessage::Closed: LocalizedMessage = LOCTEXT("PartyClosed", "Closed party"); break;
	case EMessage::Left: LocalizedMessage = LOCTEXT("PartyLeft", "Left party"); break;
	case EMessage::NewMember: LocalizedMessage = FText::Format(LOCTEXT("PartyNewMember", "{0} joined the party"), FText::FromString(MemberState->GetDisplayName())); break;
	case EMessage::MemberLeft: LocalizedMessage = FText::Format(LOCTEXT("PartyMemberLeft", "{0} left the party"), FText::FromString(MemberState->GetDisplayName())); break;
	}

	if (!LocalizedMessage.IsEmpty())
	{
		if (AILLPlayerController* PC = Cast<AILLPlayerController>(ClientData.LocalPC))
		{
			UILLPartyNotificationMessage* Message = NewObject<UILLPartyNotificationMessage>();
			Message->NotificationText = LocalizedMessage;
			Message->PartyMemberState = MemberState;
			PC->HandleLocalizedNotification(Message);
		}
	}
}

#undef LOCTEXT_NAMESPACE
