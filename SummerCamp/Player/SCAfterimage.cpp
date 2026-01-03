// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCAfterimage.h"


// Sets default values
ASCAfterimage::ASCAfterimage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, SkeletalMaster(nullptr)
	, StaticMaster(nullptr)
	, bFollowMaster(false)
	, bFadeIn(false)
	, FadeInTime(3.0f)
	, bFullyFadedIn(false)
	, bFadeOut(false)
	, FadeOutTime(3.0f)
	, bBeginFadeOut(false)
	, FadeTimer(0.0f)
	, FadeInCurve(nullptr)
	, FadeOutCurve(nullptr)
	, FadeDistanceCurve(nullptr)
	, DistanceTarget(nullptr)
	, MaxDistanceFromTarget(1.f)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root Component"));

	SkelMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Skeletal Mesh"));
	SkelMesh->SetupAttachment(RootComponent);

	StatMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh"));
	StatMesh->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void ASCAfterimage::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASCAfterimage::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

	if (bFollowMaster)
	{
		if (SkelMesh && SkeletalMaster && SkelMesh->IsVisible())
		{
			SkelMesh->SetRelativeRotation(SkeletalMaster->GetRelativeTransform().GetRotation());
			SetActorLocationAndRotation(SkeletalMaster->GetComponentLocation(), SkeletalMaster->GetAttachmentRootActor()->GetActorRotation());
		}

		if (StatMesh && StaticMaster && StatMesh->IsVisible())
		{
			StatMesh->SetRelativeTransform(StaticMaster->GetRelativeTransform());
			SetActorLocationAndRotation(StaticMaster->GetComponentLocation(), StaticMaster->GetAttachmentRootActor()->GetActorRotation());

		}
	}

	UpdateFade(DeltaTime);
}

void ASCAfterimage::SlaveToMesh(USkeletalMeshComponent* OtherSkeletalMesh, bool follow)
{
	bFollowMaster = follow;
	SkeletalMaster = OtherSkeletalMesh;
	SetActorLocationAndRotation(SkeletalMaster->GetComponentLocation(), SkeletalMaster->GetAttachmentRootActor()->GetActorRotation());
	SkelMesh->SetMasterPoseComponent(OtherSkeletalMesh);
	SkelMesh->SetVisibility(true);
	SkelMesh->SetRelativeRotation(OtherSkeletalMesh->GetRelativeTransform().GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);
}

void ASCAfterimage::SlaveToMesh(UStaticMeshComponent* OtherStaticMesh, bool follow)
{
	bFollowMaster = follow;
	StaticMaster = OtherStaticMesh;
	SetActorLocationAndRotation(StaticMaster->GetComponentLocation(), StaticMaster->GetAttachmentRootActor()->GetActorRotation());
	StatMesh->SetVisibility(true);
}

void ASCAfterimage::FreezeAnim()
{
	SkelMesh->PrimaryComponentTick.SetTickFunctionEnable(false);
}

void ASCAfterimage::ResumeAnim()
{
	SkelMesh->PrimaryComponentTick.SetTickFunctionEnable(true);
}

void ASCAfterimage::ChangeAfterimageMesh(USkeletalMesh* newMesh)
{
	SkelMesh->SetSkeletalMesh(newMesh);
	SkelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASCAfterimage::ChangeAfterimageMesh(UStaticMesh* newMesh)
{
	StatMesh->SetStaticMesh(newMesh);
	StatMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASCAfterimage::StopFollow()
{
	bFollowMaster = false;
}

void ASCAfterimage::ResumeFollow()
{
	bFollowMaster = true;
}

void ASCAfterimage::SetVisibility(bool Skel, bool Static)
{
	if (SkelMesh)
		SkelMesh->SetVisibility(Skel);
	if (StatMesh)
		StatMesh->SetVisibility(Static);
}

void ASCAfterimage::UpdateFade(float DeltaSeconds)
{
	if (!IsRunningDedicatedServer())
		return;

	FadeTimer += DeltaSeconds;

	int32 Index = 0;
	TArray<UMaterialInterface*> Materials = SkelMesh->GetMaterials();
	for (UMaterialInterface* MatInt : Materials)
	{
		bool UpdateMaterial = false;
		float NewAlpha = 1.f;
		if (FadeDistanceCurve && DistanceTarget)
		{
			const float distRatio = FVector::DistSquared(DistanceTarget->GetActorLocation(), SkelMesh->GetComponentLocation()) / FMath::Square(MaxDistanceFromTarget);
			NewAlpha = FadeDistanceCurve->GetFloatValue(distRatio);
			UpdateMaterial = true;
		}

		if (bFadeIn && !bFullyFadedIn)
		{
			if (FadeInCurve)
			{
				NewAlpha = FMath::Min(NewAlpha, FadeInCurve->GetFloatValue(FadeTimer));
			}
			else
			{
				NewAlpha = FMath::Min(NewAlpha, FadeTimer / FadeInTime);
			}

			if (FadeTimer >= FadeInTime)
			{
				bFullyFadedIn = true;
				FadeTimer = 0.f;
			}
			UpdateMaterial = true;
		}

		if (bFadeOut && bBeginFadeOut)
		{
			if (FadeOutCurve)
			{
				NewAlpha = FMath::Min(NewAlpha, FadeOutCurve->GetFloatValue(FadeTimer));
			}
			else
			{
				NewAlpha = FMath::Min(NewAlpha, 1 - (FadeTimer / FadeOutTime));
			}

			if (FadeTimer >= FadeOutTime)
			{
				// what do we do when fully faded out?
			}
			UpdateMaterial = true;
		}

		if (UpdateMaterial)
		{
			if (UMaterialInstanceDynamic* DynamicMat = Cast<UMaterialInstanceDynamic>(MatInt))
			{
				FLinearColor existingColor = FLinearColor::Transparent;
				DynamicMat->GetVectorParameterValue("Color", existingColor);
				existingColor.A = NewAlpha;
				DynamicMat->SetVectorParameterValue("Color", existingColor);
			}
			else
			{
				if (UMaterialInstanceDynamic* NewDynamicMat = SkelMesh->CreateDynamicMaterialInstance(Index, MatInt))
				{
					FLinearColor existingColor = FLinearColor::Transparent;
					NewDynamicMat->GetVectorParameterValue("Color", existingColor);
					existingColor.A = NewAlpha;
					NewDynamicMat->SetVectorParameterValue("Color", existingColor);
				}
			}
		}

		++Index;
	}
}

void ASCAfterimage::SetDistanceTarget(AActor* Target, float MaxDistance)
{
	DistanceTarget = Target;
	MaxDistanceFromTarget = FMath::Max(SMALL_NUMBER, MaxDistance);
}
