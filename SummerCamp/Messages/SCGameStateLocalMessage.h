// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLGameStateLocalMessage.h"
#include "SCPlayerController.h"
#include "SCGameStateLocalMessage.generated.h"

/**
 * @class SCMatchStateNotificationMessage
 */
UCLASS()
class USCMatchStateNotificationMessage
: public UILLPlayerNotificationMessage
{
	GENERATED_BODY()
};

/**
 * @class USCGameStateLocalMessage
 */
UCLASS(Abstract)
class USCGameStateLocalMessage
: public UILLGameStateLocalMessage
{
	GENERATED_BODY()

public:
	// Begin ULocalMessage interface
	virtual void ClientReceive(const FClientReceiveData& ClientData) const override;
	// End ULocalMessage interface

	// Begin UILLGameStateLocalMessage interface
	virtual int32 SwitchForMatchState(FName NewMatchState) const override;
	// End UILLGameStateLocalMessage interface

	enum class EMessage : uint8
	{
		WaitingPreMatch,
		WaitingPostMatch,
	};
};
