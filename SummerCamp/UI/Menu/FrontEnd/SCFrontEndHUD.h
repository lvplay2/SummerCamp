// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCBackendRequestManager.h"
#include "SCHUD.h"
#include "SCOnlineSessionClient.h"
#include "SCFrontEndHUD.generated.h"

class USCQuickPlayOverlayWidget;

/**
 * @class ASCFrontEndHUD
 */
UCLASS()
class SUMMERCAMP_API ASCFrontEndHUD
: public ASCHUD
{
	GENERATED_UCLASS_BODY()

	// Begin AActor interface
	virtual void BeginPlay() override;
	// End AActor interface

	/** Begin QuickPlay matchmaking. */
	UFUNCTION(BlueprintCallable, Category="QuickPlay")
	virtual void BeginMatchmaking(const bool bForLAN);

	//////////////////////////////////////////////////
	// News
public:
	/** Request the latest news from the backend. */
	UFUNCTION(BlueprintCallable, Category="News")
	virtual void UpdateLatestNews();

protected:
	/** Delegate called when we get the latest news from the backend. */
	virtual void OnGetNews(TArray<FSCNewsEntry>& NewsEntries, int32 ArticleCount);

	// Latest news from the backend
	UPROPERTY(BlueprintReadOnly, Transient, Category="News")
	FSCNewsEntry LatestNews;
};
