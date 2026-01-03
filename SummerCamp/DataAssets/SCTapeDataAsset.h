// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Engine/DataAsset.h"
#include "SCTapeDataAsset.generated.h"

/**
* 
*/
UCLASS(MinimalAPI, const, Blueprintable, BlueprintType)
class USCTapeDataAsset 
	: public UDataAsset
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	FText TapeName;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	USoundBase* TapeAudio;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	USoundBase* TapeAudioJA;

	/** @return PamelaTapeAudio for a USCPamelaTapeDataAsset class. */
	UFUNCTION(BlueprintPure, Category = "Tape")
	static USoundBase* GetTapeAudio(TSubclassOf<USCTapeDataAsset> TapeClass)
	{
		if (TapeClass)
		{
#if PLATFORM_PS4
			if (USCGameBlueprintLibrary::IsJapaneseSKU(this))
			{
				// Grab what language culture this is to then set the sound wave path to.
				FCultureRef Culture = FInternationalization::Get().GetCurrentCulture();
				FString CultureString = Culture->GetTwoLetterISOLanguageName().ToUpper();

				// English is our default culture lets play the sound and exit.
				if (CultureString.Equals("JA"))
				{
					return TapeClass->GetDefaultObject<USCTapeDataAsset>()->TapeAudioJA;
				}
			}
#endif
			return TapeClass->GetDefaultObject<USCTapeDataAsset>()->TapeAudio;
		}

		return nullptr;
	}

	UFUNCTION(BlueprintPure, Category = "Tape")
	static FText GetTapeName(TSubclassOf<USCTapeDataAsset> TapeClass)
	{
		if (TapeClass)
		{
			return TapeClass->GetDefaultObject<USCTapeDataAsset>()->TapeName;
		}

		return FText();
	}
};
