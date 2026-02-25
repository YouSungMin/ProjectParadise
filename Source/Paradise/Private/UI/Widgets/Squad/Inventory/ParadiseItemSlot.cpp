// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Squad/Inventory/ParadiseItemSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UParadiseItemSlot::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Select)
	{
		Btn_Select->OnClicked.AddDynamic(this, &UParadiseItemSlot::OnButtonClicked);
	}
}

void UParadiseItemSlot::NativeDestruct()
{
	if (Btn_Select)
	{
		Btn_Select->OnClicked.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UParadiseItemSlot::UpdateSlot(const FSquadItemUIData& InData)
{
	CachedData = InData;
	CachedID = InData.ID;

	// 1. 아이콘 설정
	if (Img_Icon)
	{
		if (InData.Icon)
		{
			Img_Icon->SetBrushFromTexture(InData.Icon);
			Img_Icon->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			// 아이콘 없으면 투명 처리 혹은 기본 이미지
			Img_Icon->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	// 2. 레벨 텍스트
	if (Text_Level)
	{
		if (InData.Level > 0)
		{
			Text_Level->SetText(FText::Format(FText::FromString(TEXT("Lv.{0}")), InData.Level));
			Text_Level->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Text_Level->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	// 3. 수량 텍스트 설정 (추가됨)
	if (Text_Quantity)
	{
		if (InData.Quantity > 1)
		{
			Text_Quantity->SetText(FText::Format(FText::FromString(TEXT("x{0}")), InData.Quantity));
			Text_Quantity->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			Text_Quantity->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 4. 등급 테두리 색상
	UpdateRankColor(InData.RankTag);

	// 5. 장착 표시
	if (Img_EquippedMark)
	{
		Img_EquippedMark->SetVisibility(InData.bIsEquipped ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UParadiseItemSlot::OnButtonClicked()
{
	// 상위 위젯(InventoryPanel)에게 클릭 사실 전파
	OnSlotClicked.Broadcast(CachedData);
}

void UParadiseItemSlot::UpdateRankColor(FGameplayTag RankTag)
{
	if (!Img_RankBorder) return;

	// 하드코딩을 제거하고 맵(RankColorMap)을 조회하여 색상을 결정합니다.
	FLinearColor* FoundColor = RankColorMap.Find(RankTag);
	FLinearColor FinalColor = FoundColor ? *FoundColor : DefaultRankColor;

	Img_RankBorder->SetColorAndOpacity(FinalColor);
}
