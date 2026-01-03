// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCActorSpawnerComponent.h"

#include "SCGameState.h"

#include "UObjectToken.h"
#include "MessageLog.h"

USCActorSpawnerComponent::USCActorSpawnerComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	bWantsInitializeComponent = true;

	SetIsReplicated(true);

	Billboard = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
#if WITH_EDITORONLY_DATA
	if (!IsRunningCommandlet())
	{
		// Structure to hold one-time initialization
		struct FBillboardConstructorStatics
		{
			ConstructorHelpers::FObjectFinderOptional<UTexture2D> BillboardTexture;
			FName ID_Billboard;
			FText NAME_Billboard;
			FBillboardConstructorStatics()
				: BillboardTexture(TEXT("/Engine/EditorResources/EmptyActor"))
				, ID_Billboard(TEXT("ActorSpawner"))
				, NAME_Billboard(NSLOCTEXT("ActorSpawnerCategory", "ActorSpawner", "ActorSpawner"))
			{
			}
		};
		static FBillboardConstructorStatics BillboardConstructorStatics;

		if (Billboard)
		{
			Billboard->Sprite = BillboardConstructorStatics.BillboardTexture.Get();
			Billboard->RelativeScale3D = FVector(0.5f, 0.5f, 0.5f);
			Billboard->bHiddenInGame = true;
			Billboard->SpriteInfo.Category = BillboardConstructorStatics.ID_Billboard;
			Billboard->SpriteInfo.DisplayName = BillboardConstructorStatics.NAME_Billboard;
			Billboard->bAbsoluteScale = true;
			Billboard->bIsScreenSizeScaled = true;
		}
	}
#endif // WITH_EDITORONLY_DATA
}

// Called when the game starts
void USCActorSpawnerComponent::InitializeComponent()
{
	Super::InitializeComponent();

	// Make sure we only have valid items to spawn in our actor list
	for (int32 iActor(0); iActor < ActorList.Num(); ++iActor)
	{
		if (ActorList[iActor] == nullptr)
		{
			UE_LOG(LogSC, Warning, TEXT("%s:%s has an invalid actor in their actor list of %d actor(s), removing"), *GetName(), *GetOwner()->GetName(), ActorList.Num());
			ActorList.RemoveAt(iActor--); // Remove at this location and decrement to make sure we check ALL items
		}
	}


	if (bSpawnOnInitialize)
	{
		SpawnActor();
	}

#if WITH_EDITORONLY_DATA
	if (Billboard)
	{
		Billboard->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
#endif // WITH_EDITORONLY_DATA
}

void USCActorSpawnerComponent::OnRegister()
{
//#if WITH_EDITOR
//	Super::OnRegister();
//
//	if (ActorList.Num() > 0)
//		SetChildActorClass(ActorList[0]);
//	else
//	{
//		DestroyChildActor();
//		SetChildActorClass(nullptr);
//	}
//#else
	SetChildActorClass(nullptr);

	Super::OnRegister();
//#endif

	if (UWorld* World = GetWorld())
	{
		if (ASCGameState* GS = World->GetGameState<ASCGameState>())
		{
			GS->ActorSpawnerList.AddUnique(this);
		}
	}
}

void USCActorSpawnerComponent::OnUnregister()
{
	if (UWorld* World = GetWorld())
	{
		if (ASCGameState* GS = World->GetGameState<ASCGameState>())
		{
			GS->ActorSpawnerList.Remove(this);
		}
	}

	Super::OnUnregister();
}

void USCActorSpawnerComponent::DestroySpawnedActor()
{
	if (IsValid(SpawnedActor))
	{
		SpawnedActor->Destroy();
		SpawnedActor = nullptr;
	}
}

bool USCActorSpawnerComponent::SpawnActor(TSubclassOf<AActor> ClassOverride /* = nullptr*/)
{
	if (ActorList.Num() > 0)
	{
		// destroy preview mesh...
		DestroyChildActor();

		if (SpawnedActor)
		{
			return false;
		}

		int32 index = INDEX_NONE;
		if (ClassOverride)
		{
			TArray<int32> indices;
			for (int32 i(0); i < ActorList.Num(); ++i)
			{
				if (ActorList[i]->IsChildOf(ClassOverride))
				{
					indices.Add(i);
					break;
				}
			}

			if (indices.Num() > 0)
			{
				index = indices[FMath::RandHelper(indices.Num())];
			}

			// Class we want to spawn not supported in this spawner. Return FAILURE!!!!!
			if (index == INDEX_NONE)
			{
				return false;
			}
		}
		else
		{
			index = FMath::RandHelper(ActorList.Num());
		}

		if (AActor* DefaultActor = Cast<AActor>(ActorList[index]->GetDefaultObject()))
		{
			if (DefaultActor->GetIsReplicated() && GetOwner()->Role < ROLE_Authority)
				return false;
		}

		FActorSpawnParameters SpawnInfo;
		SpawnInfo.Owner = GetOwner();
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnInfo.ObjectFlags |= EObjectFlags::RF_Transient;
		SpawnedActor = GetWorld()->SpawnActor<AActor>(ActorList[index], GetComponentTransform(), SpawnInfo);
	}

	return SpawnedActor != nullptr;
}

bool USCActorSpawnerComponent::SupportsType(const TSubclassOf<AActor> Type) const
{
	for (auto ActorType : ActorList)
	{
		if (ActorType->IsChildOf(Type))
		{
			return true;
		}
	}

	return false;
}

#if WITH_EDITOR
void USCActorSpawnerComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	//static const FName NAME_ChildActorClass = GET_MEMBER_NAME_CHECKED(USCActorSpawnerComponent, ActorList);
	//const bool UpdatePreview = (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == NAME_ChildActorClass);

	//if (UpdatePreview)
	//{
	//	if (ActorList.Num() > 0)
	//		SetChildActorClass(ActorList[0]);
	//	else
	//	{
	//		DestroyChildActor();
	//		SetChildActorClass(nullptr);
	//	}
	//}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

#include "SCWindow.h"
#include "SCDoor.h"
#include "SCCabinet.h"

void USCActorSpawnerComponent::CheckForErrors()
{
	Super::CheckForErrors();

	FMessageLog MapCheck("MapCheck");

	// If you know of a class that should ALWAYS spawn on initialize, add it to this list
	const UClass* BadClasses[] = { ASCWindow::StaticClass(), ASCDoor::StaticClass(), ASCCabinet::StaticClass(), ThisClass::StaticClass() };

	if (!bSpawnOnInitialize && ActorList.Num() > 0)
	{
		const UClass* TestClass = ActorList[0];
		for (const auto& BadClass : BadClasses)
		{
			if (TestClass->IsChildOf(BadClass) || TestClass == BadClass)
			{
				MapCheck.Error()
					->AddToken(FUObjectToken::Create(this))
					->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("%s wants to spawn a %s. This should be set to spawn on initialize!"), *GetName(), *ActorList[0]->GetName()))));
				break;
			}
		}
	}
}
#endif
