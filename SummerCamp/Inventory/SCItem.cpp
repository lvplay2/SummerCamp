// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCItem.h"

#include "Animation/AnimMontage.h"
#include "Landscape.h"
#include "VisualLogger.h"

#include "ILLInventoryComponent.h"

#include "SCCounselorAIController.h"
#include "SCCounselorCharacter.h"
#include "SCCabinet.h"
#include "SCDoubleCabinet.h"
#include "SCGameInstance.h"
#include "SCGameMode.h"
#include "SCGameState.h"
#include "SCHidingSpot.h"
#include "SCIndoorComponent.h"
#include "SCInteractableManagerComponent.h"
#include "SCInteractComponent.h"
#include "SCMinimapIconComponent.h"
#include "SCPlayerState.h"
#include "SCShoreItemSpawner.h"
#include "SCWeapon.h"

/*
				WARNING: THIS CLASS USES NET DORMANCY
	This means that if you need to update any replicated variables,
	logic, RPCs or other networking whatnot, you NEED to update the
	logic handling for dormancy.
*/

namespace SCItemNames
{
	static const FName NAME_ShimmerParam(TEXT("Shimmer"));
}

uint8 ASCItem::RotationIndex = 0;

#define ITEM_SHIMMER_ON 1.f
#define ITEM_SHIMMER_OFF 0.f

#define ITEM_DROP_DISTANCE 40.f // how far away from the character we should attempt to drop the time

#define ITEM_DROP_ITERATION 10 // the number of times to attempt a sweep to find a good location to drop
#define ITEM_DROP_ANGLE_DIVISIONS 10

#define LOCTEXT_NAMESPACE "ILLFonic.F13.SCItem"

DEFINE_LOG_CATEGORY_STATIC(LogSCItem, Warning, All);

TAutoConsoleVariable<int32> CVarDebugItemDrop(TEXT("sc.DebugItemDrop"), 0,
										 TEXT("Displays debug information for item dropping placement.\n")
										 TEXT(" 0: Off\n")
										 TEXT(" 1: On"));

ASCItem::ASCItem(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, FriendlyName(LOCTEXT("name", "Base Item"))
, NumUses(1)
, bCanShimmer(true)
, bStartsShimmering(true)
, HightlightScaleOnDropped(2.f)
{
	bReplicates = true;
	bReplicateMovement = true;
	bAlwaysRelevant = false;

	NetUpdateFrequency = 20.f;

	ItemMesh3P = CreateOptionalDefaultSubobject<UStaticMeshComponent>(ItemMesh3PComponentName);
	if (ItemMesh3P)
	{
		ItemMesh3P->bOnlyOwnerSee = false;
		ItemMesh3P->bOwnerNoSee = true;
		ItemMesh3P->bReceivesDecals = false;
		ItemMesh3P->CastShadow = true;
		ItemMesh3P->SetupAttachment(RootComponent);

		ItemMesh3P->SetCollisionProfileName(TEXT("NoCollision"));
		ItemMesh3P->SetCollisionObjectType(ECC_WorldDynamic);

		ItemMesh3P->bOwnerNoSee = false;
		ItemMesh3P->bCastInsetShadow = true;

		ItemMesh3P->SetTickGroup(TG_PostPhysics);
	}

	InteractComponent = CreateDefaultSubobject<USCInteractComponent>(TEXT("Interactable"));

	MinimapIconComponent = CreateDefaultSubobject<USCMinimapIconComponent>(TEXT("MinimapIcon"));
	MinimapIconComponent->SetVisibility(false);
	MinimapIconComponent->SetupAttachment(RootComponent);

	if (GetMesh())
		InteractComponent->SetupAttachment(GetMesh());

	InteractComponent->IgnoreClasses.Add(ThisClass::StaticClass());

	HandAttachSocket = TEXT("PropSocket");

	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bShouldCastShadows = true;
	bShouldOffsetPivotOnDrop = false;
	bIsInUse = false;
}

void ASCItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCItem, CurrentCabinet);
	DOREPLIFETIME(ASCItem, bIsPicking);
	DOREPLIFETIME(ASCItem, NumUses);
	DOREPLIFETIME(ASCItem, TimesUsed);
	DOREPLIFETIME(ASCItem, bShouldCastShadows);
	DOREPLIFETIME(ASCItem, bIsInUse);
}

