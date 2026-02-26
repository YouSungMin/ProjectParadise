// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Squad/Inventory/Slots/ParadiseUnitSlot.h"

#pragma region 로직 구현
void UParadiseUnitSlot::UpdateSlot(const FSquadItemUIData& InData)
{
	// 유닛은 레벨과 수량 표시가 필요 없으므로 부모의 핵심 로직(아이콘, 등급 테두리)만 100% 재사용합니다.
	Super::UpdateSlot(InData);
}
#pragma endregion 로직 구현