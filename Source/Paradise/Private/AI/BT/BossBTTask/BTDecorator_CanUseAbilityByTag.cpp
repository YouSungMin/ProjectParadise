// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/BossBTTask/BTDecorator_CanUseAbilityByTag.h"
#include "AIController.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"


UBTDecorator_CanUseAbilityByTag::UBTDecorator_CanUseAbilityByTag()
{
	NodeName = TEXT("Can Use Ability By Tag");
}

bool UBTDecorator_CanUseAbilityByTag::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	if (!AICon) return false;

	APawn* ControlledPawn = AICon->GetPawn();
	if (!ControlledPawn) return false;

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ControlledPawn);
	if (!ASC || !AbilityTagToCheck.IsValid()) return false;

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->AbilityTags.HasTag(AbilityTagToCheck))
		{
			FGameplayTagContainer FailureTags;
			bool bCanActivate = Spec.Ability->CanActivateAbility(Spec.Handle, ASC->AbilityActorInfo.Get(), &FailureTags);

			if (bCanActivate)
			{
				UE_LOG(LogTemp, Log, TEXT("✅ [보스 AI] '%s' 실행 가능!"), *AbilityTagToCheck.ToString());
				return true;
			}
			else
			{
				// 🚨 [추가된 디버깅 로직] 실패 사유 분석
				if (Spec.IsActive())
				{
					// 스킬이 이미 켜져있는 상태일 때
					UE_LOG(LogTemp, Error, TEXT("🛑 [보스 AI] '%s' 불가 사유: 스킬이 이미 실행 중입니다! (블루프린트에서 EndAbility가 제대로 호출되었는지 확인하세요)"), *AbilityTagToCheck.ToString());
				}
				else if (FailureTags.IsEmpty())
				{
					// 켜져있지도 않고 태그도 없을 때 (코스트/쿨타임 GE 세팅 누락 등)
					UE_LOG(LogTemp, Error, TEXT("❓ [보스 AI] '%s' 불가 사유: 시스템 거부 (태그 없음). ActorInfo가 깨졌거나, Cooldown GE 설정이 잘못되었을 수 있습니다."), *AbilityTagToCheck.ToString());
				}
				else
				{
					// 정상적인 쿨타임/마나 부족 등
					UE_LOG(LogTemp, Warning, TEXT("⏳ [보스 AI] '%s' 불가 사유: %s"), *AbilityTagToCheck.ToString(), *FailureTags.ToStringSimple());
				}

				return false;
			}
		}
	}

	return false;
}