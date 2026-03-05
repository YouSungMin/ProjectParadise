// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/AIUnit/UnitBase.h"
#include "SkillCasterUnit.generated.h"

/**
 * @class ASkillCasterUnit
 * @brief 평타 외에 여러 개의 액티브 스킬을 사용하는 유닛(보스, 특수 패밀리어 등)의 클래스입니다.
 */
UCLASS()
class ASkillCasterUnit : public AUnitBase
{
	GENERATED_BODY()
public:
	/** @brief AUnitBase의 초기화 함수를 오버라이드 */
	virtual void InitializeUnit(struct FAIUnitStats* InStats, struct FAIUnitAssets* InAssets) override;

	/** @brief 인덱스를 기반으로 특정 스킬의 전투 데이터를 가져옵니다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Skill")
	FCombatActionData GetSkillActionData(int32 SkillIndex) const;

	/** @brief 인덱스를 기반으로 특정 스킬의 FX 연출 데이터(Payload)를 반환합니다. */
	virtual struct FFXPayload* GetSkillFXPayload(int32 SkillIndex) const;

	/** @brief 인덱스를 기반으로 특정 스킬의 몽타주를 가져옵니다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Skill")
	UAnimMontage* GetSkillMontage(int32 SkillIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Combat|Skill")
	void SetCurrentCastingSkillIndex(int32 Index) { CurrentCastingSkillIndex = Index; }

	UFUNCTION(BlueprintCallable, Category = "Combat|Skill")
	int32 GetCurrentCastingSkillIndex() const { return CurrentCastingSkillIndex; }

	// 2. 부모(UnitBase)의 가상 함수 오버라이드!
	virtual FCombatActionData GetCombatActionData(ECombatActionType ActionType) const override;
	virtual struct FFXPayload* GetFXPayload(EFXEventType EventType) const override;
protected:
	/** @brief 스킬 어빌리티 핸들 목록 (부여된 스킬 관리용) */
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> SkillAbilityHandles;

	/** @brief 다중 스킬의 전투 스탯(사거리, 배율 등) 캐싱 배열 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Cached")
	TArray<FCombatActionData> CachedSkillDataArray;

	/** @brief 다중 스킬 연출 태그 캐싱 배열 (EnemyAssets의 SkillEffectTags 복사본) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Cached")
	TArray<FGameplayTag> CachedSkillEffectTags;

	/** @brief 다중 스킬 몽타주 캐싱 배열 (메모리에 로드된 상태 유지) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Cached")
	TArray<TObjectPtr<UAnimMontage>> CachedSkillMontages;

	/** @brief 현재 시전 중인 스킬의 인덱스 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|State")
	int32 CurrentCastingSkillIndex = INDEX_NONE;
};
