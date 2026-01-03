// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGameStateLocalMessage.h"

#include "SCGameMode.h"

#define LOCTEXT_NAMESPACE "ILLFonic.F13.SCGameStateLocalMessage"

void USCGameStateLocalMessage::ClientReceive(const FClientReceiveData& ClientData) const
{
	if (ClientData.MessageIndex < 0)
		return;

	FText LocalizedMessage;
	switch (static_cast<EMessage>(ClientData.MessageIndex))
	{
	case EMessage::WaitingPreMatch: LocalizedMessage = LOCTEXT("WaitingPreMatch", "Match starting"); break;
	case EMessage::WaitingPostMatch: LocalizedMessage = LOCTEXT("POstMatchOutroWait", "Match ending"); break;
	}

	if (!LocalizedMessage.IsEmpty())
	{
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(ClientData.LocalPC))
		{
			USCMatchStateNotificationMessage* Message = NewObject<USCMatchStateNotificationMessage>();
			Message->NotificationText = LocalizedMessage;
			PC->HandleLocalizedNotification(Message);
		}
	}
}

int32 USCGameStateLocalMessage::SwitchForMatchState(FName NewMatchState) const
{
	if (NewMatchState == MatchState::WaitingPreMatch)
	{
		return static_cast<int32>(EMessage::WaitingPreMatch);
	}
	else if (NewMatchState == MatchState::WaitingPostMatch)
	{
		return static_cast<int32>(EMessage::WaitingPostMatch);
	}

	return Super::SwitchForMatchState(NewMatchState);
}

#undef LOCTEXT_NAMESPACE
