// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Ingame/ParadiseCommonButton.h"

#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

#pragma region 내부 헬퍼 로직 구현
void UParadiseCommonButton::UpdateBackgroundImage(UTexture2D* TargetTex)
{
	if (Img_Bg)
	{
		if (TargetTex)
		{
			Img_Bg->SetBrushFromTexture(TargetTex, false);
			Img_Bg->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			Img_Bg->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UParadiseCommonButton::UpdateIconImage(UTexture2D* TargetTex)
{
	if (!Img_Icon) return;

	if (TargetTex)
	{
		Img_Icon->SetBrushFromTexture(TargetTex, false);
		Img_Icon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		Img_Icon->SetVisibility(ESlateVisibility::Collapsed);
	}
}
#pragma endregion 내부 헬퍼 로직 구현

#pragma region 생명주기 및 초기화
void UParadiseCommonButton::NativePreConstruct()
{
	Super::NativePreConstruct();

	// 에디터 프리뷰용 텍스트만 (바인딩 전이라 이미지 세팅 불가)
	SetButtonText(ButtonLabelText);

	if (Text_Shortcut)
	{
		Text_Shortcut->SetText(ShortcutKeyText);
	}
}

void UParadiseCommonButton::NativeConstruct()
{
	Super::NativeConstruct();

	// 바인딩 완료 시점 → 여기서 비로소 Img_Bg, Img_Icon에 접근 가능
	SetButtonText(ButtonLabelText);

	// Img_Icon 있는 버튼(태그 버튼)은 아이콘 없으므로 숨김
	// Img_Icon 없는 버튼(오토/설정)은 BgImage_Normal로 초기화
	if (Img_Icon)
	{
		UpdateIconImage(nullptr);
	}
	else
	{
		UpdateBackgroundImage(BgImage_Normal);
	}

	if (Img_GlowRing)
	{
		Img_GlowRing->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Text_Shortcut)
	{
		Text_Shortcut->SetText(ShortcutKeyText);
		Text_Shortcut->SetVisibility(ESlateVisibility::Hidden);
	}
}
#pragma endregion 생명주기 및 초기화

#pragma region 상태 변화 구현 (이미지 교체)
void UParadiseCommonButton::NativeOnPressed()
{
	Super::NativeOnPressed();

	UpdateBackgroundImage(BgImage_Pressed ? BgImage_Pressed : BgImage_Normal);

	if (bEnablePressedTint)
	{
		if (Img_Icon) Img_Icon->SetColorAndOpacity(PressedTintColor);
		if (Img_Bg) Img_Bg->SetColorAndOpacity(PressedTintColor);
	}
}

void UParadiseCommonButton::NativeOnReleased()
{
	Super::NativeOnReleased();

	UpdateBackgroundImage(BgImage_Normal);

	if (bEnablePressedTint)
	{
		if (Img_Icon) Img_Icon->SetColorAndOpacity(NormalTintColor);
		if (Img_Bg) Img_Bg->SetColorAndOpacity(NormalTintColor);
	}
}

void UParadiseCommonButton::NativeOnHovered()
{
	Super::NativeOnHovered();
}

void UParadiseCommonButton::NativeOnUnhovered()
{
	Super::NativeOnUnhovered();

	UpdateBackgroundImage(BgImage_Normal);

	// 마우스가 밖으로 나가도 원래 색상으로 복구 (안전장치)
	if (bEnablePressedTint)
	{
		if (Img_Icon) Img_Icon->SetColorAndOpacity(NormalTintColor);
		if (Img_Bg) Img_Bg->SetColorAndOpacity(NormalTintColor);
	}
}
#pragma endregion 상태 변화 구현 (이미지 교체)

#pragma region 외부 인터페이스 구현
void UParadiseCommonButton::SetButtonText(FText InText)
{
	ButtonLabelText = InText;
	if (Text_Label)
	{
		Text_Label->SetText(ButtonLabelText);
	}
}

void UParadiseCommonButton::SetButtonIcon(UTexture2D* InNormalIcon, UTexture2D* InPressedIcon)
{
	// 1. 내부 데이터 갱신 (SRP: 상태 저장은 버튼 스스로가 관리)
	// 눌림 이미지가 없으면(nullptr) 기본 이미지로 자동 대체하여 에러 및 빈 화면 방지
	BgImage_Normal = InNormalIcon;
	BgImage_Pressed = InPressedIcon ? InPressedIcon : InNormalIcon;

	// 2. 즉시 UI 갱신
	if (Img_Icon)
	{
		UpdateIconImage(InNormalIcon);
	}
	else
	{
		UpdateBackgroundImage(InNormalIcon);
	}
}

void UParadiseCommonButton::SetTagActiveState(bool bIsActive)
{
	// SetIsEnabled 대신 색상 틴트로 시각적 상태를 표현합니다.
	// bIsActive = true  → 현재 조작 중 → 밝게 (TagActiveColor)
	// bIsActive = false → 교체 대기   → 어둡게 (TagInactiveColor)
	SetColorAndOpacity(bIsActive ? TagActiveColor : TagInactiveColor);

	SetGlowRingActive(bIsActive);
}

void UParadiseCommonButton::SetGlowRingActive(bool bIsActive)
{
	// 오토 버튼이나 태그 버튼에서 개별적으로 링을 제어할 때 호출됨
	if (Img_GlowRing)
	{
		Img_GlowRing->SetVisibility(bIsActive ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UParadiseCommonButton::SetShortcutTextVisibility(bool bShow)
{
	if (Text_Shortcut)
	{
		// 키보드 모드(true)면 클릭을 방해하지 않는 HitTestInvisible로 켜고, 터치 모드(false)면 끕니다.
		Text_Shortcut->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}
#pragma endregion 외부 인터페이스 구현