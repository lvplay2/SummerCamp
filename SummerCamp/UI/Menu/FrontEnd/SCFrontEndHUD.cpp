// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCFrontEndHUD.h"

#include "SCGameInstance.h"
#include "SCOnlineMatchmakingClient.h"
#include "SCQuickPlayOverlayWidget.h"

ASCFrontEndHUD::ASCFrontEndHUD(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void ASCFrontEndHUD::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (USCGameInstance* GameInstance = Cast<USCGameInstance>(World->GetGameInstance()))
		{
			// Hide the loading screen
			GameInstance->HideLoadingScreen();
		}
	}
}

void ASCFrontEndHUD::BeginMatchmaking(const bool bForLAN)
{
	if (UWorld* World = GetWorld())
	{
		USCOnlineMatchmakingClient* OMC = Cast<USCOnlineMatchmakingClient>(Cast<USCGameInstance>(World->GetGameInstance())->GetOnlineMatchmaking());
		OMC->BeginMatchmaking(bForLAN);
	}
}

void ASCFrontEndHUD::UpdateLatestNews()
{
	// Only do this if we don't already have the news
	if (!LatestNews.Contents.IsEmpty())
		return;

	if (UWorld* World = GetWorld())
	{
		if (USCGameInstance* GameInstance = Cast<USCGameInstance>(World->GetGameInstance()))
		{
			// Request the latest news from the backend
			USCBackendRequestManager* BRM = GameInstance->GetBackendRequestManager<USCBackendRequestManager>();
			if (BRM->CanSendAnyRequests())
			{
				BRM->GetNews(FSCOnBackendNewsDelegate::CreateUObject(this, &ThisClass::OnGetNews));
			}
		}
	}
}

void ASCFrontEndHUD::OnGetNews(TArray<FSCNewsEntry>& NewsEntries, int32 ArticleCount)
{
	if (NewsEntries.Num() > 0)
	{
		LatestNews = NewsEntries[0];
	}
}
