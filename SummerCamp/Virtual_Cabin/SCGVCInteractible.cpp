// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGVCInteractible.h"
#include "SCGVCInteractibleWidget.h"

//debug log
#define VCprint(text) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.5, FColor::White,text)

// Sets default values
ASCGVCInteractible::ASCGVCInteractible(const FObjectInitializer& ObjectInitializer)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	m_CollisionComponent = ObjectInitializer.CreateDefaultSubobject<UBoxComponent>(this, TEXT("BoxComp"));
	m_CollisionComponent->InitBoxExtent(FVector(1, 1, 1));
	SetRootComponent(m_CollisionComponent);
	bReplicates = true;

	m_ExaminePosition = FVector(50, 0, -5);
}

// Called when the game starts or when spawned
void ASCGVCInteractible::BeginPlay()
{
	Super::BeginPlay();
	
	// Get a reference to the actor's mesh component
	TArray<USceneComponent*> AllChildrenComponents;
	m_CollisionComponent->GetChildrenComponents(false, AllChildrenComponents);
	for (int i = 0; i < AllChildrenComponents.Num(); ++i)
	{
		if (AllChildrenComponents[i] == nullptr)
			continue;

		if (AllChildrenComponents[i]->IsA(UStaticMeshComponent::StaticClass()) || AllChildrenComponents[i]->IsA(USkeletalMeshComponent::StaticClass()))
		{
			m_MeshComponent = Cast<UMeshComponent>(AllChildrenComponents[i]);
		}
		if (AllChildrenComponents[i]->IsA(USCGVCInteractibleWidget::StaticClass()))
		{
			m_InteractWidgetRef = Cast<USCGVCInteractibleWidget>(AllChildrenComponents[i]);
		}
	}

	SetItemUsableOnInteractible(b_ItemUsableOnInteractible);
}

// Called every frame
void ASCGVCInteractible::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

FText ASCGVCInteractible::GetName()
{
	return m_Name;
}

UMeshComponent* ASCGVCInteractible::GetMeshComponent()
{
	return m_MeshComponent;
}

void ASCGVCInteractible::OnStartInteracting_Implementation(ASCGVCCharacter* VCChar, ASCGVCPlayerController* VCController)
{	
	if (this->OnStartInteractingDelegate.IsBound())
		this->OnStartInteractingDelegate.Broadcast();
}

void ASCGVCInteractible::OnStopInteracting_Implementation(ASCGVCCharacter* VCChar, ASCGVCPlayerController* VCController)
{
	if (this->OnStopInteractingDelegate.IsBound())
		this->OnStopInteractingDelegate.Broadcast();
}

void ASCGVCInteractible::OnItemUsed_Implementation(ASCGVCCharacter* VCChar, ASCGVCPlayerController* VCController, ASCGVCItem* VCItem)
{
	if (this->OnItemUsedDelegate.IsBound())
		this->OnItemUsedDelegate.Broadcast();
}

void ASCGVCInteractible::OnActionPressedWhenExamined_Implementation(ASCGVCCharacter* VCChar, ASCGVCPlayerController* VCController)
{
	if (this->OnActionPressedWhenExaminedDelegate.IsBound())
		this->OnActionPressedWhenExaminedDelegate.Broadcast();
}

void ASCGVCInteractible::OnToggleProximityBP_Implementation(bool Toggle, bool instant)
{
}

void ASCGVCInteractible::OnToggleLookedAtBP_Implementation(bool Toggle, bool instant)
{
}

void ASCGVCInteractible::ToggleLookedAtEffect(bool Toggle, bool instant)
{
	if (!m_ShowInteractibleEffect)
		return;

	OnToggleLookedAtBP(Toggle, instant);
}

void ASCGVCInteractible::SetItemUsableOnInteractible(bool newToggle)
{
	b_ItemUsableOnInteractible = newToggle;
	if (m_InteractWidgetRef != nullptr)
	{
		m_InteractWidgetRef->m_ShowBackPackPip = newToggle;
	}
}

void ASCGVCInteractible::ToggleProximityIndicator(bool Toggle, bool instant)
{
	if (!m_ShowInteractibleEffect)
	{
		OnToggleProximityBP(false, true);
		return;
	}

	if (Toggle && !m_isPlayerInProximity)
		return;

	OnToggleProximityBP(Toggle, instant);
}

void ASCGVCInteractible::ToggleHidden(bool hide)
{
	this->SetActorHiddenInGame(hide);
}

void ASCGVCInteractible::SetInteractibleInWorld(bool interactible)
{
	m_IsInteractibleInWorld = interactible;
	if (m_InteractWidgetRef != nullptr)
	{
		m_InteractWidgetRef->m_ShowInteractibleEffect = interactible;
	}
}

void ASCGVCInteractible::ToggleCollision(bool collide)
{
	ECollisionEnabled::Type colltype;

	if (collide)
		colltype = ECollisionEnabled::QueryAndPhysics;
	else
		colltype = ECollisionEnabled::NoCollision;

	if (m_CollisionComponent != nullptr)
		m_CollisionComponent->SetCollisionEnabled(colltype);
	if (m_MeshComponent != nullptr)
		m_MeshComponent->SetCollisionEnabled(colltype);
}