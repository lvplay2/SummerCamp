// Copyright (C) 2015-2018 IllFonic, LLC.

#include "SummerCamp.h"
#include "SCKillerAIController.h"

#if WITH_EDITOR
# include "MessageLog.h"
# include "UObjectToken.h"
#endif

#include "SCCharacterAIProperties.h"

ASCKillerAIController::ASCKillerAIController(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void ASCKillerAIController::SetBotProperties(TSubclassOf<USCCharacterAIProperties> PropertiesClass)
{
	Super::SetBotProperties(PropertiesClass);

	if (PropertiesClass && !GetBotPropertiesInstance<USCJasonAIProperties>() && GetPawn())
	{
#if WITH_EDITOR
		FMessageLog("PIE").Warning()
			->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("No killer AI properties set for \"%s\""), *GetPawn()->GetName()))))
			->AddToken(FUObjectToken::Create(GetPawn()->GetClass()))
			->AddToken(FTextToken::Create(FText::FromString(TEXT(", using default values in code."))));
#endif
		UE_LOG(LogSCCharacterAI, Error, TEXT("ASCKillerAIController::SetBotProperties: No killer AI properties set for \"%s\", using default values in code."), *GetPawn()->GetName());
	}
}
