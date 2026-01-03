// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "GameFramework/LocalMessage.h"
#include "ILLPlayerController.h"
#include "ILLPlayerStateLocalMessage.generated.h"

/**
 * @class UILLPlayerStateNotificationMessage
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLPlayerStateNotificationMessage
: public UILLPlayerNotificationMessage
{
	GENERATED_UCLASS_BODY()

	// Player state this message relates to
	UPROPERTY(BlueprintReadOnly)
	AILLPlayerState* PlayerState;
};

/**
 * @class UILLPlayerStateLocalMessage
 */
UCLASS(Abstract)
class ILLGAMEFRAMEWORK_API UILLPlayerStateLocalMessage
: public ULocalMessage
{
	GENERATED_UCLASS_BODY()

	// Begin ULocalMessage interface
	virtual void ClientReceive(const FClientReceiveData& ClientData) const override;
	// End ULocalMessage interface

	enum class EMessage : uint8
	{
		RegisteredWithGame,
		UnregisteredWithGame,
		MAX
	};
};
