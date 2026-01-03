// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"

#include "Animation/AnimMontage.h"

#include "ILLInventoryComponent.h"
#include "ILLInventoryItem.h"
#include "ILLCharacter.h"
#include "ILLInventoryComponent.h"

FName AILLInventoryItem::ItemMesh3PComponentName = TEXT("ItemMesh3P");
FName AILLInventoryItem::ItemMesh1PComponentName = TEXT("ItemMesh1P");

AILLInventoryItem::AILLInventoryItem(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;

	bNetUseOwnerRelevancy = true;
	NetPriority = 2.f;

	// Equipping defaults
	EquipDurationFallback = 0.25f;
	UnequipDurationFallback = 0.25f;

	// Root Component
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

void AILLInventoryItem::Destroyed()
{
	Super::Destroyed();

	// Stop all previously started sounds immediately
	for (auto AudioComponent : ItemAudioComponents)
	{
		if (AudioComponent && !AudioComponent->IsPendingKill())
		{
			AudioComponent->Stop();
		}
	}
}

void AILLInventoryItem::OnRep_Owner()
{
	Super::OnRep_Owner();

	if (GetOwner() == nullptr)
	{
		RemovedFromInventory(OuterInventory);
	}
}

void AILLInventoryItem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_EquipUnequip);
	}

	Super::EndPlay(EndPlayReason);
}

void AILLInventoryItem::AddedToInventory(UILLInventoryComponent* Inventory)
{
	if (OuterInventory != Inventory)
	{
		OuterInventory = Inventory;
		SetOwner(Inventory->GetOwner());
	}

	// Start attached to the body
	AttachToCharacter(true);
}

void AILLInventoryItem::RemovedFromInventory(UILLInventoryComponent* Inventory)
{
	DetachFromCharacter();

	if (OuterInventory == Inventory)
	{
		OuterInventory = nullptr;
		SetOwner(nullptr);
	}
}

AILLCharacter* AILLInventoryItem::GetCharacterOwner() const
{
	if (AILLCharacter* CharacterOwner = Cast<AILLCharacter>(GetOwner()))
	{
		return CharacterOwner;
	}

	return OuterInventory ? OuterInventory->GetCharacterOwner() : nullptr;
}

bool AILLInventoryItem::IsLocallyControlled() const
{
	return (GetCharacterOwner() ? GetCharacterOwner()->IsLocallyControlled() : false);
}

bool AILLInventoryItem::CanUnequipNow() const
{
	return IsEquipped();
}

bool AILLInventoryItem::CheckScheduledUnequip()
{
	if (bScheduledUnequip && CanUnequipNow())
	{
		// We have a scheduled unequip AND can do it! Go go...
		StartUnequipping(ScheduledDelegate, ScheduledItem);
		ScheduledItem = nullptr;
		bScheduledUnequip = false;
		return true;
	}

	return false;
}

void AILLInventoryItem::StartEquipping(FILLInventoryItemEquipDelegate CompletionCallback, const bool bFromReplication)
{
	// HACK BEGIN - nfaletra - Need to figure out why we're getting in here with an item that's already equipped
	if (IsEquipped())
		return;
	// HACK END

	check(!IsEquipped());
	EquipState = EILLInventoryEquipState::Equipping;

	// Simulation hooks for sub-classes
	SimulatedStartEquipping();

	// Set a timer to complete equipping
	if (EquipDuration <= 0.f)
	{
		FinishEquipping(CompletionCallback, bFromReplication);
	}
	else
	{
		auto FinishDelegate = FTimerDelegate::CreateUObject(this, &AILLInventoryItem::FinishEquipping, CompletionCallback, bFromReplication);
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_EquipUnequip, FinishDelegate, EquipDuration, false);
	}
}

bool AILLInventoryItem::StartUnequipping(FILLInventoryItemUnequipDelegate CompletionCallback, AILLInventoryItem* NextItem)
{
	// If we can't unequip now, then schedule it!
	if (!CanUnequipNow())
	{
		ScheduledDelegate = CompletionCallback;
		ScheduledItem = NextItem;
		bScheduledUnequip = true;
		return false;
	}

	check(!IsUnequipped());
	EquipState = EILLInventoryEquipState::Unequipping;

	// Simulation hooks for sub-classes
	SimulatedStartUnequipping();

	// Set a timer to complete unequipping
	if (UnequipDuration <= 0.f)
	{
		FinishUnequipping(CompletionCallback, NextItem);
	}
	else
	{
		auto FinishDelegate = FTimerDelegate::CreateUObject(this, &AILLInventoryItem::FinishUnequipping, CompletionCallback, NextItem);
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_EquipUnequip, FinishDelegate, UnequipDuration, false);
	}

	return true;
}

float AILLInventoryItem::GetEquipDuration(float PlaybackRate/* = 1.f*/) const
{
	float Length = 0.f;
	if (EquipMontage.FirstPerson)
		Length = EquipMontage.FirstPerson->GetPlayLength();

	if (EquipMontage.ThirdPerson)
		Length = FMath::Max(Length, EquipMontage.ThirdPerson->GetPlayLength());

	if (Length == 0.f)
		return EquipDurationFallback;

	return Length / FMath::Max(PlaybackRate, KINDA_SMALL_NUMBER);
}

float AILLInventoryItem::GetUnequipDuration(float PlaybackRate/* = 1.f*/) const
{
	float Length = 0.f;
	if (UnequipMontage.FirstPerson)
		Length = UnequipMontage.FirstPerson->GetPlayLength();

	if (UnequipMontage.ThirdPerson)
		Length = FMath::Max(Length, UnequipMontage.ThirdPerson->GetPlayLength());

	if (Length == 0.f)
		return UnequipDurationFallback;

	return Length / FMath::Max(PlaybackRate, KINDA_SMALL_NUMBER);
}

