// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Calculations/ExecCalcHeal.h"
#include "GAS/Attributes/BaseAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Characters/Base/CharacterBase.h"

struct FParadiseHealStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Health);

	FParadiseHealStatics()
	{
		// 시전자(Source: 힐을 주는 사람)의 공격력을 캡처
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, AttackPower, Source, false);

		// 대상(Target: 힐을 받는 사람)의 체력 속성을 캡처
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, Health, Target, false);
	}
};

static const FParadiseHealStatics& HealStatics()
{
	static FParadiseHealStatics HStatics;
	return HStatics;
}

UExecCalcHeal::UExecCalcHeal()
{
	RelevantAttributesToCapture.Add(HealStatics().AttackPowerDef);
}

void UExecCalcHeal::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	FAggregatorEvaluateParameters EvalParams;

	float AttackPower = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(HealStatics().AttackPowerDef, EvalParams, AttackPower);
	AttackPower = FMath::Max(AttackPower, 0.f);

	// 엑셀에서 넘어온 힐 배율(DamageMultiplier 재활용) 꺼내기
	float HealMultiplier = ExecutionParams.GetOwningSpec().GetSetByCallerMagnitude(
		FGameplayTag::RequestGameplayTag(FName("Data.Damage.Multiplier")),
		false,
		1.0f
	);

	// 최종 힐량 계산 = 시전자 공격력 * 엑셀 배율
	float FinalHeal = AttackPower * HealMultiplier;

	//UE_LOG(LogTemp, Log, TEXT("FinalHeal %.1f"),FinalHeal);

	if (FinalHeal > 0.f)
	{
		// 대상의 체력(Health) 속성에 Additive(더하기) 연산으로 결과 전달
		OutExecutionOutput.AddOutputModifier(
			FGameplayModifierEvaluatedData(
				HealStatics().HealthProperty,
				EGameplayModOp::Additive,
				FinalHeal
			)
		);

		// 디버그용 로그
		//UE_LOG(LogTemp, Log, TEXT("💚 [힐 적용] 공격력(%.1f) * 배율(%.1f) = 총 힐량: %.1f"), AttackPower, HealMultiplier, FinalHeal);
	}
}
