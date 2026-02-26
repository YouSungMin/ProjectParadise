// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widgets/Squad/Inventory/ParadiseItemSlot.h"
#include "ParadiseCharacterSlot.generated.h"

#pragma region 전방 선언
class UTextBlock;
#pragma endregion 전방 선언

/**
 * @class UParadiseCharacterSlot
 * @brief 캐릭터 전용 인벤토리 슬롯 위젯
 * @details 부모의 공통 UI에 더해 '레벨(Level)' 정보만 추가로 처리하며, 수량은 렌더링하지 않습니다.
 */
UCLASS()
class PARADISE_API UParadiseCharacterSlot : public UParadiseItemSlot
{
	GENERATED_BODY()

#pragma region 로직 가상 함수
public:
	/**
	 * @brief 캐릭터 데이터를 받아 UI를 갱신합니다.
	 * @param InData 표시할 캐릭터 데이터 구조체
	 */
	virtual void UpdateSlot(const FSquadItemUIData& InData) override;
#pragma endregion 로직 가상 함수

#pragma region 자식 UI 바인딩
protected:
	/** @brief 캐릭터 레벨 표시 텍스트 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Level = nullptr;
#pragma endregion 자식 UI 바인딩
};