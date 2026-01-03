// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "GameFramework/LocalMessage.h"
#include "ILLPlayerController.h"
#include "ILLPartyLocalMessage.generated.h"

/**
 * @class UILLPartyNotificationMessage
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLPartyNotificationMessage
: public UILLPlayerNotificationMessage
{
	GENERATED_UCLASS_BODY()

	// Party member this message relates to
	UPROPERTY(BlueprintReadOnly)
	AILLPartyBeaconMemberState* PartyMemberState;
};

/**
 * @class UILLPartyLocalMessage
 */
UCLASS(Abstract)
class ILLGAMEFRAMEWORK_API UILLPartyLocalMessage
: public ULocalMessage
{
	GENERATED_UCLASS_BODY()

	// Begin ULocalMessage interface
	virtual void ClientReceive(const FClientReceiveData& ClientData) const override;
	// End ULocalMessage interface

	enum class EMessage : uint8
	{
		Created, // Local player has created a party
		Entered, // Local player has entered a party
		Closed, // Local player closed the party
		Left, // Local player left a party
		NewMember, // Another member has joined the party
		MemberLeft, // Another member has left the party
		MAX
	};
};
