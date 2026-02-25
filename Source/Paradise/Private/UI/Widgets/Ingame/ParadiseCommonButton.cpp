// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Ingame/ParadiseCommonButton.h"

#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

#pragma region 생명주기
void UParadiseCommonButton::NativePreConstruct()
{
	Super::NativePreConstruct();

	// 에디터 미리보기(Preview) 및 초기화 시 텍스트 및 아이콘 갱신 (Setter 재사용)
	SetButtonText(ButtonLabelText);
	SetButtonIcon(ButtonIcon);
}
#pragma endregion 생명주기

#pragma region 외부 인터페이스 구현
void UParadiseCommonButton::SetButtonText(FText InText)
{
	// 내부 데이터 갱신
	ButtonLabelText = InText;

	// 바인딩된 텍스트 위젯이 있다면 실제 UI 업데이트 (캡슐화)
	if (Text_Label)
	{
		Text_Label->SetText(ButtonLabelText);
	}
}

void UParadiseCommonButton::SetButtonIcon(UTexture2D* InIcon)
{
	// 내부 데이터 갱신
	ButtonIcon = InIcon;

	// 바인딩된 이미지 위젯이 있다면 실제 UI 업데이트
	if (Img_Icon)
	{
		if (ButtonIcon)
		{
			Img_Icon->SetBrushFromTexture(ButtonIcon, false);
			// 클릭 판정은 부모인 버튼 베이스가 처리해야 하므로, 이미지는 마우스 충돌을 무시하도록 설정
			Img_Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			// 디테일 패널에서 아이콘을 비워두었다면 이미지를 숨겨서 '텍스트 전용 버튼'으로 호환되게 처리
			Img_Icon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
#pragma endregion 외부 인터페이스 구현
