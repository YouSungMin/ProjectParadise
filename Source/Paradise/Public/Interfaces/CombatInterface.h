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
	 * @brief 이 유닛의 고유 연출 데이터 에셋(FXDataAsset)을 반환합니다.
	 */
	virtual UFXDataAsset* GetUnitFXData() const = 0;

	/**
	 * @brief 이 유닛이 피격당했을 때 재생할 고유 피격 태그를 반환합니다. (예: State.Hit)
	 */
	virtual FGameplayTag GetHitReactionTag() const = 0;
};
