// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Squad/Inventory/Slots/ParadiseCharacterSlot.h"
#include "Components/TextBlock.h"

#pragma region 로직 구현
void UParadiseCharacterSlot::UpdateSlot(const FSquadItemUIData& InData)
{
	// 1. 부모 클래스의 공통 로직(아이콘, 테두리 등) 우선 실행
	Super::UpdateSlot(InData);

	// 2. 캐릭터 고유 로직: 레벨 텍스트 처리 (단일 책임)
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
}
#pragma endregion 로직 구현