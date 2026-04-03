// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Chapter/ParadiseChapterSlotWidget.h"
#include "Components/Button.h"
#include "Materials/MaterialInterface.h"

#pragma region 생명주기
void UParadiseChapterSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 1. 이벤트 바인딩
	if (Btn_Chapter)
	{
		Btn_Chapter->OnClicked.AddDynamic(this, &UParadiseChapterSlotWidget::OnChapterClicked);
	}

}

void UParadiseChapterSlotWidget::NativeDestruct()
{
	// 이벤트 안전 해제 (메모리 릭 및 댕글링 포인터 방지)
	if (Btn_Chapter)
	{
		Btn_Chapter->OnClicked.RemoveAll(this);
	}

	Super::NativeDestruct();
}
#pragma endregion 생명주기

#pragma region 외부 인터페이스 구현
void UParadiseChapterSlotWidget::InitSlot(int32 InChapterID, bool bIsUnlocked, UTexture2D* InMapTexture, UMaterialInterface* InButtonMaterial)
{
	CurrentChapterID = InChapterID;
	CachedMapTexture = InMapTexture;

	if (Btn_Chapter)
	{
		// 해금 여부에 따른 비활성화 처리
		Btn_Chapter->SetIsEnabled(bIsUnlocked);

		// [Data-Driven] 전달받은 머티리얼로 버튼 이미지를 교체합니다.
		if (InButtonMaterial)
		{
			FButtonStyle NewStyle = Btn_Chapter->GetStyle();

			// Normal, Hovered, Pressed 모든 상태에 머티리얼 할당
			NewStyle.Normal.SetResourceObject(InButtonMaterial);
			NewStyle.Hovered.SetResourceObject(InButtonMaterial);
			NewStyle.Pressed.SetResourceObject(InButtonMaterial);

			// UI 머티리얼이 슬롯 영역을 꽉 채우도록 DrawAs를 Image로 설정
			NewStyle.Normal.DrawAs = ESlateBrushDrawType::Image;
			NewStyle.Hovered.DrawAs = ESlateBrushDrawType::Image;
			NewStyle.Pressed.DrawAs = ESlateBrushDrawType::Image;

			Btn_Chapter->SetStyle(NewStyle);
		}
	}
}
#pragma endregion 외부 인터페이스 구현

#pragma region 내부 로직 구현
void UParadiseChapterSlotWidget::OnChapterClicked()
{
	if (OnChapterSelected.IsBound())
	{
		OnChapterSelected.Broadcast(CurrentChapterID, CachedMapTexture);
	}
}
#pragma endregion 내부 로직 구현