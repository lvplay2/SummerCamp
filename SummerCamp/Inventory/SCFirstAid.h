// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCItem.h"

#include "SCFirstAid.generated.h"

class ASCCounselorCharacter;
class USCScoringEvent;
class USCStatBadge;

/**
 * @class ASCFirstAid
 */
UCLASS()
class SUMMERCAMP_API ASCFirstAid
: public ASCItem
{
	GENERATED_UCLASS_BODY()

public:
	// BEGIN ASCItem interface
	virtual bool Use(bool bFromInput) override;
	virtual bool CanUse(bool bFromInput) override;
	virtual UAnimMontage* GetUseItemMontage(const USkeleton* Skeleton) const;
	// END ASCItem interface

	// BEGIN AILLInventoryItem interface
	virtual void AddedToInventory(UILLInventoryComponent* Inventory) override;
	// END AILLInventoryItem interface

	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	TSubclassOf<USCScoringEvent> UseFirstAidScoreEvent;

	UPROPERTY(EditDefaultsOnly, Category = "Scoring|Badges")
	TSubclassOf<USCStatBadge> HealedSelfBadge;

	UPROPERTY(EditDefaultsOnly, Category = "Scoring|Badges")
	TSubclassOf<USCStatBadge> HealedOtherBadge;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* HealOtherMontage;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Healing", meta = (UIMin = "0.0", UIMax = "100.0"))
	float HealthRestoredPerUse;

	UPROPERTY()
	ASCCounselorCharacter* CounselorHealed;

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_SetCounselorHealed(ASCCounselorCharacter* Counselor);

	bool bPerkBonusApplied;
};
