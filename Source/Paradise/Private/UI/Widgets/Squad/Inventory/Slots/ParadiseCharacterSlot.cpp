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
		// 레벨이 0 이하이거나, 미보유 상태면 텍스트를 아예 꺼버립니다.
		// (도감 메인 위젯에서 데이터를 넘길 때 Level을 0으로 주면 자연스럽게 안 보이게 됨)
		if (InData.Level <= 0 || !InData.bIsOwned)
		{
			Text_Level->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			// 가장 안전한 포맷팅 방식 사용
			FString LevelString = FString::Printf(TEXT("Lv.%d"), InData.Level);
			Text_Level->SetText(FText::FromString(LevelString));
			Text_Level->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	}
}
#pragma endregion 로직 구현