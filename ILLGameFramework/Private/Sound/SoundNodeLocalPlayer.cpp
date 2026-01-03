// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "SoundNodeLocalPlayer.h"

#include "SoundDefinitions.h"
#include "ILLCharacter.h"

TMap<uint32, bool> USoundNodeLocalPlayer::LocallyControlledActorCache;

USoundNodeLocalPlayer::USoundNodeLocalPlayer(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void USoundNodeLocalPlayer::ParseNodes(FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances)
{
	bool bLocallyControlled = false;
	if (bool* LocallyControlledPtr = LocallyControlledActorCache.Find(ActiveSound.GetOwnerID()))
	{
		bLocallyControlled = *LocallyControlledPtr;
	}
	else
	{
	//	UE_LOG(LogAudio, Warning, TEXT("Unknown Owner '%s' in USoundNodeLocalPlayer."), *ActiveSound.GetOwnerName());
	}

	const int32 PlayIndex = bLocallyControlled ? 0 : 1;

	if (PlayIndex < ChildNodes.Num() && ChildNodes[PlayIndex])
	{
		ChildNodes[PlayIndex]->ParseNodes(AudioDevice, GetNodeWaveInstanceHash(NodeWaveInstanceHash, ChildNodes[PlayIndex], PlayIndex), ActiveSound, ParseParams, WaveInstances);
	}
}

#if WITH_EDITOR
FText USoundNodeLocalPlayer::GetInputPinName(int32 PinIndex) const
{
	return (PinIndex == 0) ? FText::FromString(TEXT("Local")) : FText::FromString(TEXT("Remote"));
}
#endif

int32 USoundNodeLocalPlayer::GetMaxChildNodes() const
{
	return 2;
}

int32 USoundNodeLocalPlayer::GetMinChildNodes() const
{
	return 2;
}
