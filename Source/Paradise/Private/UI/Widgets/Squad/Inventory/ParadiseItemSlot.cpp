// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Squad/Inventory/ParadiseItemSlot.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Engine/Texture2D.h"

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
	// MVC: Controller로부터 전달받은 Model(데이터)을 캐싱
	CachedData = InData;

	// SRP: 각 UI 요소별 업데이트 책임을 개별 함수로 분리하여 최적화 및 가독성 확보
	UpdateMainIconUI();
	UpdateRankUI();
	UpdateEquipStateUI();
	UpdateOwnershipStateUI();
}
#pragma endregion 생명주기 및 가상 함수

#pragma region 내부 로직 (단일 책임 원칙 SRP)
void UParadiseItemSlot::OnButtonClicked()
{
	if (OnSlotClicked.IsBound())
	{
		OnSlotClicked.Broadcast(CachedData);
	}
}

void UParadiseItemSlot::UpdateMainIconUI()
{
	if (!Img_Icon) return;

	if (CachedData.Icon)
	{
		Img_Icon->SetBrushFromTexture(CachedData.Icon);
		Img_Icon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		Img_Icon->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UParadiseItemSlot::UpdateRankUI()
{
	// 1. 테두리 색상 (Tint) 업데이트
	if (Img_RankBorder)
	{
		const FLinearColor* FoundColor = RankColorMap.Find(CachedData.Rarity);
		Img_RankBorder->SetColorAndOpacity(FoundColor ? *FoundColor : DefaultRankColor);
	}

	// 2. 등급 글자 엠블럼 (Texture2D) 업데이트
	if (Img_RankIcon)
	{
		TObjectPtr<UTexture2D>* FoundIcon = RankIconMap.Find(CachedData.Rarity);
		UTexture2D* TargetIcon = FoundIcon ? *FoundIcon : DefaultRankIcon;

		if (TargetIcon)
		{
			Img_RankIcon->SetBrushFromTexture(TargetIcon);
			Img_RankIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			Img_RankIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UParadiseItemSlot::UpdateEquipStateUI()
{
	if (!Img_EquippedMark) return;

	// 장착 중이면 노출, 아니면 레이아웃에서 제거
	Img_EquippedMark->SetVisibility(CachedData.bIsEquipped ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

void UParadiseItemSlot::UpdateOwnershipStateUI()
{
	if (!CachedData.bIsOwned)
	{
		// 미보유 시 흑백/어둡게 (블러 느낌 적용)
		const FLinearColor DisabledColor(0.15f, 0.15f, 0.15f, 0.9f);
		const FLinearColor DisabledRankColor(0.3f, 0.3f, 0.3f, 1.0f);

		if (Img_Icon) Img_Icon->SetColorAndOpacity(DisabledColor);
		if (Img_RankBorder) Img_RankBorder->SetColorAndOpacity(DisabledRankColor);
		if (Img_RankIcon) Img_RankIcon->SetColorAndOpacity(DisabledRankColor);

		// 터치(클릭) 상호작용 원천 차단
		if (Btn_Select) Btn_Select->SetIsEnabled(false);
	}
	else
	{
		// 보유 시 정상 색상(원래 색)으로 복원
		if (Img_Icon) Img_Icon->SetColorAndOpacity(FLinearColor::White);
		//if (Img_RankBorder) Img_RankBorder->SetColorAndOpacity(FLinearColor::White);
		if (Img_RankIcon) Img_RankIcon->SetColorAndOpacity(FLinearColor::White);

		// 터치(클릭) 활성화
		if (Btn_Select) Btn_Select->SetIsEnabled(true);
	}
}
#pragma endregion 내부 로직 (단일 책임 원칙 SRP)