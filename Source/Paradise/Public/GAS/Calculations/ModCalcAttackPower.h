// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "ModCalcAttackPower.generated.h"

/**
 * @class UModCalcAttackPower
 * @brief 시전자(Source)의 공격력에 비례하여 타겟(Target)의 공격력 버프 수치를 동적으로 계산하는 MMC 클래스입니다.
 * @details
 * 엑셀(SetByCaller)에서 전달받은 '기본 배율'에, 시전자의 공격력에 비례한 '추가 배율(스케일링)'을 합산하여 최종 버프 수치를 산출합니다.
 * 버프가 자기 자신을 끝없이 증폭시키는 현상(Feedback Loop)을 방지하기 위해, 타겟과 시전자의 스탯은 이펙트 적용 순간에 스냅샷(Snapshot)으로 캡처됩니다.
 */
UCLASS()
class PARADISE_API UModCalcAttackPower : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UModCalcAttackPower();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
private:
	/** @brief 타겟(버프를 받는 대상)의 공격력 캡처 정의
	 *  @note 캡처 시점: 버프 적용 순간 (Snapshot = true)
	 */
	FGameplayEffectAttributeCaptureDefinition TargetAttackPowerDef;

	/** @brief 시전자(버프를 쓰는 대상)의 공격력 캡처 정의
	 *  @note 캡처 시점: 버프 생성/시전 순간 (Snapshot = true)
	 */
	FGameplayEffectAttributeCaptureDefinition SourceAttackPowerDef;
};
