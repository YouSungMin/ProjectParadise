// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Structs/UnitStructs.h"
#include "Data/Structs/ItemStructs.h"
#include "Data/Structs/CombatTypes.h"
#include "AutoCombatComponent.generated.h"


class AInGameController;
class APlayerBase;

/**
 * @class UAutoCombatComponent
 * @brief 플레이어 스쿼드의 자동 전투(소환, 이동, 타겟팅, 스킬 사용)를 관리하는 컴포넌트입니다.
 * @details InGameController에 부착되어 비대해진 자동 전투 로직을 독립적으로 수행합니다.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PARADISE_API UAutoCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** @brief 기본 생성자 */
	UAutoCombatComponent();

	/**
	 * @brief 자동 전투 모드를 활성화하거나 비활성화합니다.
	 * @param bEnable true일 경우 자동 전투(타이머)가 시작되며, false일 경우 중지됩니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Auto Combat")
	void SetAutoBattleMode(bool bEnable);

	/**
	 * @brief 현재 자동 전투 모드가 켜져 있는지 확인합니다.
	 * @return 자동 전투 활성화 상태라면 true를 반환합니다.
	 */
	UFUNCTION(BlueprintPure, Category = "Auto Combat")
	bool IsAutoMode() const { return bIsAutoMode; }

protected:
	/** * @brief 코스트를 확인하고 대기열(0번 슬롯)의 퍼밀리어를 자동으로 소환하는 타이머 함수입니다.
	 */
	void CheckAndAutoSummon();

	/** * @brief 주기적으로 주변의 적을 탐색하고, 이동 및 공격 명령을 갱신하는 핵심 전투 루프 함수입니다.
	 */
	void UpdateAutoCombat();

	/**
	 * @brief 쿨타임 및 마나 상황에 맞춰 최우선 순위 스킬(궁극기 -> 스킬 -> 평타)을 발동합니다.
	 * @param PlayerPawn 액션 명령을 수행할 대상 플레이어 폰
	 */
	void ExecutePrioritizedAction(APlayerBase* PlayerPawn);

	/**
	 * @brief 플레이어 폰을 기준으로 반경 내 가장 가까운 적을 탐색합니다.
	 * @param PlayerPawn 기준이 되는 플레이어 폰
	 * @param OutDistance [OUT] 탐색된 가장 가까운 적과의 실제 거리
	 * @return 탐색된 적 액터 (맵에 적이 없을 경우 nullptr 반환)
	 */
	AActor* FindNearestEnemy(APawn* PlayerPawn, float& OutDistance);

	/**
	 * @brief 맵 상에 존재하는 적의 본진(HomeBase) 액터를 탐색하여 반환합니다.
	 * @return 적 팩션(Faction)의 본진 액터
	 */
	AActor* GetEnemyBase();

private:
	/** * @brief 이 컴포넌트를 소유하고 있는 InGameController를 캐싱하여 가져옵니다.
	 * @return 캐스팅된 AInGameController 포인터
	 */
	AInGameController* GetOwnerController() const;

	/** * @brief 현재 캐릭터가 사용 가능한 액션(궁극기 -> 스킬 -> 평타)을 파악하고,
	 * 데이터 테이블(ActionStats)을 조회하여 해당 액션의 진짜 사거리를 반환합니다.
	 */
	float GetDynamicAttackRange(class APlayerBase* PlayerPawn);

protected:

	/** @brief 무거운 연산을 막기 위해 한 번 찾은 적 기지를 기억해두는 캐싱 포인터 */
	TWeakObjectPtr<class AHomeBase> CachedEnemyBase = nullptr;

private:
	/** @brief 현재 자동 전투 활성화 여부를 나타내는 플래그 */
	bool bIsAutoMode = false;

	/** @brief 자동 소환 로직(CheckAndAutoSummon)을 반복 실행하기 위한 타이머 핸들 */
	FTimerHandle AutoSummonTimerHandle;

	/** @brief 자동 전투 로직(UpdateAutoCombat)을 반복 실행하기 위한 타이머 핸들 */
	FTimerHandle AutoCombatTimerHandle;
};