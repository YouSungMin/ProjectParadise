// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Data/Structs/CombatTypes.h"
#include "Data/Enums/GameEnums.h"
#include "GameplayTagContainer.h"
#include "CombatInterface.generated.h"


class UFXDataAsset;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UCombatInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PARADISE_API ICombatInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	/**
	 * @brief 공격할때 필요한 데이터를 요청하는 함수
	 * @param ActionType 평타인지 스킬인지 구분 (이 값에 따라 반환되는 데미지 계수가 달라짐)
	 * @return FCombatActionData (몽타주, GE 클래스, 데미지 계수 등)
	 */
	virtual FCombatActionData GetCombatActionData(ECombatActionType ActionType) const = 0;

	/**
	 * @brief 특정 상황(EventType)에 맞는 최종 연출 데이터(Payload)를 반환합니다.
	 * @param EventType 피격, 공격, 사망 등 요청할 연출의 종류
	 * @return FFXPayload* 사운드, 파티클 등이 담긴 구조체 포인터 (없으면 nullptr)
	 */
	virtual struct FFXPayload* GetFXPayload(EFXEventType EventType) const = 0;
};
