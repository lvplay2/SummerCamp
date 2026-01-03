// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLCharacterAnimInstance.h"

#include "Animation/AnimMontage.h"
#include "AnimInstanceProxy.h"

TAutoConsoleVariable<int32> CVarILLDebugAnimation(
	TEXT("ill.DebugAnimation"),
	0,
	TEXT("Displays debug information for locomotion states on the local actor.\n")
	TEXT(" 0: None\n")
	TEXT(" 1: On Screen (5 local+remote, 9 remote only)\n")
	TEXT(" 2: Log (6 local+remote, 10 remote only)\n")
	TEXT(" 3: Both (7 local+remote, 11 remote only)\n")
);

TAutoConsoleVariable<float> CVarILLDebugAnimationLingerTime(
	TEXT("ill.DebugAnimationLingerTime"),
	2.f,
	TEXT("Time (in seconds) to display state changes on screen")
);

UILLCharacterAnimInstance::UILLCharacterAnimInstance(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, bAutomaticallyCollectRawCurves(false)
, RawCurvesLastUpdated_TimeStamp(0.f)
{
}

void UILLCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

}

void UILLCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// Updating PLAYER on SERVER/CLIENT (LOCAL?)
	if (const ACharacter* const Character = Cast<const ACharacter>(TryGetPawnOwner()))
		Log(FColor::Yellow, false, FString::Printf(TEXT("Updating %s on %s (%s)"), *GetNameSafe(Character), Character->Role == ROLE_Authority ? TEXT("server") : TEXT("client"), Character->IsLocallyControlled() ? TEXT("local") : TEXT("not local")));

	if (bAutomaticallyCollectRawCurves)
	{
		UpdateRawCurves();
	}
}

void UILLCharacterAnimInstance::NativeUninitializeAnimation()
{
	Super::NativeUninitializeAnimation();

	UnblendedCurveValues.Empty();
	RawCurvesLastUpdated_TimeStamp = 0.f;
}

