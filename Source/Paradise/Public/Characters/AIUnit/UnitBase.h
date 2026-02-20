// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/ObjectPoolInterface.h"
#include "Data/Structs/UnitStructs.h"
#include "Characters/Base/CharacterBase.h"
#include "GameplayTagContainer.h"
#include "UnitBase.generated.h"

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

	/** @brief 유닛 초기화 및 ID 설정 */
	void InitializeUnit(struct FAIUnitStats* InStats, struct FAIUnitAssets* InAssets);


	void SetUnitID(FName InID) { UnitID = InID; }
	FName GetUnitID() const { return UnitID; }
	FGameplayTag GetFactionTag()const {
		return FactionTag;
	}
	virtual void Die()override;

	UFUNCTION(BlueprintCallable, Category = "Unit|Logic")
	bool IsEnemy(AUnitBase* OtherUnit);

	void PlayRangeAttack();
protected:
	/** @brief 데이터 테이블 조회를 위한 RowName 저장 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|Data")
	FName UnitID;

	/** @brief 기본 공격 데미지 이펙트 (GE) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Cache")
	TSubclassOf<UGameplayEffect> CachedDamageEffectClass;

	/** @brief 기본 공격 몽타주 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Cache")
	TObjectPtr<UAnimMontage> CachedAttackMontage;

	/** @brief 캐싱된 원거리 투사체 클래스 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Cached")
	TSubclassOf<AActor> CachedProjectileClass;

	/** @brief 캐싱된 공격 사거리 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Cached")
	float CachedAttackRange = 150.0f;

	// =========================================================
	// GAS Handles (어빌리티 관리)
	// =========================================================

	/** @brief 평타 어빌리티 핸들 */
	FGameplayAbilitySpecHandle BasicAbilityHandle;

	/** @brief 스킬 어빌리티 핸들 목록 (보스/패밀리어 용) */
	TArray<FGameplayAbilitySpecHandle> SkillAbilityHandles;

	UFUNCTION(BlueprintCallable, Category = "AI|Movement")
	void SetAvoidanceEnabled(bool bEnable);
};