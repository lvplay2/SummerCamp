// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCFearComponent.h"
#include "SCCounselorCharacter.h"
#include "SCPlayerState.h"
#include "SCPerkData.h"

TAutoConsoleVariable<int32> CVarDebugFear(TEXT("sc.DebugFear"), 0,
										  TEXT("Displays debug information for fear.\n")
										  TEXT(" 0: None\n")
										  TEXT(" 1: Display debug info"));

void USCFearComponent::LockFearCMD::Register()
{
	LockFearCmdHandle = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("sc.LockFearTo"),
		TEXT("Set fear to specified level and lock it there."),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &LockFearCMD::LockFearToCMD),
		ECVF_Cheat
	);

	bRegistered = true;
}

void USCFearComponent::LockFearCMD::UnRegister()
{
	IConsoleManager::Get().UnregisterConsoleObject(LockFearCmdHandle);
	bRegistered = false;
}

USCFearComponent::USCFearComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, LockFearCommand(this)
{
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_DuringPhysics;
	bAutoActivate = true;

	bNetAddressable = true;
	bReplicates = true;

	UpdateFearDelay = 1.f/20.f; // Process fear updates at 20FPS

	FearAmount.SetMaxValue(100.0f);
	FearAmount = 0.0f;
}

USCFearComponent::~USCFearComponent()
{
	LockFearCommand.UnRegister();
}

void USCFearComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USCFearComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USCFearComponent, FearAmount);
	DOREPLIFETIME(USCFearComponent, bLockFearLevel);
}

void USCFearComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (bLockFearLevel) // If fear is locked, attempt to display the current locked fear and early out of function
	{
		if (CVarDebugFear.GetValueOnGameThread() > 0 && IsLocallyControlled())
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Purple, FString::Printf(TEXT("Fear Locked: %.2f"), (float)FearAmount), true, FVector2D(1.2f, 1.2f));

		return;
	}

	// Check our FPS throttle
	const float RealDT = DeltaTime;
	if (UpdateFearDelay > 0.f && CVarDebugFear.GetValueOnGameThread() <= 0) // Don't throttle if debug is on. Makes it hard to read!
	{
		TimeUntilFearUpdate -= DeltaTime;
		if (TimeUntilFearUpdate > 0.f)
		{
			// Still update CosmeticFear
			CosmeticFear = FMath::FInterpTo(CosmeticFear, FearAmount, RealDT, 1.f);
			return;
		}
		TimeUntilFearUpdate = UpdateFearDelay;
		DeltaTime = UpdateFearDelay;
	}

	const ASCCounselorCharacter* Counselor = GetCharacterOwner();
	ASCPlayerState* PlayerState = Counselor ? Counselor->GetPlayerState() : nullptr;
	if (!Counselor || !PlayerState)
		return;

	const float FearMod = FearModifier.Get(Counselor, true);

	for (TPair<TSubclassOf<USCFearEventData>, FFearEvent>& CurEvent : ActiveFearEvents)
	{
		if (!CurEvent.Key)
			continue;

		FFearEvent& Event = CurEvent.Value;
		const USCFearEventData* Data = Cast<USCFearEventData>(CurEvent.Key->GetDefaultObject());

		// Get max fear added by this event modified by perks
		const float EventMaxFearAdded = [&Data, &Counselor, &PlayerState]()
		{
			float MaxFearModifier = 0.0f;
			if (Data->GetClass() == Counselor->CorpseSpottedFearEvent || Data->GetClass() == Counselor->DarkFearEvent)
			{
				for (const USCPerkData* Perk : PlayerState->GetActivePerks())
				{
					if (Data->GetClass() == Counselor->CorpseSpottedFearEvent)
						MaxFearModifier += Perk->DeadBodyFearModifier;
					else if (Data->GetClass() == Counselor->DarkFearEvent)
						MaxFearModifier += Perk->DarknessFearModifier;
				}
			}

			float MaxFearCap = Data->MaxFearCap;
			MaxFearCap += MaxFearCap * MaxFearModifier;

			return MaxFearCap;
		}();

		if (CVarDebugFear.GetValueOnGameThread() > 0 && IsLocallyControlled())
		{
			FString Name = Data->GetName();
			Name.RemoveFromStart(TEXT("Default__"));
			Name.RemoveFromEnd(TEXT("_C"));
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, Event.Active ? FColor::Green : FColor::Red, *Name, false);
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, Event.Active ? FColor::Green : FColor::Red, FString::Printf(TEXT("        Delay: %.2f / %.2f"), Event.CurrentIncreaseDelay, Data->IncreaseDelay), false);
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, Event.Active ? FColor::Green : FColor::Red, FString::Printf(TEXT("        Added Fear: %.2f / %.2f"), Event.CurrentFearAdded, EventMaxFearAdded), false);
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, Event.Active ? FColor::Green : FColor::Red, FString::Printf(TEXT("        Delay: %.2f / %.2f"), Event.CurrentDecreaseDelay, Data->DecreaseDelay), false);
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, Event.Active ? FColor::Green : FColor::Red, FString::Printf(TEXT("        Removed Fear: %.2f / %.2f"), Event.CurrentFearRemoved, EventMaxFearAdded), false);
			if (Data->AutoDeactivateTimer > 0.0f)
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, Event.Active ? FColor::Green : FColor::Red, FString::Printf(TEXT("        DeactivateTimer: %.2f / %.2f"), Event.CurrentDeactivateTimer, Data->AutoDeactivateTimer), false);
		}

		float RateMultiplier = 1.0f;
		if (Data->FearDistanceCurve)
			RateMultiplier = Data->FearDistanceCurve->GetFloatValue(Event.CurveData);

		const auto ApplyPerkModifiers = [&Data, &Counselor, &PlayerState](const float CurrentFearMod, const bool bIncreaseFear)
		{
			float NewFearModifier = CurrentFearMod;
			float PerkFearModifier = 0.f;
			for (const USCPerkData* Perk : PlayerState->GetActivePerks())
			{
				check(Perk);

				if (!Perk)
					continue;

				if (bIncreaseFear)
					PerkFearModifier += Perk->FearModifier;

				if (Counselor->GetCurrentHidingSpot())
					PerkFearModifier += Perk->HidespotFearModifier;

				if (Perk->GetClass() == Counselor->TeamworkPerk)
					PerkFearModifier += Perk->CounselorsAroundFearModifier * Counselor->GetNumCounselorsAroundMe();
				else if (Perk->GetClass() == Counselor->LoneWolfPerk && Counselor->GetNumCounselorsAroundMe() == 0)
					PerkFearModifier += Perk->CounselorsAroundFearModifier;
			}

			if (CVarDebugFear.GetValueOnGameThread() > 0 && Counselor->IsLocallyControlled())
			{
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Cyan, FString::Printf(TEXT("Fear Modifier (No Perks): %.2f"), NewFearModifier), false);
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Cyan, FString::Printf(TEXT("Perk Fear Modifier: %.2f"), PerkFearModifier), false);
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Cyan, FString::Printf(TEXT("Fear Modifer (With Perks): %.2f"), NewFearModifier + PerkFearModifier), false);
			}

			NewFearModifier += PerkFearModifier;

			return NewFearModifier;
		};

		bool MaxReached = false;
		if (Event.Active != Data->bDecreaseWhenActive)
		{
			if (FearAmount >= 100.0f)
				continue;

			Event.CurrentIncreaseDelay += DeltaTime;
			if (Event.CurrentIncreaseDelay >= Data->IncreaseDelay)
			{
				Event.CurrentIncreaseDelay = Data->IncreaseDelay;

				if (Event.CurrentFearAdded < EventMaxFearAdded)
				{
					const float NewFear = Data->IncreaseFearValue * DeltaTime * ApplyPerkModifiers(FearMod, Event.Active) * RateMultiplier;
					Event.CurrentFearAdded += NewFear;
					Event.CurrentFearRemoved = FMath::Max(0.0f, Event.CurrentFearRemoved - NewFear);
					FearAmount += NewFear;
					continue;
				}

				MaxReached = true;
			}
		}
		else
		{
			if (FearAmount <= 0.0f)
				continue;

			Event.CurrentDecreaseDelay += DeltaTime;
			if (Event.CurrentDecreaseDelay >= Data->DecreaseDelay)
			{
				Event.CurrentDecreaseDelay = Data->DecreaseDelay;

				if (Event.CurrentFearRemoved < EventMaxFearAdded && Event.CurrentFearAdded > 0.0f)
				{
					const float NewFear = Data->DecreaseFearValue * DeltaTime * ApplyPerkModifiers(FearMod, Event.Active);
					Event.CurrentFearRemoved += NewFear;
					Event.CurrentFearAdded = FMath::Max(0.0f, Event.CurrentFearAdded - NewFear);
					FearAmount -= NewFear;
					continue;
				}

				MaxReached = true;
			}
		}

		if (Event.Active && Data->AutoDeactivateTimer > 0.0f && MaxReached)
		{
			Event.CurrentDeactivateTimer += DeltaTime;
			Event.Active = Event.CurrentDeactivateTimer < Data->AutoDeactivateTimer;
		}
	}

	if (HasAuthority()) 
	{
		FearAmount = FMath::Clamp((float)FearAmount, 0.0f, 100.0f);

		// Mark this player as scared
		if (FearAmount >= 100.0f)
		{
			PlayerState->GotScared();
		}
	}

	if (CVarDebugFear.GetValueOnGameThread() > 0 && IsLocallyControlled())
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Purple, FString::Printf(TEXT("Total Fear: %.2f"), (float)FearAmount), true, FVector2D(1.2f,1.2f));

	CosmeticFear = FMath::FInterpTo(CosmeticFear, FearAmount, RealDT, 1.0f);
}

