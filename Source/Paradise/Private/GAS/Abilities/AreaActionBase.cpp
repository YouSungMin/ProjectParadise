// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/AreaActionBase.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Characters/Base/CharacterBase.h"

UAreaActionBase::UAreaActionBase()
{
	// 기본적으로 감지할 태그 설정
	HitEventTag = FGameplayTag::RequestGameplayTag(FName("Event.Montage.ApplyEffect"));
}

void UAreaActionBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 코스트 및 쿨타임 확인 (Commit)
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 데이터 가져오기 (인터페이스 사용)
	// BaseGameplayAbility에서 만든 함수가 캐싱된 데이터를 줍니다.
	FCombatActionData CombatData = GetCombatDataFromActor();

	if (!CombatData.MontageToPlay)
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ [AreaActionBase] 재생할 몽타주가 없습니다."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (ACharacterBase* AvatarChar = Cast<ACharacterBase>(GetAvatarActorFromActorInfo()))
	{
		AvatarChar->SetCurrentActionData(CombatData);
	}

	PlayMontageAndWaitCallback(CombatData.MontageToPlay);

	// 타격 이벤트 대기 (WaitGameplayEvent)
	UAbilityTask_WaitGameplayEvent* EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, HitEventTag, nullptr, false, false
	);

	EventTask->EventReceived.AddDynamic(this, &UAreaActionBase::OnGameplayEventReceived);
	EventTask->ReadyForActivation();
}

void UAreaActionBase::OnGameplayEventReceived(FGameplayEventData Payload)
{
	// 맞은 대상(Target) 확인
	AActor* TargetActor = const_cast<AActor*>(Payload.Target.Get());
	if (!TargetActor) return;

	// 데이터 다시 조회 (Base 클래스에서 캐싱해주므로 비용 걱정 없음)
	FCombatActionData CombatData = GetCombatDataFromActor();

	// GE 클래스가 없으면 데미지 못 줌
	if (!CombatData.EffectClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠️ [AreaActionBase] DamageEffectClass가 설정되지 않았습니다."));
		return;
	}

	// GE 스펙 생성 (Make Spec)
	// BaseGameplayAbility에 구현된 Helper 함수 사용
	FGameplayEffectSpecHandle SpecHandle = MakeSpecHandle(CombatData.EffectClass, GetAbilityLevel());

	if (SpecHandle.IsValid())
	{
		// 데미지 계수 전달 (SetByCaller)
		SpecHandle.Data->SetSetByCallerMagnitude(
			FGameplayTag::RequestGameplayTag(FName("Data.Damage.Multiplier")),
			CombatData.Stats.DamageMultiplier
		);

		if (CombatData.Stats.BuffDuration > 0.0f) // 또는 CombatData.Stats.BuffDuration
		{
			SpecHandle.Data->SetSetByCallerMagnitude(
				FGameplayTag::RequestGameplayTag(FName("Data.Buff.Duration")),
				CombatData.Stats.BuffDuration
			);
		}
		// 적용 (Apply)
		ApplySpecHandleToTarget(TargetActor, SpecHandle);
	}
}
