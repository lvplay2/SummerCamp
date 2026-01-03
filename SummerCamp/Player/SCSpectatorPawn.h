// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/SpectatorPawn.h"
#include "SCSpectatorPawn.generated.h"

class ASCCharacter;

/**
 * @class ASCSpectatorPawn
 */
UCLASS()
class SUMMERCAMP_API ASCSpectatorPawn 
: public ASpectatorPawn
{
	GENERATED_UCLASS_BODY()

	// Begin APawn Interface
	virtual void OnRep_Controller() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	virtual void MoveForward(float Val) override;
	virtual void MoveRight(float Val) override;
	virtual void MoveUp_World(float Val) override;
	virtual void AddControllerPitchInput(float Val) override;
	virtual void AddControllerYawInput(float Val) override;
	virtual void Tick(float DeltaSeconds) override;
	// End APawn Interface

	virtual void OnReceivedController();

	void StartShowScoreboard();
	void StopShowScoreboard();
	void ToggleHUD();
	void ToggleSpectatorShowAll();

private:
	bool bToggleHUD;
	bool bToggleShowAll;

	/** Have we been detached from the player we are spectating (deatch happens when a player escapes) */
	bool bDetachedFromViewingPlayer;

	/** How far back we are from the player we are spectating */
	float ViewingOffset;

	/** The player we are currently spectating */
	UPROPERTY()
	class ASCCharacter* Viewing;

	/** Spectator camera component */
	UPROPERTY()
	class UCameraComponent* SpectatorCamera;

	/** Spring arm for camera component */
	UPROPERTY()
	class USpringArmComponent* SpringArm;

public:
	APostProcessVolume* GetPostProcessVolume() { return ForcedPostProcessVolume; }
private:
	UPROPERTY()
	APostProcessVolume* ForcedPostProcessVolume;

};
