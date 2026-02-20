// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Calculations/ExecCalcCombat.h"
#include "GAS/Attributes/BaseAttributeSet.h"
#include "GAS/System/ParadiseGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Characters/Base/CharacterBase.h"
#include "GameplayEffectTypes.h"

struct FParadiseDamageStatics
{
	// 필요한 속성 정의
	DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Defense);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CritRate);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CritDamage);
	DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingDamage);

	FParadiseDamageStatics()
	{
		// 스냅샷 여부 설정 (true: GE 생성 시점 값, false: 적용 시점 값)

		// Source(공격자)의 스탯
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, AttackPower, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, CritRate, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, CritDamage, Source, false);

		// Target(피해자)의 스탯
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, Defense, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, IncomingDamage, Target, false);
	}
};

static const FParadiseDamageStatics& DamageStatics()
{
	static FParadiseDamageStatics DStatics;
	return DStatics;
}

// 생성자: 캡처할 속성 등록
UExecCalcCombat::UExecCalcCombat()
{
	RelevantAttributesToCapture.Add(DamageStatics().AttackPowerDef);
	RelevantAttributesToCapture.Add(DamageStatics().DefenseDef);
	RelevantAttributesToCapture.Add(DamageStatics().CritRateDef);
	RelevantAttributesToCapture.Add(DamageStatics().CritDamageDef);
	RelevantAttributesToCapture.Add(DamageStatics().IncomingDamageDef);
}

void UExecCalcCombat::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	// 필수 데이터 가져오기 (태그, ASC 등)
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	// 태그 컨테이너 (버프/디버프 확인용)
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();


	FAggregatorEvaluateParameters EvalParams;

	EvalParams.SourceTags = SourceTags; // 공격자의 태그 정보 전달
	EvalParams.TargetTags = TargetTags; // 피해자의 태그 정보 전달

	// =========================================================
	//  기초 데미지 계산 (Base Damage)
	// =========================================================

	// 공격력 가져오기
	float AttackPower = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().AttackPowerDef, EvalParams, AttackPower);
	AttackPower = FMath::Max(AttackPower, 0.f); // 음수 방지

	float DamageMultiplier = Spec.GetSetByCallerMagnitude(
		FGameplayTag::RequestGameplayTag(FName("Data.Damage.Multiplier")),
		false,
		1.0f // 못 찾으면 기본값 1.0 (평타)
	);

	// 현재 데미지 누적
	float CurrentDamage = AttackPower * DamageMultiplier;

	// =========================================================
	//  치명타 계산 (Critical Hit)
	// =========================================================

	float CritRate = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CritRateDef, EvalParams, CritRate);
	CritRate = FMath::Clamp(CritRate, 0.f, 1.f); // 0~1 사이 안전장치

	// 랜덤 확률 체크 (0.0 ~ 1.0)
	const bool bIsCritical = FMath::RandRange(0.f, 1.f) <= CritRate;

	if (bIsCritical)
	{
		float CritDamage = 0.f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CritDamageDef, EvalParams, CritDamage);
		CritDamage = FMath::Max(CritDamage, 1.0f); // 최소 1배수 보장

		CurrentDamage *= CritDamage; // 배율 적용

		// 디버그 로그
		// UE_LOG(LogTemp, Warning, TEXT("CRITICAL HIT! Damage: %f"), CurrentDamage);
	}


	// =========================================================
	//  방어력 적용 (Defense)
	// =========================================================

	float Defense = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DefenseDef, EvalParams, Defense);
	Defense = FMath::Max(Defense, 0.f);

	// 비율 감소
	CurrentDamage *= (100.f / (100.f + Defense));

	// 최소 데미지 보장 (방어력이 높아도 최소 1은 들어감)
	CurrentDamage = FMath::Max(CurrentDamage, 1.0f);


	// =========================================================
	//  최종 결과 적용 (Output)
	// =========================================================

	if (CurrentDamage > 0.f)
	{
		// IncomingDamage 속성에 '더하기(Additive)' 연산으로 값 전달
		OutExecutionOutput.AddOutputModifier(
			FGameplayModifierEvaluatedData(
				DamageStatics().IncomingDamageProperty,
				EGameplayModOp::Additive,
				CurrentDamage
			)
		);

		if (AActor* TargetActor = TargetASC->GetAvatarActor())
		{
			if (ACharacterBase* TargetChar = Cast<ACharacterBase>(TargetActor))
			{
				TargetChar->SpawnDamagePopup(CurrentDamage, bIsCritical);
			}
		}
	}
}