void ASCItem::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (UWorld* World = GetWorld())
	{
		InteractComponent->OnInteract.AddDynamic(this, &ASCItem::OnInteract);
		InteractComponent->OnCanInteractWith.BindDynamic(this, &ASCItem::CanInteractWith);

		if (ASCGameState* GS = Cast<ASCGameState>(World->GetGameState()))
		{
			GS->ItemList.AddUnique(this);
		}
	}
}

void ASCItem::Destroyed()
{
	if (UWorld* World = GetWorld())
	{
		if (ASCGameState* GS = Cast<ASCGameState>(World->GetGameState()))
		{
			GS->ItemList.Remove(this);
		}
	}

	Super::Destroyed();
}

void ASCItem::BeginPlay()
{
	Super::BeginPlay();

	HighlightSizeOnDropped = InteractComponent->HighlightDistance * HightlightScaleOnDropped;

	if (bCanShimmer)
	{
		if (!IsRunningDedicatedServer())
		{
			TArray<UMaterialInterface*> Materials = GetMesh()->GetMaterials();
			for (int32 i(0); i < Materials.Num(); ++i)
			{
				GetMesh()->CreateDynamicMaterialInstance(i, Materials[i]);
			}
		}

		SetShimmerEnabled(bStartsShimmering);
	}

	MinimapIconComponent->SetVisibility(false);

	if (ItemMesh3P)
		bWasCastingShadows = ItemMesh3P->CastShadow;
}

void ASCItem::SetOwner(AActor* NewOwner)
{
	// Stay awake while we have an owner
	if (NewOwner)
		SetNetDormancy(DORM_Awake);

	Super::SetOwner(NewOwner);

	ASCCharacter* OldCharacter = OwningCharacter;
	OwningCharacter = Cast<ASCCharacter>(NewOwner);

	// If the character has changed, do things!
	if (OldCharacter != OwningCharacter)
		OwningCharacterChanged(OldCharacter);
}

void ASCItem::OnRep_Owner()
{
	// Keep this here for net relevancy changes
	Super::OnRep_Owner();

	// Stay awake while we have an owner
	if (GetOwner())
		SetNetDormancy(DORM_Awake);
	else
		SetNetDormancy(DORM_DormantAll);

	ASCCharacter* OldCharacter = OwningCharacter;
	OwningCharacter = Cast<ASCCharacter>(GetOwner());

	// If the character has changed, do things!
	if (OldCharacter != OwningCharacter)
		OwningCharacterChanged(OldCharacter);
}

void ASCItem::AttachToCharacter(bool bAttachToBody, FName OverrideSocket/* = NAME_None*/)
{
	const FName Socket = OverrideSocket.IsNone() ? (bAttachToBody ? BodyAttachSocket : HandAttachSocket) : OverrideSocket;

	if (GetParentActor())
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	if (AILLCharacter* CharacterOwner = GetCharacterOwner())
	{
		AttachToComponent(CharacterOwner->GetThirdPersonMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);
	}

	SetActorHiddenInGame(bAttachToBody);

	if (Role == ROLE_Authority)
	{
		ForceNetUpdate();
		bShouldCastShadows = false;
		OnRep_Shadows();
	}
}

void ASCItem::DetachFromCharacter()
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	SetShimmerEnabled(true);

	if (Role == ROLE_Authority)
	{
		ForceNetUpdate();
		SetActorHiddenInGame(false);
		bShouldCastShadows = bWasCastingShadows;
		OnRep_Shadows();
		bIsInUse = false;
	}
}

void ASCItem::OwningCharacterChanged(ASCCharacter* OldOwner)
{
	// No owner, we've been dropped! (Clients)
	if (OwningCharacter == nullptr)
	{
		if (HighlightSizeOnDropped > 0.f)
			InteractComponent->HighlightDistance = HighlightSizeOnDropped;

		InteractComponent->bIsEnabled = true;
		SetShimmerEnabled(InteractComponent->bCanHighlight);

		SetNetDormancy(DORM_DormantAll);
		MinimapIconComponent->SetVisibility(OldOwner != nullptr);
	}
	else
	{
		if (OwningCharacter->Role == ROLE_Authority)
		{
			SetNetDormancy(DORM_Awake);

			EnableInteraction(false);

			// FIXME: Doesn't handle swapping
			if (CurrentShoreSpawner)
			{
				CurrentShoreSpawner->bIsInUse = false;
				CurrentShoreSpawner = nullptr;
			}
		}

		if (CurrentCabinet)
		{
			ForceNetUpdate();
			CurrentCabinet->RemoveItem(this);
			CurrentCabinet = nullptr;
		}

		SetShimmerEnabled(false);
		OwningCharacter->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
		MinimapIconComponent->SetVisibility(false);
	}
}

