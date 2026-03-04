// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Squad/Inventory/Slots/ParadiseMiscSlot.h"
#include "Components/TextBlock.h"

#pragma region 로직 가상 함수
void UParadiseMiscSlot::UpdateSlot(const FSquadItemUIData& InData)
{
	// 1. 부모 호출 (아이콘 렌더링, 미보유 시 흑백 처리 및 클릭 잠금은 부모가 알아서 해줌)
	Super::UpdateSlot(InData);

	// 2. 자식 고유의 '수량(Quantity)' 텍스트 처리
	if (Text_Quantity)
	{
		// 도감 화면이거나 수량이 0 이하이면 텍스트를 완전히 끕니다.
		if (InData.Quantity <= 0 || !InData.bIsOwned)
		{
			Text_Quantity->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			// 수량 표기 (가장 안전한 C++ 포맷팅 방식)
			FString QuantityString = FString::Printf(TEXT("x%d"), InData.Quantity);
			Text_Quantity->SetText(FText::FromString(QuantityString));

			Text_Quantity->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	}
}
#pragma endregion 로직 가상 함수