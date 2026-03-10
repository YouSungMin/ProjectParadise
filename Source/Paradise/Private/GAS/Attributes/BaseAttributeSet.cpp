// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Attributes/BaseAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Characters/Base/CharacterBase.h"

UBaseAttributeSet::UBaseAttributeSet()
{
	// 생성자: 안전을 위해 기본값 초기화 (데이터 테이블 로딩 전 임시값)
	InitHealth(100.0f);
	InitMaxHealth(100.0f);
	InitMana(0.0f);
	InitMaxMana(20.0f);

	InitAttackPower(10.0f);
	InitDefense(0.0f);

	InitCritRate(0.0f);
	InitCritDamage(1.5f); // 기본 치명타 피해 150%

	InitMoveSpeed(400.0f);
	InitAttackSpeed(1.0f);
	InitCooldown(0.0f);
	InitAttackRange(100.0f);
}

void UBaseAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// 체력 (Health)
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	// 마나 (Mana)
	else if (Attribute == GetManaAttribute())
	{
		float CurrentMana = GetMana();

		// 2. 최대 마나를 넘지 않게, 0 밑으로 떨어지지 않게 제한합니다.
		float ClampedNewValue = FMath::Clamp(NewValue, 0.f, GetMaxMana());

		// 3. 💡 마나가 깎였을 때만(ClampedNewValue가 CurrentMana보다 작을 때만) 로그를 출력합니다.
		if (ClampedNewValue < CurrentMana)
		{
			// 사용한(깎인) 마나 계산
			float UsedMana = CurrentMana - ClampedNewValue;

			UE_LOG(LogTemp, Log, TEXT("💧 [마나 차감] 사용한 마나: %.1f / 남은 마나: %.1f"), UsedMana, ClampedNewValue);
		}
	}
	// 치명타 확률 (CritRate)
	else if (Attribute == GetCritRateAttribute())
	{
		// 0.0(0%) ~ 1.0(100%) 사이로 제한
		NewValue = FMath::Clamp(NewValue, 0.0f, 1.0f);
	}
	// 치명타 피해 (CritDamage)
	else if (Attribute == GetCritDamageAttribute())
	{
		// 최소 1.0배(100%) 이상이어야 함 (0배면 데미지가 증발하므로)
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	// 공격 속도 (AttackSpeed)
	else if (Attribute == GetAttackSpeedAttribute())
	{
		// 0.1배 미만으로 내려가면 애니메이션이 멈추거나 역재생될 수 있음
		NewValue = FMath::Max(NewValue, 0.1f);
	}
	// 사거리, 이속, 쿨타임, 공격력, 방어력 등
	else if (Attribute == GetAttackPowerAttribute())
	{
		float CurrentAttackPower = GetAttackPower(); // 변하기 전의 현재 공격력
		NewValue = FMath::Max(NewValue, 0.0f);       // 음수 방지

		// 값이 실제로 변했을 때만 로그를 찍도록 설정 (소수점 오차 무시)
		if (!FMath::IsNearlyEqual(CurrentAttackPower, NewValue))
		{
			float Difference = NewValue - CurrentAttackPower;
			UE_LOG(LogTemp, Warning, TEXT("⚔️ [버프/디버프] 공격력 변경! 이전: %.1f ➡️ 현재: %.1f (변화량: %+.1f)"),
				CurrentAttackPower, NewValue, Difference);
		}
	}
	// 사거리, 이속, 쿨타임, 방어력 등
	else if (Attribute == GetAttackRangeAttribute() ||
		Attribute == GetMoveSpeedAttribute() ||
		Attribute == GetCooldownAttribute() ||
		Attribute == GetDefenseAttribute())
	{
		// 음수 방지
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UBaseAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float LocalDamage = GetIncomingDamage();
		SetIncomingDamage(0.f);

		if (LocalDamage > 0.f)
		{
			const float NewHealth = GetHealth() - LocalDamage;
			SetHealth(NewHealth);

			// 1. 타겟 액터가 누구인지 확인
			AActor* TargetActor = Data.Target.GetAvatarActor();
			UE_LOG(LogTemp, Warning, TEXT("===================================="));
			UE_LOG(LogTemp, Warning, TEXT("🩸 [데미지 판정] 타겟 액터: %s, 남은 HP: %.2f"), TargetActor ? *TargetActor->GetName() : TEXT("Null"), NewHealth);

			// 2. 캐릭터 베이스로 캐스팅 시도
			if (ACharacterBase* Character = Cast<ACharacterBase>(TargetActor))
			{
				if (NewHealth <= 0.0f)
				{
					UE_LOG(LogTemp, Warning, TEXT("💀 [데미지 판정] 타겟 사망! -> Die() 호출"));
					if (!Character->IsDead())
					{
						Character->Die();
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("🎯 [데미지 판정] 타겟 생존! -> PlayHitReaction() 호출 시도!"));
					Character->PlayHitReaction();
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("❌ [데미지 판정] 타겟 액터를 ACharacterBase로 캐스팅 실패!"));
			}
			UE_LOG(LogTemp, Warning, TEXT("===================================="));
		}
	}
}

void UBaseAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	// 값이 실제로 변했을 때만 디버그 로그 출력
	if (OldValue != NewValue)
	{
		// 어떤 스탯인지, 몇에서 몇으로 변했는지, 증감량은 얼마인지 출력!
		UE_LOG(LogTemp, Warning, TEXT("📈 [스탯 변경] %s : %.1f ➡️ %.1f (%+.1f)"),
			*Attribute.GetName(), OldValue, NewValue, NewValue - OldValue);
	}
}
