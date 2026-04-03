// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widgets/Squad/Inventory/ParadiseItemSlot.h"
#include "ParadiseMiscSlot.generated.h"

#pragma region 전방 선언
class UTextBlock;
#pragma endregion 전방 선언

/**
 * @class UParadiseMiscSlot
 * @brief 재료, 소모품, 재화 등을 표시하는 기타 아이템 전용 슬롯 위젯
 * @details 레벨이 아닌 보유 '수량(Quantity)'을 표시하는 책임을 가집니다.
 */
UCLASS()
class PARADISE_API UParadiseMiscSlot : public UParadiseItemSlot
{
	GENERATED_BODY()
	
#pragma region 로직 가상 함수
public:
	/**
	 * @brief 소모품 데이터를 받아 UI를 갱신합니다.
	 * @param InData 표시할 데이터 구조체 (Quantity 활용)
	 */
	virtual void UpdateSlot(const FSquadItemUIData& InData) override;
#pragma endregion 로직 가상 함수

#pragma region 자식 UI 바인딩
protected:
	/** @brief 아이템 보유 수량 표시 텍스트 (예: x99) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Quantity = nullptr;
#pragma endregion 자식 UI 바인딩
};
