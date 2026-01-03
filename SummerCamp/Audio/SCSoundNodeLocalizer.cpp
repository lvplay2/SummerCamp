// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SummerCamp.h"

#include "SCSoundNodeLocalizer.h"
#include "ActiveSound.h"
#include "SCGameBlueprintLibrary.h"

/*-----------------------------------------------------------------------------
	USoundNodeMixer implementation.
-----------------------------------------------------------------------------*/
USCSoundNodeLocalizer::USCSoundNodeLocalizer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USCSoundNodeLocalizer::ParseNodes( FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances )
{
	RETRIEVE_SOUNDNODE_PAYLOAD(sizeof(int32));
	DECLARE_SOUNDNODE_ELEMENT(int32, NodeIndex);

	// Pick a random child node and save the index.
	bool IsJA = false;

#if PLATFORM_PS4
	if (USCGameBlueprintLibrary::IsJapaneseSKU(this))
	{
		// Grab what language culture this is to then set the sound wave path to.
		FCultureRef Culture = FInternationalization::Get().GetCurrentCulture();
		FString CultureString = Culture->GetTwoLetterISOLanguageName().ToUpper();

		// English is our default culture lets play the sound and exit.
		if (CultureString.Equals("JA"))
		{
			IsJA = true;
		}
	}
#endif

	NodeIndex = IsJA ? 1 : 0;

#if WITH_EDITOR
	bool bIsPIESound = (GEditor != nullptr) && ((GEditor->bIsSimulatingInEditor || GEditor->PlayWorld != nullptr) && ActiveSound.GetWorldID() > 0);
#endif //WITH_EDITOR

	if (NodeIndex < ChildNodes.Num() && ChildNodes[NodeIndex])
	{
		ChildNodes[NodeIndex]->ParseNodes(AudioDevice, GetNodeWaveInstanceHash(NodeWaveInstanceHash, ChildNodes[NodeIndex], NodeIndex), ActiveSound, ParseParams, WaveInstances);
	}
}

void USCSoundNodeLocalizer::CreateStartingConnectors()
{
	// Mixers default with two connectors.
	InsertChildNode( ChildNodes.Num() );
	InsertChildNode( ChildNodes.Num() );
}
