// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCGameMode_Offline.h"
#include "SCGameMode_SPChallenges.generated.h"

class USCMapRegistry;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSCOnMatchEnd);

/**
 * @class ASCGameMode_SPChallenges
 */
UCLASS(Config=Game)
class SUMMERCAMP_API ASCGameMode_SPChallenges
: public ASCGameMode_Offline
{
	GENERATED_UCLASS_BODY()

public:
	// Begin AActor interface
	virtual void Tick(float DeltaSeconds) override;
	// End AActor interface

	// Begin AGameMode interface
	virtual void StartPlay() override;
	virtual void EndMatch() override;
	// End AGameMode interface

	/** Bindable event for end match to clean up any Blueprint needs */
	UPROPERTY(BlueprintAssignable)
	FSCOnMatchEnd OnMatchEnd;

protected:
	// Begin AILLGameMode interface
	virtual void GameSecondTimer() override;
	// End AILLGameMode interface

	// Begin ASCGameMode interface
	virtual void HandlePreMatchIntro() override;
	virtual bool CharacterEscaped(AController* CounselorController, ASCCounselorCharacter* CounselorCharacter) override;
	// End ASCGameMode interface
};