int32 ASCItem::CanInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	if (GetOuterInventory())
		return 0;

	if (IsInCabinet())
		return 0;

	if (bIsPicking)
		return 0;

	ASCCharacter* SCCharacter = Cast<ASCCharacter>(Character);
	if (InventoryMax > 0 && InventoryMax <= SCCharacter->NumberOfItemTypeInInventory(GetClass()))
		return 0;

	if (SCCharacter->IsInSpecialMove())
		return 0;

	if (SCCharacter->CurrentStance == ESCCharacterStance::Swimming)
		return 0;

	if (SCCharacter->IsInWheelchair())
		return 0;

	if (bIsInUse)
		return 0;

	return (int32)EILLInteractMethod::Press;
}

void ASCItem::OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	ASCCharacter* Character = Cast<ASCCharacter>(Interactor);
	check(Character);

	switch (InteractMethod)
	{
	case EILLInteractMethod::Press:
		// We're already picking this item up
		if (bIsPicking)
			return;

		bIsPicking = true;
		Character->SimulatePickupItem(this);

		break;
	}
}

void ASCItem::OnDropped(ASCCharacter* DroppingCharacter, FVector AtLocation /*= FVector::ZeroVector*/)
{
	UE_VLOG(this, LogSCItem, Verbose, TEXT("Dropping %s @ %s"), *GetName(), AtLocation == FVector::ZeroVector ? TEXT("Searching...") : *AtLocation.ToString());
	DetachFromCharacter();
	bIsPicking = false;
	EnableInteraction(true);

	SetActorRotation(DropRotation);

	FVector StartLocation = AtLocation;
	FVector EndLocation = AtLocation;

	const bool bSpecifiedLocation = !AtLocation.IsNearlyZero();

	if (!bSpecifiedLocation)
		StartLocation = EndLocation = DroppingCharacter ? DroppingCharacter->GetActorLocation() : GetActorLocation();

	// we are going to randomly rotate this in 360 degrees so the world forward is fine
	const FVector DropForward = FVector(1.f, 0.f, 0.f);

	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	if (DroppingCharacter)
	{
		// ignore the dropping character and their capsule
		QueryParams.AddIgnoredActor(DroppingCharacter);
		QueryParams.AddIgnoredActor(DroppingCharacter->PickingItem);
		QueryParams.AddIgnoredComponent(DroppingCharacter->GetCapsuleComponent());
		QueryParams.AddIgnoredComponent(DroppingCharacter->GetMesh());
		QueryParams.bReturnPhysicalMaterial = true;

		// specifically to ignore the killers weapon when he drops his mask
		if (DroppingCharacter->GetCurrentWeapon())
		{
			const ASCWeapon* Weapon = reinterpret_cast<const ASCWeapon*>(DroppingCharacter->GetCurrentWeapon());
			QueryParams.AddIgnoredActor(Weapon);
		}
	}


	FVector Origin, Extent;
	GetActorBounds(false, Origin, Extent);
	const float HalfHeight = Extent.Z * 0.5f;
	const FCollisionShape CollisionBox = FCollisionShape::MakeBox(Extent);

	bool bValidLocation = false;
	// grab a random location and attempt to find a valid ground location
	for (uint8 i(0); i < ITEM_DROP_ITERATION && !bValidLocation; ++i)
	{
		// do a sweep down to find a good dropping locatoin

		const float DropAngle = ((float)RotationIndex++ / ITEM_DROP_ANGLE_DIVISIONS) * 360.f;
		if (RotationIndex >= ITEM_DROP_ANGLE_DIVISIONS)
			RotationIndex = 0;

		const FVector DropLocation = StartLocation + (!bSpecifiedLocation ? (DropForward * ITEM_DROP_DISTANCE).RotateAngleAxis(DropAngle, FVector::UpVector) : FVector::ZeroVector);

		if (CVarDebugItemDrop.GetValueOnGameThread() > 0)
		{
			DrawDebugBox(GetWorld(), DropLocation + (!bSpecifiedLocation ? FVector::UpVector * HalfHeight : FVector::ZeroVector), CollisionBox.GetExtent(), FQuat::Identity, FColor::Green, false, 60.0f);
			DrawDebugBox(GetWorld(), DropLocation - (FVector::UpVector * 200.f), CollisionBox.GetExtent(), FQuat::Identity, FColor::Red, false, 60.0f);
		}

		if (GetWorld()->SweepSingleByChannel(Hit,
											 DropLocation + (!bSpecifiedLocation ? FVector::UpVector * HalfHeight : FVector::ZeroVector), // start location of sweep
											 DropLocation - (FVector::UpVector * 2000.f), // end location of sweep
											 FQuat::Identity,// GetActorBounds returns the bounds of the actor as it currently is so no need to rotate
											 ECC_Visibility, // channel to sweep against
											 CollisionBox, // collision shape
											 QueryParams) // additional query params
			)
		{

			if (CVarDebugItemDrop.GetValueOnGameThread() > 0)
			{
				DrawDebugBox(GetWorld(), Hit.ImpactPoint, CollisionBox.GetExtent(), FQuat::Identity, FColor::Yellow, false, 60.0f);
			}

			if (Hit.Time <= KINDA_SMALL_NUMBER) // the sweep didnt move, means there is a good chance its colliding with something, lets try a different location
			{
				if (bSpecifiedLocation)
				{
					StartLocation = EndLocation = StartLocation + FVector::UpVector * HalfHeight;
				}
				else
				{
					StartLocation = EndLocation = [&]() -> FVector
					{
						if (DroppingCharacter)
						{
							// Use hide spot's item drop location
							ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(DroppingCharacter);
							if (ASCHidingSpot* HidingSpot = Counselor ? Counselor->GetCurrentHidingSpot() : nullptr)
							{
								return HidingSpot->GetItemDropLocation();
							}
							
							// Use the dropping character's location
							return DroppingCharacter->GetActorLocation();
						}
						
						// Default to using the item's location
						return GetActorLocation();
					}();
				}

				continue;
			}

			if (!CurrentShoreSpawner)
			{
				TArray<FOverlapResult> Overlaps;
				// lets do a sweep test to attempt to grab a water volume
				if (GetWorld()->OverlapMultiByChannel(Overlaps, Hit.Location, FQuat::Identity, ECC_WorldDynamic, CollisionBox, QueryParams) || Overlaps.Num() > 0)
				{
					for (const FOverlapResult& Overlap : Overlaps) // loop through potential overlaps
					{
						if (Overlap.GetActor())
						{
							if (APhysicsVolume* Volume = Cast<APhysicsVolume>(Overlap.GetActor()))
							{
								if (Volume->bWaterVolume)
								{
									// Totally water, find a shore spawner
									ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>();
									FTransform ItemTransform(FTransform::Identity);
									if (GameMode->GetShoreLocation(GetActorLocation(), this, ItemTransform)) // GetShorLocation sets CurrentShoreSpawner
									{
										// Recurse back into this function, but we'll have a shorespawner set so we shouldn't stack overflow
										OnDropped(nullptr, ItemTransform.GetLocation());
										return;
									}
								}
							}
						}
					}
				}
			}

			EndLocation = Hit.Location + OriginOffset;
			bValidLocation = true;
			break;
		}
	}

	if (bValidLocation)
	{
		// set actor location but let it attempt to sweep just to be safe
		SetActorLocation(EndLocation, true, nullptr, ETeleportType::TeleportPhysics);
	}
	else
	{
		FVector DropLoc = DroppingCharacter ? DroppingCharacter->GetActorLocation() : GetActorLocation();
		// For the case of the an item leaning up against a wall (its Location is above the ground). This is to avoid floating items. 
		// Using a sweep instead of a line-trace because items leaning into the wall may swap incorrectly (i.e. INTO the wall) with the player's equipped item, ultimately 
		// disabling the iteraction pip. 
		GetWorld()->SweepSingleByChannel(Hit, DropLoc, DropLoc + (FVector::UpVector * -2000.f), FQuat::Identity,  ECC_Visibility, CollisionBox, QueryParams);

		// Drop the item to the ground, or whatever surface it was laying on with a small bump from our extents so that it doesn't end up in the ground.
		DropLoc.Z = Hit.ImpactPoint.Z + Extent.GetMin();

		SetActorLocation(DropLoc, false, nullptr, ETeleportType::TeleportPhysics);
	}

	GetActorBounds(false, Origin, Extent);
	
	if (CVarDebugItemDrop.GetValueOnGameThread() > 0)
	{
		DrawDebugBox(GetWorld(), EndLocation, CollisionBox.GetExtent(), FQuat::Identity, FColor::Blue, false, 5.0f);
	}

	if (!DroppedSoundCue.IsNull())
	{
		MULTI_DeferredPlayDroppedSound();
	}

	SetOwner(nullptr);
}