void USCFearComponent::PushFearEvent(TSubclassOf<USCFearEventData> FearEvent, float CurveData /*=0.0f*/)
{
	ensureMsgf(FearEvent != nullptr, TEXT("NULL FEAR EVENT!"));

	if (!ActiveFearEvents.Contains(FearEvent))
	{
		FFearEvent Event;
		Event.Active = true;
		Event.CurveData = CurveData;
		ActiveFearEvents.Add(FearEvent, Event);
	}
	else if (ActiveFearEvents[FearEvent].Active == false)
	{
		ActiveFearEvents[FearEvent].Active = true;
		ActiveFearEvents[FearEvent].CurrentIncreaseDelay = 0.0f;
		ActiveFearEvents[FearEvent].CurrentDeactivateTimer = 0.0f;
	}

	ActiveFearEvents[FearEvent].CurveData = CurveData;
}


void USCFearComponent::PopFearEvent(TSubclassOf<USCFearEventData> FearEvent)
{
	ensureMsgf(FearEvent != nullptr, TEXT("NULL FEAR EVENT!"));

	if (!ActiveFearEvents.Contains(FearEvent))
	{
		FFearEvent Event;
		Event.Active = false;
		ActiveFearEvents.Add(FearEvent, Event);
	}
	else if (ActiveFearEvents[FearEvent].Active == true)
	{
		ActiveFearEvents[FearEvent].Active = false;
		ActiveFearEvents[FearEvent].CurrentDecreaseDelay = 0.0f;
	}
}

ASCCounselorCharacter* USCFearComponent::GetCharacterOwner() const
{
	if (ASCCounselorCharacter* CharacterOwner = Cast<ASCCounselorCharacter>(GetOwner()))
	{
		return CharacterOwner;
	}

	return nullptr;
}

bool USCFearComponent::IsLocallyControlled() const
{
	return (GetCharacterOwner() ? GetCharacterOwner()->IsLocallyControlled() : false);
}

void USCFearComponent::LockFearCMD::LockFearToCMD(const TArray<FString>& Args)
{
	float InFearLevel = 0;
	bool Lock = true;
	if (Args.Num() > 0)
		InFearLevel = FCString::Atof(*Args[0]);

	if (Args.Num() > 1)
	{
		FString lockstring = Args[1];

		Lock = (lockstring.Contains(TEXT("true")) || lockstring.Contains(TEXT("1")));
	}

	Owner->LockFearTo(InFearLevel, Lock);
}

bool USCFearComponent::SERVER_LockFearTo_Validate(float NewFear, bool Lock)
{
	return true;
}

void USCFearComponent::SERVER_LockFearTo_Implementation(float NewFear, bool Lock)
{
	LockFearTo(NewFear, Lock);
}

void USCFearComponent::LockFearTo(float InFearLevel, bool Lock /* = true */)
{
	if (GetOwnerRole() < ROLE_Authority)
	{
		SERVER_LockFearTo(InFearLevel, Lock);
	}
	else
	{
		FearAmount = FMath::Clamp(InFearLevel, 0.f, 100.f);
		CosmeticFear = FearAmount;
		bLockFearLevel = Lock;
	}
}

void USCFearComponent::RegisterLockFearCMD()
{
	if (IsLocallyControlled() && !LockFearCommand.bRegistered)
	{
		LockFearCommand.Register();
	}
}
