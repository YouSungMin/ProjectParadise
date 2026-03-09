// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Gacha/ParadiseGachaResultWidget.h"
#include "UI/Widgets/Gacha/ParadiseGachaResultSlotWidget.h"

#include "Framework/Lobby/LobbyPlayerController.h"

#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

#include "Engine/Texture2D.h"

#pragma region 생명주기
void UParadiseGachaResultWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Confirm)
	{
		Btn_Confirm->OnClicked.AddDynamic(this, &UParadiseGachaResultWidget::OnConfirmClicked);
	}

	CachedPlayerController = GetOwningPlayer<ALobbyPlayerController>();
}

void UParadiseGachaResultWidget::NativeDestruct()
{
	if (Btn_Confirm)
	{
		Btn_Confirm->OnClicked.RemoveAll(this);
	}

	CachedPlayerController = nullptr;

	Super::NativeDestruct();
}
#pragma endregion 생명주기

#pragma region 외부 인터페이스
void UParadiseGachaResultWidget::ShowResults(const TArray<FGachaResult>& Results)
{
	if (Results.IsEmpty()) return;

	// 1. 단일 소환(1연차)일 경우 -> 전신 일러스트 및 상세 스탯 뷰 표출
	if (Results.Num() == 1)
	{
		// 1연차 — 세로 카드 1장
		if (Switcher_ResultLayout)
		{
			Switcher_ResultLayout->SetActiveWidgetIndex(INDEX_SINGLE);
		}

		SetupSingleResult(Results[0]);
		OnDisplaySingleResult(Results[0]);
	}
	// 2. 다중 소환(10연차)일 경우 -> 니케 스타일 10칸 슬롯 뷰 표출
	else
	{
		// 10연차 — 5열 2행 그리드
		if (Switcher_ResultLayout)
		{
			Switcher_ResultLayout->SetActiveWidgetIndex(INDEX_MULTI);
		}
		SetupMultiResult(Results);
		OnDisplayMultiResult(Results);
	}
}
#pragma endregion 외부 인터페이스

#pragma region 내부 로직
void UParadiseGachaResultWidget::SetupSingleResult(const FGachaResult& Result)
{
	// 1. 아이템 아이콘
	if (Img_SingleItemIcon && Result.ItemCardIllust)
	{
		Img_SingleItemIcon->SetBrushFromTexture(Result.ItemCardIllust);
	}

	// 2. 등급 테두리 텍스처 교체
	if (Img_SingleRarityBorder)
	{
		if (TObjectPtr<UTexture2D>* BorderTex = RarityBorderTextureMap.Find(Result.PulledRarity))
		{
			if (*BorderTex)
			{
				Img_SingleRarityBorder->SetBrushFromTexture(*BorderTex);
			}
		}
	}

	// 3. 아이템 이름
	if (Text_SingleItemName)
	{
		Text_SingleItemName->SetText(FText::FromName(Result.ItemName));
	}
}

void UParadiseGachaResultWidget::SetupMultiResult(const TArray<FGachaResult>& Results)
{
	if (!Grid_MultiResult || !SlotWidgetClass) return;

	// 기존 슬롯 전부 제거 후 재생성 (재사용 방지)
	Grid_MultiResult->ClearChildren();

	// 등급 테두리 맵을 raw 포인터 맵으로 변환 (SlotWidget 인터페이스 호환)
	TMap<EItemRarity, UTexture2D*> RawBorderMap;
	for (const TPair<EItemRarity, TObjectPtr<UTexture2D>>& Pair : RarityBorderTextureMap)
	{
		RawBorderMap.Add(Pair.Key, Pair.Value.Get());
	}

	for (int32 i = 0; i < Results.Num(); ++i)
	{
		UParadiseGachaResultSlotWidget* SlotWidget =
			CreateWidget<UParadiseGachaResultSlotWidget>(this, SlotWidgetClass);

		if (!SlotWidget) continue;

		SlotWidget->SetSlotData(Results[i], RawBorderMap);

		// 5열 기준 행/열 계산
		const int32 Column = i % GridColumnCount;
		const int32 Row = i / GridColumnCount;

		UUniformGridSlot* GridSlot = Grid_MultiResult->AddChildToUniformGrid(SlotWidget, Row, Column);
		if (GridSlot)
		{
			GridSlot->SetHorizontalAlignment(HAlign_Fill);
			GridSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}
}

void UParadiseGachaResultWidget::OnConfirmClicked()
{
	SetVisibility(ESlateVisibility::Collapsed);

	if (CachedPlayerController.IsValid())
	{
		CachedPlayerController->ReturnFromGachaToSummon();
	}
}
#pragma endregion 내부 로직