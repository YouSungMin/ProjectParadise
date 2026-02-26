// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Squad/Inventory/ParadiseItemSlot.h"
#include "Components/Image.h"
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

	// 1. 공통 아이콘 설정
	if (Img_Icon)
	{
		if (InData.Icon)
		{
			Img_Icon->SetBrushFromTexture(InData.Icon);
			Img_Icon->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			Img_Icon->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	// 2. 공통 등급 테두리 갱신
	UpdateRankColor(InData.RankTag);

	// 3. 공통 장착 마크 갱신
	if (Img_EquippedMark)
	{
		Img_EquippedMark->SetVisibility(InData.bIsEquipped ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UParadiseItemSlot::OnButtonClicked()
{
	if (OnSlotClicked.IsBound())
	{
		OnSlotClicked.Broadcast(CachedData);
	}
}

void UParadiseItemSlot::UpdateRankColor(FGameplayTag RankTag)
{
	if (!Img_RankBorder) return;
	FLinearColor* FoundColor = RankColorMap.Find(RankTag);
	Img_RankBorder->SetColorAndOpacity(FoundColor ? *FoundColor : DefaultRankColor);
}
