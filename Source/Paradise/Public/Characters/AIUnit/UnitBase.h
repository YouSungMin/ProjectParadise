// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/ObjectPoolInterface.h"
#include "Data/Structs/UnitStructs.h"
#include "Characters/Base/CharacterBase.h"
#include "GameplayTagContainer.h"
#include "UnitBase.generated.h"


class UAIPerceptionStimuliSourceComponent;//0304 김성현 전방선언 추가
UCLASS()
class PARADISE_API AUnitBase : public ACharacterBase, public IObjectPoolInterface
{
	GENERATED_BODY()
public:
	AUnitBase();

	/** 오브젝트 풀 인터페이스 구현 */
	virtual void OnPoolActivate_Implementation() override;
	virtual void OnPoolDeactivate_Implementation() override;

	/**
	 * @brief 실제 전투 데이터를 조회하는 함수
	 * @details PlayerBase가 호출하면, GameInstance와 EquipmentComponent를 탐색하여 결과 리턴.
	 */
	virtual FCombatActionData GetCombatActionData(ECombatActionType ActionType) const override;

	/** @brief 특정 상황(EventType)에 맞는 최종 연출 데이터(Payload)를 반환합니다. (기본값: nullptr) */
	virtual struct FFXPayload* GetFXPayload(EFXEventType EventType) const override;

	/** @brief 유닛 초기화 및 ID 설정 */
	virtual void InitializeUnit(struct FAIUnitStats* InStats, struct FAIUnitAssets* InAssets);

	const FAIUnitAssets& GetUnitAssets() const { return UnitAssets; }

	void SetUnitID(FName InID) { UnitID = InID; }
	FName GetUnitID() const { return UnitID; }
	FGameplayTag GetFactionTag()const {
		return FactionTag;
	}
	virtual void Die()override;

	UFUNCTION(BlueprintCallable, Category = "Unit|Logic")
	bool IsEnemy(AUnitBase* OtherUnit);

	void PlayRangeAttack();

	/** @brief 캐싱된 기본 공격 사거리 반환 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	float GetAttackRange() const { return BasicAttackData.Stats.AttackRange; }

	/** @brief 부모의 가상 함수 오버라이드 */
	virtual UAnimMontage* GetDeathMontage() const override;

	/** @brief 부모의 가상 함수 오버라이드 */
	virtual UAnimMontage* GetHitMontage() const override;
protected:
	// 사망 애니메이션 종료 시 호출됨
	virtual void OnDeathAnimationFinished() override;

	// 실제 풀 반환 로직
	UFUNCTION()
	void ExecuteReturnToPool();
protected:
	/** @brief AI 인지 자극 컴포넌트  */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAIPerceptionStimuliSourceComponent> StimuliSourceComp= nullptr;

	/** @brief 데이터 테이블 조회를 위한 RowName 저장 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|Data")
	FName UnitID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|Data")
	FAIUnitAssets UnitAssets;

	/** @brief 기본 공격에 필요한 전투 데이터 모음 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Data")
	FCombatActionData BasicAttackData;

	/** @brief 피격/사망 등 생존 반응 연출 캐싱 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Cached")
	FReactionFXSettings CachedReactionFX;

	/** @brief 평타/스킬 등 공격 행동 연출 캐싱 (몬스터 본체용) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Cached")
	FActionFXSettings CachedActionFX;

	/** @brief 사망 몽타주 캐싱 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Cached")
	TObjectPtr<UAnimMontage> CachedDeathMontage = nullptr;

	/** @brief 피격 몽타주 캐싱 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Cached")
	TObjectPtr<UAnimMontage> CachedHitMontage = nullptr;

	// =========================================================
	// GAS Handles (어빌리티 관리)
	// =========================================================

	/** @brief 평타 어빌리티 핸들 */
	FGameplayAbilitySpecHandle BasicAbilityHandle;

	UFUNCTION(BlueprintCallable, Category = "AI|Movement")
	void SetAvoidanceEnabled(bool bEnable);

	/** @brief 몽타주 끝의 Notify가 호출되지 않는 버그 방지용 타이머*/
	FTimerHandle FailSafeDestroyTimerHandle;
};