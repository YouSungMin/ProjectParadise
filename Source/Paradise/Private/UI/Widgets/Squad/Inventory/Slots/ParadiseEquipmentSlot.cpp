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
		// 도감에서는 레벨을 0으로 넘겨줄 것이므로 자동으로 숨겨집니다.
		if (InData.Level <= 0 || !InData.bIsOwned)
		{
			Text_Level->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			// 장비는 "+1" 스타일로 표기하는 것이 일반적입니다.
			FString EnhanceString = FString::Printf(TEXT("+%d"), InData.Level);
			Text_Level->SetText(FText::FromString(EnhanceString));
			Text_Level->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	}
}
#pragma endregion 로직 구현