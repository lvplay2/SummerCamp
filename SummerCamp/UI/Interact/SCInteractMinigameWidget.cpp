// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCInteractMinigameWidget.h"

#include "SCCharacter.h"
#include "SCCounselorCharacter.h"
#include "ILLInteractableManagerComponent.h"
#include "SCInteractComponent.h"
#include "Image.h"
#include "CanvasPanel.h"
#include "CanvasPanelSlot.h"

#define INTERACT_BUFFER 1.5f

USCInteractMinigameWidget::USCInteractMinigameWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, ProgressImage(nullptr)
, RootCanvas(nullptr)
, InteractionText(FText::GetEmpty())
, ProgressCircleMaterial(nullptr)
, ProgressCircleActiveMaterial(nullptr)
, ActivePip(nullptr)
, LastPipEnd(0.0f)
{
	static ConstructorHelpers::FObjectFinder<UMaterialInstance> ActiveMaterial(TEXT("Material'/Game/UI/Interact/Materials/ProgressCircle_Inst'"));
	if (ActiveMaterial.Object)
	{
		ProgressCircleActiveMaterial = ActiveMaterial.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInstance> Material(TEXT("Material'/Game/UI/Interact/Materials/ProgressCircleFat_Inst'"));
	if (Material.Object)
	{
		ProgressCircleMaterial = Material.Object;
	}

	SetVisibility(ESlateVisibility::Collapsed);

	MaxPipCount = 10;
	MinPipCount = 5;
	MaxPipWidth = 30.0f;
	MinPipWidth = 15.0f;
}

void USCInteractMinigameWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	ASCCharacter* Character = Cast<ASCCharacter>(GetOwningPlayerPawn());
	if (!Character)
		return;

	USCInteractComponent* Interactable = Character->GetInteractable();
	if (!Interactable || (Interactable->InteractMethods & (int32)EILLInteractMethod::Hold) == 0)
		return;

	float interactPercent = Character->GetInteractTimePercent();

	if (interactPercent > 0.f)
	{
		if (ActivePip)
		{
			float pipStart = ActivePip->Position / 360.f;
			float pipEnd = pipStart + (ActivePip->Width + INTERACT_BUFFER) / 360.f;

			if (interactPercent > pipStart)
				ActivePip->State = EInteractMinigamePipState::Active;

			if (interactPercent > pipEnd)
				PrevPip();
		}

		if (ProgressImage)
		{
			if (UMaterialInstanceDynamic* mat = ProgressImage->GetDynamicMaterial())
			{
				mat->SetScalarParameterValue(TEXT("Alpha"), interactPercent);
				mat->SetVectorParameterValue(TEXT("Color"), ProgressColor);
			}
		}

		SetPipColors(interactPercent);
	}
}

float USCInteractMinigameWidget::GetHoldTimer()
{
	ASCCharacter* Character = Cast<ASCCharacter>(GetOwningPlayerPawn());
	if (!Character)
		return 0;

	return Character->GetInteractTimePercent();
}