void UILLCharacterAnimInstance::UpdateRawCurves()
{
	// TODO: Make sure the sequence is actually being played and not blocked by a montage slot
	// CLEANUP: Replace TMap<>.Contains with TMap<>.Find to remove double searches when the key exists

	/* ----------------------------------------------------------- */
	// Reading raw curves is hard. Let's go on a journey together.

	// This is the unblended, maximum curve value for the current frame
	UnblendedCurveValues.Empty();

	const auto ReadCurvesFromSequenceAsset = [this](const UAnimSequenceBase* Sequence, const float FrameTime, TMap<FName, float>& OutCurvesForThisAsset)
	{
		if (!Sequence)
			return;

		Log(FColor::Cyan, false, FString::Printf(TEXT(" -- Sequence %s @ %.2fs (%d float curves)"), *GetNameSafe(Sequence), FrameTime, Sequence->RawCurveData.FloatCurves.Num()));

		// Read the curves
		for (const FFloatCurve& Curve : Sequence->RawCurveData.FloatCurves)
		{
			// Set the value in the map to the largest value for the curve we can find
			const float CurveValue = Curve.Evaluate(FrameTime);
			if (OutCurvesForThisAsset.Contains(Curve.Name.DisplayName))
			{
				OutCurvesForThisAsset[Curve.Name.DisplayName] = FMath::Max(OutCurvesForThisAsset[Curve.Name.DisplayName], CurveValue);
			}
			else
			{
				OutCurvesForThisAsset.Add(Curve.Name.DisplayName, CurveValue);
			}
		}
	};

	// Lambda to sort through an array of assets and read the curves
	const auto ReadCurvesFromSequenceArray = [this, &ReadCurvesFromSequenceAsset](const TArray<FAnimTickRecord>& TickRecords)
	{
		for (const FAnimTickRecord& Record : TickRecords)
		{
			if (!Record.TimeAccumulator || !Record.SourceAsset)
				continue;

			TMap<FName, float> CurvesForThisAsset;
			float FrameTime = *Record.TimeAccumulator;
			const UAnimSequenceBase* Sequence = Cast<const UAnimSequenceBase>(Record.SourceAsset);

			// We have a single asset we can operate on
			if (Sequence)
			{
				ReadCurvesFromSequenceAsset(Sequence, FrameTime, CurvesForThisAsset);
			}
			// We may not have a single asset, so check for a blendspace
			else
			{
				// This asset may also be a montage, but for basic locomotion we don't want to handle that. For now.
				if (Record.BlendSpace.BlendSampleDataCache != nullptr)
				{
					Log(FColor::Cyan, false, FString::Printf(TEXT(" -- BlendSpace %s (W: %.3f  I: %.1f, %.1f)"), *GetNameSafe(Record.SourceAsset), Record.EffectiveBlendWeight, Record.BlendSpace.BlendSpacePositionX, Record.BlendSpace.BlendSpacePositionY));

					for (const FBlendSampleData& Sample : *Record.BlendSpace.BlendSampleDataCache)
					{
						if (!Sample.Animation)
							continue;

						FrameTime = Sample.Time;
						const float BlendWeight = Sample.GetWeight();

						Log(FColor::Cyan, false, FString::Printf(TEXT(" --- Sequence %s (W: %.3f) @ %.2fs (%d float curves)"), *GetNameSafe(Sample.Animation), BlendWeight, FrameTime, Sample.Animation->RawCurveData.FloatCurves.Num()));
						for (const FFloatCurve& Curve : Sample.Animation->RawCurveData.FloatCurves)
						{
							// For a blendspace we want to blend our curve values together with any playing assets
							const float CurveValue = Curve.Evaluate(FrameTime) * BlendWeight;
							if (CurvesForThisAsset.Contains(Curve.Name.DisplayName))
							{
								CurvesForThisAsset[Curve.Name.DisplayName] += CurveValue;
							}
							else
							{
								CurvesForThisAsset.Add(Curve.Name.DisplayName, CurveValue);
							}
						}
					}
				}
			}

			// Merge all the values we found into the master list
			for (const TPair<FName, float>& Entry : CurvesForThisAsset)
			{
				if (UnblendedCurveValues.Contains(Entry.Key))
				{
					UnblendedCurveValues[Entry.Key] = FMath::Max(UnblendedCurveValues[Entry.Key], Entry.Value);
					Log(FColor::Emerald, false, FString::Printf(TEXT(" -- Curve %s @ %.2f (max %.2f)"), *Entry.Key.ToString(), Entry.Value, UnblendedCurveValues[Entry.Key]));
				}
				else
				{
					UnblendedCurveValues.Add(Entry.Key, Entry.Value);
					Log(FColor::Emerald, false, FString::Printf(TEXT(" -- Curve %s @ %.2f"), *Entry.Key.ToString(), Entry.Value));
				}
			}
		}
	};

	// Read all the curves in the ungrouped list
	FAnimInstanceProxy& Proxy = GetProxyOnGameThread<FAnimInstanceProxy>();
	Log(FColor::Yellow, false, FString::Printf(TEXT(" - Ungrouped animations")));
	ReadCurvesFromSequenceArray(Proxy.GetUngroupedActivePlayersRead());

	// Read all the curves in all the sync groups
	const TArray<FAnimGroupInstance>& SyncGroups = Proxy.GetSyncGroupRead();
	for (const FAnimGroupInstance& SyncGroup : SyncGroups)
	{
		if (CVarILLDebugAnimation.GetValueOnGameThread() > 0)
		{
			if (SyncGroup.bCanUseMarkerSync)
			{
				FString Markers;
				for (FName Name : SyncGroup.ValidMarkers)
				{
					Markers.Append(Name.ToString());
					Markers += TEXT(", ");
				}
				Markers.RemoveAt(Markers.Len() - 2, 2);
				Log(FColor::Yellow, false, FString::Printf(TEXT(" - Syncgroup (%d) uses markers (%s)"), SyncGroup.ActivePlayers.Num(), *Markers));
			}
			else
			{
				Log(FColor::Yellow, false, FString::Printf(TEXT(" - Syncgroup (%d)"), SyncGroup.ActivePlayers.Num()));
			}
		}

		ReadCurvesFromSequenceArray(SyncGroup.ActivePlayers);
	}

	// Get them montages too
	for (const auto& MontageInstance : MontageInstances)
	{
		if (MontageInstance->Montage)
			ReadCurvesFromSequenceAsset(MontageInstance->Montage, MontageInstance->GetPosition(), UnblendedCurveValues);
	}

	// Curves read! Let's use a couple of them! Maybe! (look for calls to GetMaxCurveValue)
	/* ------------------------------------------------------------------------------------ */

	RawCurvesLastUpdated_TimeStamp = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
}

void UILLCharacterAnimInstance::Log(const FColor Color, const bool bShouldLinger, const FString& DebugString) const
{
	const int32 AnimationDebugValue = CVarILLDebugAnimation.GetValueOnGameThread();
	if (AnimationDebugValue > 0)
	{
		// So inflexible
		const ACharacter* const Character = Cast<const ACharacter>(TryGetPawnOwner());
		const bool bIsLocallyControlled = Character && Character->IsLocallyControlled();

		// Remote only
		if (AnimationDebugValue & 8)
		{
			if (bIsLocallyControlled)
				return;
		}
		// Local+Remote
		else if ((AnimationDebugValue & 4) == 0 && bIsLocallyControlled == false)
		{
			return;
		}
		// else local only

		const float DisplayTime = bShouldLinger ? CVarILLDebugAnimationLingerTime.GetValueOnGameThread() : 0.f;

		const FString Output = FString::Printf(TEXT("%d. %s"), GFrameCounter, *DebugString);
		if (AnimationDebugValue & 1)
		{
			// One off messages need to be held on creen for longer
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, DisplayTime, Color, Output, false);
		}

		if (AnimationDebugValue & 2)
		{
			UE_LOG(LogTemp, Display, TEXT("%s"), *Output);
		}
	}
}