void AILLInventoryItem::SimulatedStartEquipping()
{
	// Play Montage
	if (EquipMontage.FirstPerson != nullptr || EquipMontage.ThirdPerson != nullptr)
	{
		EquipDuration = GetCharacterOwner()->PlayAnimMontage(&EquipMontage);
	}
	else
	{
		EquipDuration = EquipDurationFallback;
	}

	// Attach to the hand
	if (bAutoAttach)
	{
		AttachToCharacter(false);
	}

	// Play a sound
	PlayItemSound(EquipStartSound);
}

void AILLInventoryItem::SimulatedFinishEquipping()
{
	// Play a sound
	PlayItemSound(EquipFinishedSound);
}

void AILLInventoryItem::SimulatedStartUnequipping()
{
	// Play Montage
	if (UnequipMontage.FirstPerson != nullptr || UnequipMontage.ThirdPerson != nullptr)
	{
		UnequipDuration = GetCharacterOwner()->PlayAnimMontage(&UnequipMontage);
	}
	else
	{
		UnequipDuration = UnequipDurationFallback;
	}

	// Attach to the body
	if (bAutoAttach)
	{
		AttachToCharacter(true);
	}

	// Play a sound
	PlayItemSound(UnequipStartSound);
}

void AILLInventoryItem::SimulatedFinishUnequipping()
{
	// Play a sound
	PlayItemSound(UnequipFinishedSound);
}

void AILLInventoryItem::FinishEquipping(FILLInventoryItemEquipDelegate CompletionCallback, const bool bFromReplication)
{
	check(IsEquipping());
	EquipState = EILLInventoryEquipState::Equipped;

	// Simulation hooks for sub-classes
	SimulatedFinishEquipping();

	// Notify listeners
	CompletionCallback.Execute(this);

	// Check if we already need to switch to something else
	CheckScheduledUnequip();
}

void AILLInventoryItem::FinishUnequipping(FILLInventoryItemUnequipDelegate CompletionCallback, AILLInventoryItem* NextItem)
{
	check(IsUnequipping());
	EquipState = EILLInventoryEquipState::Unequipped;

	// Simulation hooks for sub-classes
	SimulatedFinishUnequipping();

	// Notify listeners
	CompletionCallback.Execute(this, NextItem);
}

void AILLInventoryItem::AttachToCharacter(bool AttachToBody, FName OverrideSocket/* = NAME_None*/)
{
	const FName Socket = OverrideSocket.IsNone() ? (AttachToBody ? BodyAttachSocket : HandAttachSocket) : OverrideSocket;

	// No valid attach point, hide the item
	if (Socket.IsNone())
	{
		if (ItemMesh3P)
			ItemMesh3P->SetHiddenInGame(true);

		if (ItemMesh1P)
			ItemMesh1P->SetHiddenInGame(true);

		return;
	}

	// Attach each item mesh to the appropriate character mesh
	if (ItemMesh3P)
	{
		ItemMesh3P->AttachToComponent(GetCharacterOwner()->GetThirdPersonMesh(), FAttachmentTransformRules(EAttachmentRule::SnapToTarget, false), Socket);
		ItemMesh3P->SetHiddenInGame(false);
	}

	if (ItemMesh1P)
	{
		ItemMesh1P->AttachToComponent(GetCharacterOwner()->GetFirstPersonMesh(), FAttachmentTransformRules(EAttachmentRule::SnapToTarget, false), Socket);
		ItemMesh1P->SetHiddenInGame(false);
	}
}

void AILLInventoryItem::DetachFromCharacter()
{
	// Re-attach mesh components to the actor's root component
	if (ItemMesh3P)
	{
		ItemMesh3P->AttachToComponent(Root, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, false));
		ItemMesh3P->SetHiddenInGame(false);
	}

	if (ItemMesh1P)
	{
		ItemMesh1P->AttachToComponent(Root, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, false));
		ItemMesh1P->SetHiddenInGame(true);
	}
}

void AILLInventoryItem::UpdateMeshVisibility(const bool bFirstPerson)
{
	const bool bShouldBeVisible = IsEquipped();
	if (ItemMesh1P)
	{
		ItemMesh1P->SetHiddenInGame(bShouldBeVisible && bFirstPerson);
	}
	if (ItemMesh3P)
	{
		ItemMesh3P->SetHiddenInGame(bShouldBeVisible && !bFirstPerson);
	}
}

void AILLInventoryItem::PlayItemSound(USoundCue* Sound, const bool bAutoStopOnDestroy/* = true*/)
{
	if (GetNetMode() != NM_DedicatedServer && Sound)
	{
		AILLCharacter* CharacterOwner = GetCharacterOwner();
		USceneComponent* AttachToComponent = CharacterOwner ? CharacterOwner->GetRootComponent() : GetRootComponent();
		if (UAudioComponent* NewComponent = UGameplayStatics::SpawnSoundAttached(Sound, AttachToComponent))
		{
			if (bAutoStopOnDestroy)
			{
				ItemAudioComponents.Add(NewComponent);
			}
		}
	}
}
const TArray<FILLCharacterMontage>* AILLInventoryItem::GetFidgets(const AILLCharacter* Character, const bool bExactMatchOnly /*= false*/) const
{
	for (int32 iFidget = 0; iFidget < Fidgets.Num(); ++iFidget)
	{
		if (Fidgets[iFidget].CanPlayOnCharacter(Character, bExactMatchOnly))
		{
			return &Fidgets[iFidget].Montages;
		}
	}

	return nullptr;
}
