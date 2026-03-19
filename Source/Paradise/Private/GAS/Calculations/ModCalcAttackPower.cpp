// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Calculations/ModCalcAttackPower.h"
#include "GAS/Attributes/BaseAttributeSet.h"

UModCalcAttackPower::UModCalcAttackPower()
{
	// 타겟의 공격력 캡처
	TargetAttackPowerDef = FGameplayEffectAttributeCaptureDefinition(
		UBaseAttributeSet::GetAttackPowerAttribute(),
		EGameplayEffectAttributeCaptureSource::Target,
		true	// 스냅 샷 true
	);

	// 소스의 공격력 캡처
	SourceAttackPowerDef = FGameplayEffectAttributeCaptureDefinition(
		UBaseAttributeSet::GetAttackPowerAttribute(),
		EGameplayEffectAttributeCaptureSource::Source,
		true
	);

	RelevantAttributesToCapture.Add(TargetAttackPowerDef);
	RelevantAttributesToCapture.Add(SourceAttackPowerDef);
}

float UModCalcAttackPower::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	
	// 엑셀에서 넘어온 기본 퍼센트 (예: 0.1 = 10%)
	float BaseMultiplier = Spec.GetSetByCallerMagnitude(
		FGameplayTag::RequestGameplayTag(FName("Data.Damage.Multiplier")), false, 0.1f);

	// 시전자(나)의 현재 공격력 꺼내기
	float SourceAttackPower = 0.f;
	GetCapturedAttributeMagnitude(SourceAttackPowerDef, Spec, EvalParams, SourceAttackPower);
	SourceAttackPower = FMath::Max(SourceAttackPower, 0.f);

	// 타겟(아군 또는 본인)의 현재 공격력 꺼내기
	float TargetAttackPower = 0.f;
	GetCapturedAttributeMagnitude(TargetAttackPowerDef, Spec, EvalParams, TargetAttackPower);

	// 예: 시전자의 공격력 100당 버프 효율이 2%(0.02)씩 증가함
	float BonusMultiplier = (SourceAttackPower / 100.0f) * 0.02f;

	// 최종 퍼센트 = (엑셀 기본 퍼센트) + (공격력 비례 추가 퍼센트)
	float FinalMultiplier = BaseMultiplier + BonusMultiplier;

	float FinalBuffAmount = TargetAttackPower * FinalMultiplier;

	// 디버그용 로그
	UE_LOG(LogTemp, Warning, TEXT("⚔️ [공격력 버프 연산] 기본(%.0f%%) + 보너스(%.0f%%) = 총 %.0f%% 적용! (실제 증가량: %.1f)"),
		BaseMultiplier * 100.f, BonusMultiplier * 100.f, FinalMultiplier * 100.f, FinalBuffAmount);

	return FinalBuffAmount;
}
