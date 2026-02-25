// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Squad/ParadiseSquadSlot.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"

#pragma region 생명주기
void UParadiseSquadSlot::NativePreConstruct()
{
	Super::NativePreConstruct();

	// 에디터에서 설정한 Width/Height 값을 SizeBox에 즉시 반영
	if (RootSizeBox)
	{
		RootSizeBox->SetWidthOverride(SlotWidth);
		RootSizeBox->SetHeightOverride(SlotHeight);
	}
}

void UParadiseSquadSlot::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Select)
	{
		Btn_Select->OnClicked.AddDynamic(this, &UParadiseSquadSlot::OnButtonClicked);
	}

	// 초기엔 선택 안 된 상태
	if (Img_SelectionBorder)
	{
		Img_SelectionBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UParadiseSquadSlot::NativeDestruct()
{
	if (Btn_Select)
	{
		Btn_Select->OnClicked.RemoveAll(this);
	}
	Super::NativeDestruct();
}
#pragma endregion 생명주기

#pragma region 로직 구현
void UParadiseSquadSlot::InitSlot(int32 InSlotIndex)
{
	SlotIndex = InSlotIndex;

	if (Text_Level)
	{
		// 3번 인덱스 이상은 유닛 슬롯 -> 레벨 표기 안 함
		if (SlotIndex >= 3)
		{
			Text_Level->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			// 캐릭터 슬롯은 일단 보이도록 설정 (데이터가 없으면 나중에 꺼짐)
			Text_Level->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	}
}

void UParadiseSquadSlot::UpdateSlot(const FSquadItemUIData& InData)
{
	// ID가 없으면 비어있는 슬롯으로 간주
	bIsEmpty = InData.ID.IsNone();

	if (bIsEmpty)
	{
		// 1. 비어있음 (Empty)
		if (Img_Icon) Img_Icon->SetVisibility(ESlateVisibility::Collapsed);
		if (Img_EmptyPlaceholder) Img_EmptyPlaceholder->SetVisibility(ESlateVisibility::Visible);
		if (Text_Level) Text_Level->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		// 2. 채워짐 (Filled)
		if (Img_Icon)
		{
			Img_Icon->SetBrushFromTexture(InData.Icon);
			Img_Icon->SetVisibility(ESlateVisibility::Visible);
		}

		if (Img_EmptyPlaceholder) Img_EmptyPlaceholder->SetVisibility(ESlateVisibility::Collapsed);

		if (Text_Level)
		{
			// 여기서도 이중 체크 (안전장치)
			const bool bIsUnitSlot = (SlotIndex >= 3);
			if (bIsUnitSlot)
			{
				Text_Level->SetVisibility(ESlateVisibility::Collapsed);
			}
			else
			{
				Text_Level->SetText(FText::Format(FText::FromString(TEXT("Lv.{0}")), InData.Level));
				Text_Level->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
		}
	}
}

void UParadiseSquadSlot::SetSelected(bool bIsSelected)
{
	if (Img_SelectionBorder)
	{
		Img_SelectionBorder->SetVisibility(bIsSelected ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UParadiseSquadSlot::OnButtonClicked()
{
	if (SlotIndex != -1)
	{
		OnSlotClicked.Broadcast(SlotIndex);
	}
}
#pragma endregion 로직 구현