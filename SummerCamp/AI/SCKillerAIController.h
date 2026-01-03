// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "SCCharacterAIController.h"
#include "SCKillerAIController.generated.h"

class USCCharacterAIProperties;
class USCJasonAIProperties;

/**
 * @class ASCKillerAIController
 */
UCLASS(Config=Game)
class SUMMERCAMP_API ASCKillerAIController
: public ASCCharacterAIController
{
	GENERATED_UCLASS_BODY()
	
	// Begin ASCCharacterAIController interface
public:
	virtual void SetBotProperties(TSubclassOf<USCCharacterAIProperties> PropertiesClass) override;
	// End ASCCharacterAIController interface
};
