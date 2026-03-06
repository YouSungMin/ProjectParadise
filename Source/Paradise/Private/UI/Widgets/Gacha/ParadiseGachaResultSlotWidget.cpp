// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Gacha/ParadiseGachaResultSlotWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

#pragma region 외부 인터페이스 구현
void UParadiseGachaResultSlotWidget::SetSlotData(
	const FGachaResult& InResult,
	const TMap<EItemRarity, UTexture2D*>& InRarityBorderMap)
{
	// 1. 아이템 아이콘 설정
	if (Img_ItemIcon && InResult.ItemIcon)
	{
		Img_ItemIcon->SetBrushFromTexture(InResult.ItemIcon);
	}

	// 2. 등급별 테두리 텍스처 교체
	if (Img_RarityBorder)
	{
		if (UTexture2D* const* BorderTex = InRarityBorderMap.Find(InResult.PulledRarity))
		{
			if (*BorderTex)
			{
				Img_RarityBorder->SetBrushFromTexture(*BorderTex);
			}
		}
	}

	// 3. 중복 여부 표시 ("중복" 텍스트)
	if (Text_Duplicate)
	{
		const ESlateVisibility DupVisibility =
			InResult.bIsDuplicate
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed;

		Text_Duplicate->SetVisibility(DupVisibility);
	}

	// 4. 환산 재화 수량
	if (Text_Value)
	{
		Text_Value->SetText(FText::AsNumber(InResult.ConvertedFragments));
	}

	// 5. 블루프린트 후처리 (등장 애니메이션 등)
	OnSlotDataSet(InResult);
}
#pragma endregion 외부 인터페이스 구현