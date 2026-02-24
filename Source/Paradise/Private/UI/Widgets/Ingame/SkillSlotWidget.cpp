// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Ingame/SkillSlotWidget.h"

#include "CommonButtonBase.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "TimerManager.h"

USkillSlotWidget::USkillSlotWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#pragma region 생명주기
void USkillSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 입력 처리는 버튼에게 위임하고, 위젯은 그 결과만 받아 처리합니다.
	if (Btn_SkillAction)
	{
		Btn_SkillAction->OnClicked().AddUObject(this, &USkillSlotWidget::OnSkillButtonClicked);
	}

	// 초기 상태 설정
	ClearCooldownVisual();
}

void USkillSlotWidget::NativeDestruct()
{
	// 타이머 정리 (메모리 누수 방지)
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CooldownTimerHandle);
	}

	Super::NativeDestruct();
}
#pragma endregion 생명주기

#pragma region 외부 인터페이스 구현
void USkillSlotWidget::UpdateSlotInfo(UTexture2D* InIconTexture, float InMaxCooldownTime)
{
	if (InIconTexture && Img_SkillIcon)
	{
		Img_SkillIcon->SetBrushFromTexture(InIconTexture);
	}

	MaxCooldown = InMaxCooldownTime;

	// 데이터가 변경되면 쿨타임 표시도 초기화
	ClearCooldownVisual();
}

void USkillSlotWidget::RefreshCooldown(float CurrentTime, float MaxTime)
{
	CurrentCooldown = CurrentTime;
	MaxCooldown = MaxTime;

	if (CurrentCooldown > 0.0f)
	{
		// 쿨타임이 남아있다면 버튼을 비활성화하여 입력을 차단합니다
		if (Btn_SkillAction)
		{
			Btn_SkillAction->SetIsEnabled(false);
		}

		if (PB_Cooldown)
		{
			PB_Cooldown->SetVisibility(ESlateVisibility::HitTestInvisible);
		}

		if (Text_CooldownTime)
		{
			Text_CooldownTime->SetVisibility(ESlateVisibility::HitTestInvisible);
		}

		// 타이머가 돌고 있지 않다면 시작합니다.
		if (GetWorld() && !GetWorld()->GetTimerManager().IsTimerActive(CooldownTimerHandle))
		{
			GetWorld()->GetTimerManager().SetTimer(
				CooldownTimerHandle,
				this,
				&USkillSlotWidget::UpdateCooldownVisual, 
				UpdateInterval, 
				true);
		}
	}
	else
	{
		ClearCooldownVisual();
	}
}
#pragma endregion 외부 인터페이스 구현

#pragma region 내부 로직 구현
void USkillSlotWidget::OnSkillButtonClicked()
{
	// 쿨타임 중이 아닐 때만 로직 수행 (이중 검증)
	if (CurrentCooldown <= 0.0f)
	{
		if (OnSkillActionRequested.IsBound())
		{
			OnSkillActionRequested.Broadcast();
		}
	}
}

void USkillSlotWidget::UpdateCooldownVisual()
{
	CurrentCooldown -= UpdateInterval;

	if (CurrentCooldown <= 0.0f)
	{
		ClearCooldownVisual();
		return;
	}

	// 퍼센트 계산 (0.0 ~ 1.0)
	const float Percent = FMath::Clamp(CurrentCooldown / MaxCooldown, 0.0f, 1.0f);

	if (PB_Cooldown)
	{
		PB_Cooldown->SetPercent(Percent);
	}

	if (Text_CooldownTime)
	{
		// 소수점 올림 처리하여 정수로 표시
		Text_CooldownTime->SetText(FText::AsNumber(FMath::CeilToInt(CurrentCooldown)));
	}
}

void USkillSlotWidget::ClearCooldownVisual()
{
	CurrentCooldown = 0.0f;

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CooldownTimerHandle);
	}

	if (PB_Cooldown)
	{
		PB_Cooldown->SetPercent(0.0f);
		PB_Cooldown->SetVisibility(ESlateVisibility::Collapsed); // 드로우 콜 절약
	}

	if (Text_CooldownTime)
	{
		Text_CooldownTime->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 쿨타임 종료 시 버튼 재활성화
	if (Btn_SkillAction)
	{
		Btn_SkillAction->SetIsEnabled(true);
	}
}
#pragma endregion 내부 로직 구현