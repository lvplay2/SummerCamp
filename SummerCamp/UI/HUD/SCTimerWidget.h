// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "UI/SCUserWidget.h"
#include "SCTimerWidget.generated.h"

/**
 * @class USCTimerWidget
 */
UCLASS()
class SUMMERCAMP_API USCTimerWidget
: public USCUserWidget
{
	GENERATED_UCLASS_BODY()

public:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Animation")
	void ShowGameTimer(bool bFromTimeToShow = false);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Animation")
	void HideGameTimer();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Animation")
	void ShowPoliceTimer();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Animation")
	void HidePoliceTimer();

	UFUNCTION(BlueprintPure, Category = "WalkieTalkie")
	ESlateVisibility GetWalkieTalkieVisibility() const;


protected:
	UPROPERTY(BlueprintReadOnly, Category = "Timer")
	FText TimerText;

	UPROPERTY(BlueprintReadOnly, Category = "Timer")
	FText PoliceTimerText;

	UPROPERTY(EditDefaultsOnly, Category = "Timer")
	int32 TimeToShow;

private:
	bool bShowTimer;
	bool bShowPoliceTimer;
};
