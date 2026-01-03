// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCUserWidget.h"
#include "StatModifiedValue.h"
#include "SCInteractMinigameWidget.generated.h"

class UImage;
class UCanvasPanel;
struct FInteractMinigamePip;

/**
 * @class USCInteractMinigameWidget
 */
UCLASS()
class SUMMERCAMP_API USCInteractMinigameWidget
: public USCUserWidget
{
	GENERATED_UCLASS_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	float GetHoldTimer();

	UPROPERTY(BlueprintReadWrite, Category="Interaction")
	UImage* ProgressImage;

	UPROPERTY(BlueprintReadWrite, Category="Interaction")
	UCanvasPanel* RootCanvas;

	UPROPERTY()
	FText InteractionText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
	FLinearColor ProgressColor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
	FLinearColor PipActiveColor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
	FLinearColor PipHitColor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
	FLinearColor PipMissColor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
	FLinearColor PipDefaultColor;

	void Start();
	void Stop();
	void OnInteractSkill(int InteractKey);

	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsCurrentPipActive() const;

	UFUNCTION(BlueprintPure, Category = "Interaction")
	int GetCurrentPipKey() const;

	UPROPERTY(EditDefaultsOnly, Category = "Randomization")
	int MaxPipCount;

	UPROPERTY(EditDefaultsOnly, Category = "Randomization")
	int MinPipCount;

	UPROPERTY(EditDefaultsOnly, Category = "Randomization")
	float MaxPipWidth;

	UPROPERTY(EditDefaultsOnly, Category = "Randomization")
	float MinPipWidth;

	UPROPERTY(EditDefaultsOnly, Category = "Randomization")
	FStatModifiedValue MinPipCountMod;

	UPROPERTY(EditDefaultsOnly, Category = "Randomization")
	FStatModifiedValue MaxPipCountMod;

	UPROPERTY(EditDefaultsOnly, Category = "Randomization")
	FStatModifiedValue MinPipWidthMod;

	UPROPERTY(EditDefaultsOnly, Category = "Randomization")
	FStatModifiedValue MaxPipWidthMod;

	UFUNCTION(BlueprintPure, Category = "Interaction")
	FLinearColor GetActivePipColor() const;

	/** Called whenever a player presses the correct button on a pip and is allowed to move forward */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnSuccess();

	/** Called whenever a player fails to press the correct button (or any button) on a pip and is sent backwards */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnFailure();

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:

	void NextPip();
	void PrevPip();
	void SetPipColors(float Percentage);

	UPROPERTY()
	UMaterialInstance* ProgressCircleMaterial;

	UPROPERTY()
	UMaterialInstance* ProgressCircleActiveMaterial;

	TDoubleLinkedList<FInteractMinigamePip*> InteractPips;

	FInteractMinigamePip* ActivePip;

	UPROPERTY()
	UAudioComponent* LoopSound;

	int32 ConsecutiveFails;

	float LastPipEnd;
};
