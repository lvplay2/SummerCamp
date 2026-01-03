// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLOnlineSessionClient.h"
#include "SCOnlineSessionClient.generated.h"

class UILLLocalPlayer;

/**
 * @class USCOnlineSessionClient
 */
UCLASS()
class SUMMERCAMP_API USCOnlineSessionClient
: public UILLOnlineSessionClient
{
	GENERATED_UCLASS_BODY()

	// Begin UILLOnlineSessionClient interface
	virtual bool HostGameSession(TSharedPtr<FILLGameSessionSettings> SessionSettings) override;
	virtual void PerformSignoutAndDisconnect(UILLLocalPlayer* Player, FText FailureText) override;
	// End UILLOnlineSessionClient interface
};
