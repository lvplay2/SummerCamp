// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLStreamingImageWidget.h"

#include "ILLGameInstance.h"

void UILLStreamingImageWidget::SetVisibility(ESlateVisibility InVisibility)
{
	Super::SetVisibility(InVisibility);

	if (!StreamedTexture.IsNull())
	{
		UWorld* World = GetWorld();
		UILLGameInstance* GameInstance = World ? World->GetGameInstance<UILLGameInstance>() : nullptr;
		if (GameInstance)
		{
			if (IsVisible())
			{
				FStreamableDelegate Delegate;
				Delegate.BindUObject(this, &UILLStreamingImageWidget::TextureLoaded);
				GameInstance->StreamableManager.RequestAsyncLoad(StreamedTexture.ToSoftObjectPath(), Delegate, 0, true, false, TEXT("ILL Slate Image Load"));
			}
			else
			{
				Brush.SetResourceObject(nullptr);
				GameInstance->StreamableManager.Unload(StreamedTexture.ToSoftObjectPath());
			}
		}
	}
}

void UILLStreamingImageWidget::TextureLoaded()
{
	if (IsVisible())
	{
		Brush.SetResourceObject(StreamedTexture.Get());
	}
	else
	{
		UWorld* World = GetWorld();
		UILLGameInstance* GameInstance = World ? World->GetGameInstance<UILLGameInstance>() : nullptr;
		if (GameInstance)
		{
			GameInstance->StreamableManager.Unload(StreamedTexture.ToSoftObjectPath());
		}
	}
}