void ASCItem::MULTI_DeferredPlayDroppedSound_Implementation()
{
	if (!DroppedSoundCue.Get() && !DroppedSoundCue.IsNull())
	{
		USCGameInstance* GameInstance = Cast<USCGameInstance>(GetWorld()->GetGameInstance());
		GameInstance->StreamableManager.RequestAsyncLoad(DroppedSoundCue.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &ThisClass::PlayDroppedSound));
		return;
	}

	PlayDroppedSound();
}

void ASCItem::PlayDroppedSound()
{
	if (!DroppedSoundCue.Get())
	{
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(GetWorld(), DroppedSoundCue.Get(), GetActorLocation());
}

void ASCItem::EnableInteraction(bool bInEnable)
{
	ForceNetUpdate();
	InteractComponent->bIsEnabled = bInEnable;
}

bool ASCItem::ShouldAttach(bool bFromInput)
{
	return true;
}

bool ASCItem::CanUse(bool bFromInput)
{
	return true;
}

bool ASCItem::Use(bool bFromInput)
{
	UE_LOG(LogSCItem, Warning, TEXT("Use() not implemented in child class!"));
	return false;
}

void ASCItem::OnRep_CurrentCabinet()
{
	if (!CurrentCabinet)
		return;

	// Sometimes cabinets just don't know about us. So let's make sure they do.
	if (ASCDoubleCabinet* DoubleCabinet = Cast<ASCDoubleCabinet>(CurrentCabinet))
	{
		// Dear double cabinets, I hate you. Love, IllFonic
		const FVector Location = GetActorLocation();
		const float DistSq = (Location - DoubleCabinet->ItemAttachLocation->GetComponentLocation()).SizeSquared();
		const float DistSq2 = (Location - DoubleCabinet->ItemAttachLocation2->GetComponentLocation()).SizeSquared();
		if (DistSq < DistSq2)
			DoubleCabinet->ContainedItem = this;
		else
			DoubleCabinet->ContainedItem2 = this;
	}
	else
	{
		CurrentCabinet->ContainedItem = this;
	}
}

UAnimSequenceBase* ASCItem::GetBestAsset(const USkeleton* Skeleton, const TArray<UAnimSequenceBase*>& AssetArray) const
{
	for (UAnimSequenceBase* Asset : AssetArray)
	{
		if (Asset && Asset->GetSkeleton() == Skeleton)
			return Asset;
	}

	return nullptr;
}

UAnimMontage* ASCItem::GetBestAsset(const USkeleton* Skeleton, const TArray<UAnimMontage*>& AssetArray) const
{
	for (UAnimMontage* Asset : AssetArray)
	{
		if (Asset && Asset->GetSkeleton() == Skeleton)
			return Asset;
	}

	return nullptr;
}

void ASCItem::SetShimmerEnabled(const bool bEnabled)
{
	if (bEnabled)
	{
		// Can't ever shimmer
		if (!bCanShimmer)
			return;

		// Don't turn on shimmer for items we're holding
		if (OwningCharacter)
			return;

		if (UWorld* World = GetWorld())
		{
			// If the interaction isn't allowed, don't enable
			if (APlayerController* PC = GEngine->GetFirstLocalPlayerController(World))
			{
				if (AILLCharacter* LocalPlayer = Cast<AILLCharacter>(PC->GetCharacter()))
				{
					if (!InteractComponent->IsInteractionAllowed(LocalPlayer))
						return;
				}
			}
		}
	}

	if (!IsRunningDedicatedServer())
	{
		for (UMaterialInterface* Material : GetMesh()->GetMaterials())
		{
			if (UMaterialInstanceDynamic* DynamicMat = Cast<UMaterialInstanceDynamic>(Material))
			{
				DynamicMat->SetScalarParameterValue(SCItemNames::NAME_ShimmerParam, bEnabled ? ITEM_SHIMMER_ON : ITEM_SHIMMER_OFF);
			}
		}
	}
}

void ASCItem::BeginItemUse()
{
	bIsInUse = true;
}

void ASCItem::EndItemUse()
{
	bIsInUse = false;
}

void ASCItem::OnRep_NumUses()
{
	if (UILLInventoryComponent* Inventory = GetOuterInventory())
	{
		if (ASCCounselorCharacter* OwningCounselor = Cast<ASCCounselorCharacter>(Inventory->GetOwner()))
		{
			OwningCounselor->UpdateInventoryWidget();
		}
	}
}

void ASCItem::OnRep_Shadows()
{
	if (ItemMesh3P)
		ItemMesh3P->SetCastShadow(bShouldCastShadows);
}

#undef LOCTEXT_NAMESPACE
