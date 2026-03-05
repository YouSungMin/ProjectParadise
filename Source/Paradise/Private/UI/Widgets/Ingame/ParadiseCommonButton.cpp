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
}
#pragma endregion 생명주기 및 초기화

#pragma region 상태 변화 구현 (이미지 교체)
void UParadiseCommonButton::NativeOnPressed()
{
	Super::NativeOnPressed();

	UpdateBackgroundImage(BgImage_Pressed ? BgImage_Pressed : BgImage_Normal);

	if (bEnablePressedTint && Img_Icon)
	{
		Img_Icon->SetColorAndOpacity(PressedTintColor);
	}
}

void UParadiseCommonButton::NativeOnReleased()
{
	Super::NativeOnReleased();

	UpdateBackgroundImage(BgImage_Normal);

	if (bEnablePressedTint && Img_Icon)
	{
		Img_Icon->SetColorAndOpacity(NormalTintColor);
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

void UParadiseCommonButton::SetButtonIcon(UTexture2D* InIcon)
{
	// 1. 내부 데이터 갱신
	BgImage_Normal = InIcon;
	BgImage_Pressed = InIcon;

	if (Img_Icon)
	{
		UpdateIconImage(InIcon);
	}
	else
	{
		UpdateBackgroundImage(InIcon);
	}
}

void UParadiseCommonButton::SetTagActiveState(bool bIsActive)
{
	// SetIsEnabled 대신 색상 틴트로 시각적 상태를 표현합니다.
	// bIsActive = true  → 현재 조작 중 → 밝게 (TagActiveColor)
	// bIsActive = false → 교체 대기   → 어둡게 (TagInactiveColor)
	SetColorAndOpacity(bIsActive ? TagActiveColor : TagInactiveColor);
}

#pragma endregion 외부 인터페이스 구현