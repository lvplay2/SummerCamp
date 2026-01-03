// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "Components/ArrowComponent.h"
#include "SCGVCItem.h"

//debug log
#define VCprint(text) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.5, FColor::White,text)

// Sets default values
ASCGVCItem::ASCGVCItem(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	m_RotationOffsetArrow = ObjectInitializer.CreateDefaultSubobject<UArrowComponent>(this, TEXT("RotationOffsetPivot"));
	m_RotationOffsetArrow->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	//m_RotationOffsetArrow->SetRelativeLocationAndRotation(FVector(0, 0, 0), FQuat(0, 0, 0, 0));

	m_WorldAnchorPosition = GetActorLocation();
	m_WorldAnchorRotation = GetActorRotation();
}

void ASCGVCItem::BeginPlay()
{
	Super::BeginPlay();
}

void ASCGVCItem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void ASCGVCItem::SnapToAnchor()
{
	SetActorLocation(m_WorldAnchorPosition);
	SetActorRotation(m_WorldAnchorRotation);

	OnInvokeSnapToAnchorCue();
}

void ASCGVCItem::SetToForwardRendering()
{
	if (m_MeshComponent != nullptr)
	{
		m_MeshComponent->SetRenderCustomDepth( true );
		m_MeshComponent->ViewOwnerDepthPriorityGroup = SDPG_Foreground;
		m_MeshComponent->SetDepthPriorityGroup(SDPG_Foreground);
	}
}

void ASCGVCItem::SetToWorldRendering()
{
	if (m_MeshComponent != nullptr)
	{
		m_MeshComponent->SetRenderCustomDepth( false );
		m_MeshComponent->ViewOwnerDepthPriorityGroup = SDPG_World;
		m_MeshComponent->DepthPriorityGroup = SDPG_World;
	}
}

FString ASCGVCItem::GetItemID()
{
	return m_ItemID;
}

UArrowComponent* ASCGVCItem::GetRotationPivot()
{
	return m_RotationOffsetArrow;
}

void ASCGVCItem::SetItemToExamineRotationPivot()
{
	if (m_MeshComponent != nullptr && m_RotationOffsetArrow != nullptr)
	{
		m_MeshPrevRelativePosition = m_MeshComponent->RelativeLocation;
		m_MeshPrevRelativeRotation = m_MeshComponent->RelativeRotation;

		m_MeshComponent->AttachToComponent(m_RotationOffsetArrow, FAttachmentTransformRules::KeepWorldTransform);

		m_OrigScale = m_MeshComponent->RelativeScale3D;
		m_MeshComponent->SetWorldScale3D(FVector(m_ScaleWhenExamining, m_ScaleWhenExamining, m_ScaleWhenExamining));
	}
}

void ASCGVCItem::SetItemToNormalLocationAndPivot()
{
	if (m_MeshComponent != nullptr && m_RotationOffsetArrow != nullptr)
	{
		m_MeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		m_MeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
		m_MeshComponent->SetRelativeLocationAndRotation(m_MeshPrevRelativePosition, FQuat(m_MeshPrevRelativeRotation));
		m_MeshComponent->SetWorldScale3D(m_OrigScale);
		m_RotationOffsetArrow->SetRelativeRotation(FQuat(0, 0, 0, 0));
	}
}

void ASCGVCItem::OnDropped_Implementation()
{
}

void ASCGVCItem::OnPickedUp_Implementation()
{
}

void ASCGVCItem::OnInvokePickUpCue_Implementation()
{}

void ASCGVCItem::OnInvokeDropCue_Implementation()
{}

void ASCGVCItem::OnInvokeSnapToAnchorCue_Implementation()
{}

void ASCGVCItem::OnInvokeEnteredExamineModeCue_Implementation()
{}