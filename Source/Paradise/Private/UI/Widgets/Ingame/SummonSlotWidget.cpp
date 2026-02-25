// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Ingame/SummonSlotWidget.h"
#include "UI/Widgets/Ingame/ParadiseCommonButton.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"

USummonSlotWidget::USummonSlotWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#pragma region 생명주기
void USummonSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 입력은 버튼에게 위임하고, 위젯은 로직만 처리합니다.
	if (Btn_SummonAction)
	{
		Btn_SummonAction->OnClicked().AddUObject(this, &USummonSlotWidget::OnSummonButtonClicked);
	}

	//StopCooldownTimer();
}

void USummonSlotWidget::NativeDestruct()
{
	//StopCooldownTimer();
	if (Btn_SummonAction)
	{
		// 델리게이트 해제는 안전하게
		Btn_SummonAction->OnClicked().RemoveAll(this);
	}
	Super::NativeDestruct();
}
#pragma endregion 생명주기

#pragma region 외부 인터페이스 구현
void USummonSlotWidget::InitSlot(int32 InIndex)
{
	SlotIndex = InIndex;
}

void USummonSlotWidget::UpdateSlotInfo(UTexture2D* IconTexture, int32 InCost)
{

	// 1. 아이콘 처리
	if (Img_SummonIcon)
	{
		// 텍스처가 있으면 넣고, 없으면 그냥 흰색 네모라도 뜨게 둠
		if (IconTexture)
		{
			Img_SummonIcon->SetBrushFromTexture(IconTexture);
			Img_SummonIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			// 데이터가 없으면 아이콘을 숨깁니다 (혹은 투명 슬롯 처리)
			Img_SummonIcon->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	// 2. 버튼 처리
	if (Btn_SummonAction)
	{
		// 테스트용: 무조건 활성화 (데이터 없어도 눌러서 애니메이션 테스트 가능하게)
		Btn_SummonAction->SetIsEnabled(true);
	}

	// 3. 텍스트 처리
	if (Text_CostValue)
	{
		Text_CostValue->SetText(FText::AsNumber(InCost));
		Text_CostValue->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

//	// 1. 아이콘 처리
//	if (Img_SummonIcon)
//	
//			if (Btn_SummonAction) Btn_SummonAction->SetIsEnabled(true);
//		{
//		if (IconTexture)
//		{
//			Img_SummonIcon->SetBrushFromTexture(IconTexture);
//			Img_SummonIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
//}
//		else
//		{
//			// 아이콘이 없으면 숨김 (혹은 빈 슬롯 이미지)
//			Img_SummonIcon->SetVisibility(ESlateVisibility::Hidden);
//
//			if (Btn_SummonAction) Btn_SummonAction->SetIsEnabled(false);
//		}
//	}
//
//	// 2. 텍스트 처리
//	if (Text_CostValue)
//	{
//		Text_CostValue->SetText(FText::AsNumber(InCost));
//		Text_CostValue->SetVisibility(ESlateVisibility::HitTestInvisible);
//	}
}

void USummonSlotWidget::ScheduleReveal(UTexture2D* IconTexture, int32 InCost, float DelayTime)
{
	// 1. 기존 타이머 리셋 및 데이터 임시 저장 (캡슐화)
	GetWorld()->GetTimerManager().ClearTimer(RevealTimerHandle);
	PendingIcon = IconTexture;
	PendingCost = InCost;

	// 2. 당겨질 때 '빈 칸'으로 보이도록 UI 강제 숨김 처리
	if (Img_SummonIcon) Img_SummonIcon->SetVisibility(ESlateVisibility::Hidden);
	if (Text_CostValue) Text_CostValue->SetVisibility(ESlateVisibility::Hidden);

	// 아직 비어있는 칸이므로 클릭을 막아 연타 버그를 예방합니다.
	if (Btn_SummonAction) Btn_SummonAction->SetIsEnabled(false);

	// 3. 타이머 가동 (DelayTime 초 뒤에 OnRevealTimerFinished 호출)
	GetWorld()->GetTimerManager().SetTimer(RevealTimerHandle, this, &USummonSlotWidget::OnRevealTimerFinished, DelayTime, false);
}

void USummonSlotWidget::PlayIntroAnimation()
{
	if (Anim_Intro)
	{
		// 처음부터 재생 (Forward), 1배속, 루프 없음
		PlayAnimation(Anim_Intro, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);
	}
}

void USummonSlotWidget::PlayShiftAnimation()
{
	if (Anim_Shift)
	{
		// 처음부터 1배속으로 재생
		PlayAnimation(Anim_Shift, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);
	}
}

//void USummonSlotWidget::RefreshCooldown(float CurrentTime, float MaxTime)
//{
//	CurrentCooldownTime = CurrentTime;
//	MaxCooldownTime = MaxTime;
//
//	if (CurrentCooldownTime > 0.0f)
//	{
//		// 쿨타임 시작: 버튼 비활성화 및 타이머 가동
//		if (Btn_SummonAction) Btn_SummonAction->SetIsEnabled(false);
//
//		if (PB_Cooldown) PB_Cooldown->SetVisibility(ESlateVisibility::HitTestInvisible);
//		if (Text_CooldownTime) Text_CooldownTime->SetVisibility(ESlateVisibility::HitTestInvisible);
//
//		if (GetWorld() && !GetWorld()->GetTimerManager().IsTimerActive(CooldownTimerHandle))
//		{
//			GetWorld()->GetTimerManager().SetTimer(CooldownTimerHandle, this, &USummonSlotWidget::UpdateCooldownVisual, UpdateInterval, true);
//		}
//	}
//	else
//	{
//		StopCooldownTimer();
//	}
//}
#pragma endregion 외부 인터페이스 구현

#pragma region 내부 로직
void USummonSlotWidget::OnSummonButtonClicked()
{
	// 인덱스가 유효한 경우에만 상위 패널로 이벤트 전파
	if (SlotIndex >= 0)
	{
		OnSlotClicked.Broadcast(SlotIndex);
	}
}

void USummonSlotWidget::OnRevealTimerFinished()
{
	// 지연 시간이 만료되었으므로, 저장해둔 임시 데이터로 UI를 실제 갱신하고 애니메이션을 재생합니다.
	UpdateSlotInfo(PendingIcon, PendingCost);
	PlayIntroAnimation();

	// 피드백 복구: 다시 선명하게 만듦
	if (Img_SummonIcon) Img_SummonIcon->SetOpacity(1.0f);
}

//void USummonSlotWidget::UpdateCooldownVisual()
//{
//	CurrentCooldownTime -= UpdateInterval;
//
//	if (CurrentCooldownTime <= 0.0f)
//	{
//		StopCooldownTimer();
//		return;
//	}
//
//	if (PB_Cooldown && MaxCooldownTime > 0.0f)
//	{
//		PB_Cooldown->SetPercent(CurrentCooldownTime / MaxCooldownTime);
//	}
//
//	if (Text_CooldownTime)
//	{
//		Text_CooldownTime->SetText(FText::AsNumber(FMath::CeilToInt(CurrentCooldownTime)));
//	}
//}

//void USummonSlotWidget::StopCooldownTimer()
//{
//	if (GetWorld())
//	{
//		GetWorld()->GetTimerManager().ClearTimer(CooldownTimerHandle);
//	}
//
//	if (PB_Cooldown)
//	{
//		PB_Cooldown->SetPercent(0.0f);
//		PB_Cooldown->SetVisibility(ESlateVisibility::Collapsed);
//	}
//
//	if (Text_CooldownTime)
//	{
//		Text_CooldownTime->SetVisibility(ESlateVisibility::Collapsed);
//	}
//
//	// 쿨타임 종료 시 버튼 활성화 (단, 아이콘이 있어야 함)
//	if (Btn_SummonAction && Img_SummonIcon && Img_SummonIcon->GetVisibility() != ESlateVisibility::Hidden)
//	{
//		Btn_SummonAction->SetIsEnabled(true);
//	}
//}
#pragma endregion 내부 로직
