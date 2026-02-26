// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widgets/Squad/Inventory/ParadiseItemSlot.h"
#include "ParadiseEquipmentSlot.generated.h"

#pragma region 전방 선언
class UTextBlock;
#pragma endregion 전방 선언

/**
 * @class UParadiseEquipmentSlot
 * @brief 무기 및 방어구 전용 인벤토리 슬롯 위젯
 * @details 강화 수치(레벨)와 중복 보유 수량(Quantity)을 모두 표시합니다.
 */
UCLASS()
class PARADISE_API UParadiseEquipmentSlot : public UParadiseItemSlot
{
	GENERATED_BODY()

#pragma region 로직 가상 함수
public:
	/**
	 * @brief 장비 데이터를 받아 UI를 갱신합니다.
	 * @param InData 표시할 장비 데이터 구조체
	 */
	virtual void UpdateSlot(const FSquadItemUIData& InData) override;
#pragma endregion 로직 가상 함수

#pragma region 자식 UI 바인딩
protected:
	/** @brief 장비 강화 수치(+1, +2 등) 텍스트 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Level = nullptr;
#pragma endregion 자식 UI 바인딩
};