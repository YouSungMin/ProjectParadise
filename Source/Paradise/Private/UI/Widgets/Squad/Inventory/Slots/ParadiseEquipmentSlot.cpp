// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Squad/Inventory/Slots/ParadiseEquipmentSlot.h"
#include "Components/TextBlock.h"

#pragma region 로직 구현
void UParadiseEquipmentSlot::UpdateSlot(const FSquadItemUIData& InData)
{
	// 1. 부모 클래스의 공통 로직 실행
	Super::UpdateSlot(InData);

	// 2. 장비 고유 로직: 강화 수치(레벨) 처리
	if (Text_Level)
	{
		if (InData.Level > 0)
		{
			// 장비 특성에 맞춰 +기호 표기
			Text_Level->SetText(FText::Format(FText::FromString(TEXT("+{0}")), InData.Level));
			Text_Level->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Text_Level->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
#pragma endregion 로직 구현