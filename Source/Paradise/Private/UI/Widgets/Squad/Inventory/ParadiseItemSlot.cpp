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
	UpdateRankColor(InData.Rarity);

	// 3. 공통 장착 마크 갱신
	if (Img_EquippedMark)
	{
		Img_EquippedMark->SetVisibility(InData.bIsEquipped ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	// 보유 여부에 따른 상태 변경 (도감용)
	if (!InData.bIsOwned)
	{
		// 아이콘을 흑백/어둡게 (블러 느낌)
		if (Img_Icon) Img_Icon->SetColorAndOpacity(FLinearColor(0.15f, 0.15f, 0.15f, 0.9f));
		if (Img_RankBorder) Img_RankBorder->SetColorAndOpacity(FLinearColor(0.3f, 0.3f, 0.3f, 1.0f));

		// 터치(클릭) 원천 차단
		if (Btn_Select) Btn_Select->SetIsEnabled(false);
	}
	else
	{
		// 보유한 아이템이면 정상 색상 및 클릭 활성화
		if (Img_Icon) Img_Icon->SetColorAndOpacity(FLinearColor::White);
		if (Img_RankBorder) Img_RankBorder->SetColorAndOpacity(FLinearColor::White); // 틴트 초기화
		if (Btn_Select) Btn_Select->SetIsEnabled(true);
	}
}

void UParadiseItemSlot::OnButtonClicked()
{
	if (OnSlotClicked.IsBound())
	{
		OnSlotClicked.Broadcast(CachedData);
	}
}

void UParadiseItemSlot::UpdateRankColor(EItemRarity Rarity)
{
	if (!Img_RankBorder) return;

	// Map에서 Enum 값으로 색상을 찾습니다.
	FLinearColor* FoundColor = RankColorMap.Find(Rarity);
	Img_RankBorder->SetColorAndOpacity(FoundColor ? *FoundColor : DefaultRankColor);
}