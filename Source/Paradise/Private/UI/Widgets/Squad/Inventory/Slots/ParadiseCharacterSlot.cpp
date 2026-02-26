// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Squad/Inventory/Slots/ParadiseCharacterSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UParadiseCharacterSlot::NativeConstruct()
{
	Super::NativeConstruct();
	if (Btn_Select) Btn_Select->OnClicked.AddDynamic(this, &UParadiseCharacterSlot::OnButtonClicked);
}

void UParadiseCharacterSlot::NativeDestruct()
{
	if (Btn_Select) Btn_Select->OnClicked.RemoveAll(this);
	Super::NativeDestruct();
}

void UParadiseCharacterSlot::UpdateSlot(const FSquadItemUIData& InData)
{
	CachedData = InData;

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

	UpdateRankColor(InData.RankTag);

	if (Img_EquippedMark)
	{
		Img_EquippedMark->SetVisibility(InData.bIsEquipped ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UParadiseCharacterSlot::OnButtonClicked()
{
	OnSlotClicked.Broadcast(CachedData);
}

void UParadiseCharacterSlot::UpdateRankColor(FGameplayTag RankTag)
{
	if (!Img_RankBorder) return;
	FLinearColor* FoundColor = RankColorMap.Find(RankTag);
	Img_RankBorder->SetColorAndOpacity(FoundColor ? *FoundColor : DefaultRankColor);
}
