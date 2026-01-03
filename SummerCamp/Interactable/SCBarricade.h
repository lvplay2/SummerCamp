// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "SCAnimDriverComponent.h"
#include "SCBarricade.generated.h"

class USCScoringEvent;

enum class EILLInteractMethod : uint8;

/**
 * @class ASCBarricade
 */
UCLASS()
class SUMMERCAMP_API ASCBarricade
: public AActor
{
	GENERATED_BODY()

public:
	ASCBarricade(const FObjectInitializer& ObjectInitializer);

	// BEGIN AActor interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// END AActor interface

	/** Interaction method, called from LinkedDoor only */
	UFUNCTION()
	void OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod);

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USceneComponent* Root;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UStaticMeshComponent* DestroyedMesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCAnimDriverComponent* UseBarricadeDriver;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	class USCSpecialMoveComponent* CloseBarricade_SpecialMoveHandler;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	class USCSpecialMoveComponent* OpenBarricade_SpecialMoveHandler;

	UFUNCTION(BlueprintCallable, Category = "Default")
	void SetEnabled(bool Enable);

	UFUNCTION(BlueprintCallable, Category = "Default")
	void BreakAndDestroy();

	UFUNCTION(BlueprintCallable, Category = "Default")
	bool IsEnabled() const;

	UFUNCTION()
	void OnUseStarted(ASCCharacter* Interactor);

	UFUNCTION()
	void OnUsedBarricade(ASCCharacter* Interactor);

	UFUNCTION()
	virtual void OnBarricadeAborted(ASCCharacter* Interactor);

	UFUNCTION()
	virtual void UseBarricadeDriverTick(FAnimNotifyData NotifyData);

	UPROPERTY()
	class ASCDoor* LinkedDoor;

	/** Score Event for barricading the door */
	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	TSubclassOf<USCScoringEvent> BarricadeDoorScoreEvent;

	/** Audio to play when The barricade is hit */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue* DamagedAudio;

	/** Audio to play when the barricade is set */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue* EnableBarricadeAudio;

	/** Audio to play when the barricade is removed */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue* DisableBarricadeAudio;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Enabled, BlueprintReadOnly, Category = "Default")
	bool bEnabled;

	UFUNCTION()
	virtual void OnRep_Enabled();

	// If true, someone is lowering the barricade right now
	UPROPERTY(ReplicatedUsing = OnRep_Enabled, BlueprintReadOnly, Category = "Default")
	bool bIsBarricading;

	UPROPERTY(ReplicatedUsing=OnRep_Destroyed)
	bool bDestroyed;

	UFUNCTION()
	virtual void OnRep_Destroyed();

	UPROPERTY(Replicated, Transient)
	FRotator OriginalRotation;
};
