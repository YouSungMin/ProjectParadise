// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Ingame/SkillSlotWidget.h"

#include "UI/Widgets/Ingame/ParadiseCommonButton.h"
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
		//Btn_SkillAction->OnClicked().AddUObject(this, &USkillSlotWidget::OnSkillButtonClicked);

		Btn_SkillAction->OnPressed().AddUObject(this, &USkillSlotWidget::OnSkillButtonPressed);
		Btn_SkillAction->OnReleased().AddUObject(this, &USkillSlotWidget::OnSkillButtonReleased);
	}

	// 기본 아이콘이 할당되어 있으면 즉시 세팅
	if (Tex_DefaultSkillIcon && Img_SkillIcon)
	{
		if (UMaterialInstanceDynamic* DynamicMat = Img_SkillIcon->GetDynamicMaterial())
		{
			DynamicMat->SetTextureParameterValue(FName("SpriteTexture"), Tex_DefaultSkillIcon);
		}
	}

	if (Img_Shortcut)
	{
		if (!ShortcutKeyImage.IsNull())
		{
			Img_Shortcut->SetBrushFromTexture(ShortcutKeyImage.LoadSynchronous());
		}
		Img_Shortcut->SetVisibility(ESlateVisibility::Hidden);
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
	if (Btn_SkillAction)
	{
		Btn_SkillAction->OnPressed().RemoveAll(this);
		Btn_SkillAction->OnReleased().RemoveAll(this);
	}

	Super::NativeDestruct();
}
#pragma endregion 생명주기

#pragma region 외부 인터페이스 구현
void USkillSlotWidget::UpdateSlotInfo(UTexture2D* InIconTexture, float InMaxCooldownTime)
{
	if (InIconTexture && Img_SkillIcon)
	{
		if (UMaterialInstanceDynamic* DynamicMat = Img_SkillIcon->GetDynamicMaterial())
		{
			DynamicMat->SetTextureParameterValue(FName("SpriteTexture"), InIconTexture);
		}
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
void USkillSlotWidget::SetManaAffordable(bool bAffordable)
{
	bIsManaAffordable = bAffordable;

	// 1. 쿨타임이 돌고 있지 않을 때만 버튼 활성화 여부 결정 (쿨타임 중이면 어차피 꺼져있어야 함)
	if (CurrentCooldown <= 0.0f && Btn_SkillAction)
	{
		Btn_SkillAction->SetIsEnabled(bIsManaAffordable);
	}

	// 2. 마나가 부족하면 아이콘을 어둡게 처리하여 직관적인 UX 제공
	if (Img_SkillIcon)
	{
		Img_SkillIcon->SetColorAndOpacity(bIsManaAffordable ? NormalTintColor : FLinearColor(0.2f, 0.2f, 0.2f, 1.0f));
	}
}
#pragma endregion 외부 인터페이스 구현

#pragma region 내부 로직 구현
void USkillSlotWidget::OnSkillButtonPressed()
{
	if (Img_SkillIcon)
	{
		Img_SkillIcon->SetColorAndOpacity(PressedTintColor);
	}

	if (CurrentCooldown <= 0.0f)
	{
		if (OnSkillPressed.IsBound())
		{
			OnSkillPressed.Broadcast();
		}
	}
}

void USkillSlotWidget::OnSkillButtonReleased()
{
	if (Img_SkillIcon)
	{
		Img_SkillIcon->SetColorAndOpacity(NormalTintColor);
	}

	if (CurrentCooldown <= 0.0f)
	{
		if (OnSkillReleased.IsBound())
		{
			OnSkillReleased.Broadcast();
		}
	}
}

void USkillSlotWidget::SetShortcutTextVisibility(bool bShow)
{
	if (Img_Shortcut)
	{
		Img_Shortcut->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
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
		Btn_SkillAction->SetIsEnabled(bIsManaAffordable);
	}
}
#pragma endregion 내부 로직 구현