void USCInteractMinigameWidget::Start()
{
	if (!RootCanvas)
		return;

	ASCCharacter* Character = Cast<ASCCharacter>(GetOwningPlayerPawn());
	if (!Character)
		return;

	USCInteractComponent* Interactable = Character->GetInteractable();
	if (!Interactable)
		return;

	// This will make is so that the first failure will always notify the Killer.
	ConsecutiveFails = Interactable->AllowableFailsBeforeKillerNotify;

	if (Interactable->bRandomizePips)
	{
		Interactable->Pips.Empty();

		int maxPip = MaxPipCount;
		int minPip = MinPipCount;
		float maxPipW = MaxPipWidth;
		float minPipW = MinPipWidth;
		if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character))
		{
			if (Counselor->IsAbilityActive(EAbilityType::Repair))
			{
				minPipW = 70.0f;
				maxPipW = 90.0f;
				maxPip = 3;
				minPip = 1;
			}
			else
			{
				// grab counselor specific stats to adjust ranges
				minPipW += MinPipWidthMod.Get(Counselor);
				maxPipW += MaxPipWidthMod.Get(Counselor);
				maxPip -= (int)MaxPipCountMod.Get(Counselor);
				minPip -= (int)MinPipCountMod.Get(Counselor);
			}
		}

		const int PipCount = FMath::RandRange(minPip, maxPip);
		float PrevStart = 0.0f;
		float PrevWidth = minPipW;
		const float PipPrecent = (350.0f / (float) PipCount);
		for (int i(0); i<PipCount; ++i)
		{
			FInteractMinigamePip NewPip;

			const float NewLoc =  PipPrecent * (float)i;
			PrevStart = NewLoc > PrevStart ? NewLoc : PrevStart;

			NewPip.Position = FMath::RandRange(PrevStart+PrevWidth, FMath::Min(PrevStart+PrevWidth+maxPipW, 350.0f-maxPipW));
			PrevStart = NewPip.Position;
			NewPip.Width = FMath::FRandRange(minPipW, maxPipW);
			PrevWidth = NewPip.Width + 10.0f;
			NewPip.State = EInteractMinigamePipState::None;
			NewPip.WidgetRef = nullptr;
			NewPip.InteractKey = FMath::Rand()%2;

			Interactable->Pips.Add(NewPip);

			if (PrevStart+PrevWidth+maxPipW >= 350.0f)
				break;
		}
	}

	if (Interactable->Pips.Num() == 0)
		return;

	SetVisibility(ESlateVisibility::Visible);

	if (ProgressCircleMaterial)
	{
		for (auto& pip : Interactable->Pips)
		{
			pip.WidgetRef = NewObject<UImage>(UImage::StaticClass());
			pip.WidgetRef->SetVisibility(ESlateVisibility::Visible);
			pip.WidgetRef->SetBrushFromMaterial(ProgressCircleMaterial);
			
			UMaterialInstanceDynamic* mat = pip.WidgetRef->GetDynamicMaterial();
			mat->SetScalarParameterValue(TEXT("Alpha"), pip.Width / 360.f);
			mat->SetVectorParameterValue(TEXT("Color"), PipDefaultColor);
			pip.WidgetRef->SetRenderAngle(pip.Position);

			RootCanvas->AddChildToCanvas(pip.WidgetRef);

			UCanvasPanelSlot* slot = Cast<UCanvasPanelSlot>(pip.WidgetRef->Slot);
			slot->SetSize(FVector2D(256.0f, 256.0f));

			InteractPips.AddTail(&pip);
		}

		ActivePip = InteractPips.GetHead()->GetValue();

		SetPipColors(0.0f);
	}

	LoopSound = UGameplayStatics::CreateSound2D(GetWorld(), Interactable->MinigameLoopCue);
	if (LoopSound)
		LoopSound->Play();
}

void USCInteractMinigameWidget::Stop()
{
	if (!RootCanvas)
		return;

	SetVisibility(ESlateVisibility::Collapsed);

	for (FInteractMinigamePip* Pip : InteractPips)
	{
		if (Pip)
		{
			Pip->WidgetRef->SetVisibility(ESlateVisibility::Collapsed);
			RootCanvas->RemoveChild(Pip->WidgetRef);
			Pip->State = EInteractMinigamePipState::None;
		}
	}

	InteractPips.Empty();
	ActivePip = nullptr;

	if (LoopSound)
	{
		LoopSound->Stop();
		LoopSound->DestroyComponent();
		LoopSound = nullptr;
	}
}

void USCInteractMinigameWidget::OnInteractSkill(int InteractKey)
{
	if (!ActivePip)
		return;

	ASCCharacter* Character = Cast<ASCCharacter>(GetOwningPlayerPawn());
	if (!Character)
		return;

	USCInteractComponent* Interactable = Character->GetInteractable();
	if (!Interactable)
		return;

	const float interactPercent = Character->GetInteractTimePercent();
	const float pipStart = ActivePip->Position / 360.f;
	const float pipEnd = pipStart + (ActivePip->Width + INTERACT_BUFFER) / 360.f;

	if (interactPercent < LastPipEnd)
		return;

	if (interactPercent > pipStart && interactPercent <= pipEnd && InteractKey == ActivePip->InteractKey)
	{
		LastPipEnd = pipEnd;
		ActivePip->State = EInteractMinigamePipState::Hit;
		Character->SetInteractSpeedModifier(Character->GetInteractSpeedModifier() + 0.5f);
		NextPip();
		UGameplayStatics::PlaySound2D(GetWorld(), Interactable->MinigameSuccessCue);
	}
	else
	{
		PrevPip();
	}
}

bool USCInteractMinigameWidget::IsCurrentPipActive() const
{
	if (ActivePip)
	{
		return ActivePip->State == EInteractMinigamePipState::Active;
	}

	return false;
}

int USCInteractMinigameWidget::GetCurrentPipKey() const
{
	if (ActivePip)
		return ActivePip->InteractKey;

	return -1;
}

void USCInteractMinigameWidget::NextPip()
{
	if (auto nextNode = InteractPips.FindNode(ActivePip)->GetNextNode())
	{
		ActivePip = nextNode->GetValue();
	}
	else
	{
		ActivePip = nullptr;
	}

	OnSuccess();
}

