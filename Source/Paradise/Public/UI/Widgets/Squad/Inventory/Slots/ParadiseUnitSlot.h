// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widgets/Squad/Inventory/ParadiseItemSlot.h"
#include "ParadiseUnitSlot.generated.h"

/**
 * @class UParadiseUnitSlot
 * @brief 유닛/퍼밀리어 전용 인벤토리 슬롯 위젯
 * @details 레벨 및 수량 텍스트 위젯을 전혀 바인딩하지 않아 UI 메모리 최적화를 극대화합니다.
 */
UCLASS()
class PARADISE_API UParadiseUnitSlot : public UParadiseItemSlot
{
	GENERATED_BODY()
	
#pragma region 로직 가상 함수
public:
	/**
	 * @brief 유닛 데이터를 받아 UI를 갱신합니다.
	 * @details 추가 텍스트 렌더링 로직 없이 부모의 공통 로직만 수행합니다.
	 * @param InData 표시할 유닛 데이터 구조체
	 */
	virtual void UpdateSlot(const FSquadItemUIData& InData) override;
#pragma endregion 로직 가상 함수
};
