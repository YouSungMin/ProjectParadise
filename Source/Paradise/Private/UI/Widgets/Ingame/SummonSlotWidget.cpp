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

	if (Text_Shortcut)
	{
		Text_Shortcut->SetText(ShortcutKeyText);
		Text_Shortcut->SetVisibility(ESlateVisibility::Collapsed);
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
	if (Img_Outline) Img_Outline->SetVisibility(ESlateVisibility::Hidden);

	// 아직 비어있는 칸이므로 클릭을 막아 연타 버그를 예방합니다.
	if (Btn_SummonAction) Btn_SummonAction->SetIsEnabled(false);

	// 3. 타이머 가동 (DelayTime 초 뒤에 OnRevealTimerFinished 호출)
	GetWorld()->GetTimerManager().SetTimer(RevealTimerHandle, this, &USummonSlotWidget::OnRevealTimerFinished, DelayTime, false);
}

void USummonSlotWidget::PlayIntroAnimation()
{
	if (Anim_Intro)
	{
		// 자동 소환 시 애니메이션 겹침 방지 (끊기지 않고 자연스럽게 가속)
		if (IsAnimationPlaying(Anim_Intro))
		{
			float CurrentTime = GetAnimationCurrentTime(Anim_Intro);
			PlayAnimation(Anim_Intro, CurrentTime, 1, EUMGSequencePlayMode::Forward, 1.5f);
		}
		else
		{
			PlayAnimation(Anim_Intro, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);
		}
	}
}

void USummonSlotWidget::PlayShiftAnimation()
{
	if (Anim_Shift)
	{
		// 0.5초마다 RequestPurchase가 호출될 때 애니메이션이 끝나지 않았다면 이어서 재생해야 합니다.
		if (IsAnimationPlaying(Anim_Shift))
		{
			float CurrentTime = GetAnimationCurrentTime(Anim_Shift);
			PlayAnimation(Anim_Shift, CurrentTime, 1, EUMGSequencePlayMode::Forward, 2.0f);
		}
		else
		{
			PlayAnimation(Anim_Shift, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);
		}
	}
}

void USummonSlotWidget::SetShortcutTextVisibility(bool bShow)
{
	if (Text_Shortcut)
	{
		// 키보드 모드면 클릭을 방해하지 않는(HitTestInvisible) 상태로 보여주고, 터치 모드면 숨깁니다.
		Text_Shortcut->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

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
	if (Img_Outline) Img_Outline->SetVisibility(ESlateVisibility::HitTestInvisible);
}
#pragma endregion 내부 로직