void USCInteractMinigameWidget::PrevPip()
{
	ASCCharacter* Character = Cast<ASCCharacter>(GetOwningPlayerPawn());
	if (!Character)
		return;

	USCInteractComponent* Interactable = Character->GetInteractable();
	if (!Interactable)
		return;

	ActivePip->State = EInteractMinigamePipState::Miss;
	Character->SetInteractSpeedModifier(1.f);

	if (auto preNode = InteractPips.FindNode(ActivePip)->GetPrevNode())
	{
		float prevStart = preNode->GetValue()->Position / 360.f;
		float prevEnd = prevStart + (preNode->GetValue()->Width + INTERACT_BUFFER) / 360.f;
		if (Character->GetInteractTimePercent() > prevEnd)
			Character->SetInteractTimePercent(prevEnd);
	}
	else
	{
		Character->SetInteractTimePercent(0.0f);
	}

	float nextStart = 350.0f;
	if (auto NextNode = InteractPips.FindNode(ActivePip)->GetNextNode())
	{
		nextStart = NextNode->GetValue()->Position-10.0f;
	}

	ActivePip->Width += ActivePip->Width * 0.25f;
	
	if (ActivePip->Width + ActivePip->Position > nextStart)
		ActivePip->Width = nextStart - ActivePip->Position;

	UMaterialInstanceDynamic* mat = ActivePip->WidgetRef->GetDynamicMaterial();
	mat->SetScalarParameterValue(TEXT("Alpha"), ActivePip->Width / 360.f);

	OnFailure();

	UGameplayStatics::PlaySound2D(GetWorld(), Interactable->MinigameFailCue);

	if (++ConsecutiveFails >= Interactable->AllowableFailsBeforeKillerNotify)
	{
		ConsecutiveFails = 0;
		if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character))
			Counselor->Native_NotifyKillerOfMinigameFail(Interactable->NotifyKillerMinigameFailCue);
	}

	Character->FailureInteractAnim();
}

void USCInteractMinigameWidget::SetPipColors(float Percentage)
{
	ASCCharacter* Character = Cast <ASCCharacter>(GetOwningPlayerPawn());
	if (!Character)
		return;

	USCInteractComponent* Interactable = Character->GetInteractable();
	if (!Interactable || (Interactable->InteractMethods & (int32)EILLInteractMethod::Hold) == 0)
		return;

	for (auto pip : InteractPips)
	{
		if (pip->WidgetRef)
		{
			UMaterialInstanceDynamic* mat = pip->WidgetRef->GetDynamicMaterial();

			switch (pip->State)
			{
			case EInteractMinigamePipState::None:
				mat->SetVectorParameterValue("Color", PipDefaultColor);
				break;

			case EInteractMinigamePipState::Active:
				mat->SetVectorParameterValue("Color", PipActiveColor);
				break;

			case EInteractMinigamePipState::Hit:
			{
				if (pip->bIsFat)
				{
					pip->WidgetRef->SetBrushFromMaterial(ProgressCircleActiveMaterial);
					mat = pip->WidgetRef->GetDynamicMaterial();
					pip->bIsFat = false;
				}

				const float End = (pip->Position / 360.f) + (pip->Width + INTERACT_BUFFER) / 360.f;
				mat->SetScalarParameterValue(TEXT("Alpha"), pip->Width / 360.f);
				mat->SetVectorParameterValue("Color", Percentage > End ? PipActiveColor : PipHitColor);
				break;
			}

			case EInteractMinigamePipState::Miss:
				mat->SetVectorParameterValue("Color", PipMissColor);
				break;

			default:
				break;
			}
		}
	}
}

FLinearColor USCInteractMinigameWidget::GetActivePipColor() const
{
	if (ActivePip)
	{
		switch (ActivePip->State)
		{
			case EInteractMinigamePipState::None:
				return FLinearColor(PipDefaultColor.R,PipDefaultColor.G,PipDefaultColor.B, 1.0f);
			case EInteractMinigamePipState::Active:
				return FLinearColor(PipActiveColor.R, PipActiveColor.G, PipActiveColor.B, 1.0f);
			case EInteractMinigamePipState::Hit:
				return FLinearColor(PipHitColor.R, PipHitColor.G, PipHitColor.B, 1.0f);
			case EInteractMinigamePipState::Miss:
				return FLinearColor(PipMissColor.R, PipMissColor.G, PipMissColor.B, 1.0f);
		}
	}

	return FLinearColor(PipDefaultColor.R, PipDefaultColor.G, PipDefaultColor.B, 1.0f);;
}
