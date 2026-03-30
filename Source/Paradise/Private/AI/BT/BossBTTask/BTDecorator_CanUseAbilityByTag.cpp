// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/BossBTTask/BTDecorator_CanUseAbilityByTag.h"
#include "Paradise/Paradise.h" //로그
#include "AIController.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"


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

	// ASC 가져오기
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ControlledPawn);
	if (!ASC || !AbilityTagToCheck.IsValid()) return false;

	// ASC에 부여된 모든 어빌리티를 순회
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		// 어빌리티가 우리가 찾는 태그를 가지고 있는지 확인
		if (Spec.Ability && Spec.Ability->GetAssetTags().HasTag(AbilityTagToCheck))
		{
			// 실패 사유를 담을 빈 태그 컨테이너
			FGameplayTagContainer FailureTags;

			// 태그가 일치하면, GAS 자체 로직(마나, 쿨타임 검사)을 돌려서 true/false 반환
			bool bCanActivate = Spec.Ability->CanActivateAbility(Spec.Handle, ASC->AbilityActorInfo.Get(), &FailureTags);

			if (bCanActivate)
			{
				//UE_LOG(LogParadiseAI, Log, TEXT("✅ [보스 AI] '%s' 스킬 사용 가능! (조건 통과)"), *AbilityTagToCheck.ToString());
				return true;
			}
			else
			{
				// 🚨 쿨타임 정보를 담을 변수 선언
				float TimeRemaining = 0.f;
				float CooldownDuration = 0.f;

				// 어빌리티로부터 실제 쿨타임 정보를 받아옵니다.
				Spec.Ability->GetCooldownTimeRemainingAndDuration(Spec.Handle, ASC->AbilityActorInfo.Get(), TimeRemaining, CooldownDuration);

				//if (Spec.IsActive())
				//{
				//	// 스킬이 아직 안 끝나서(EndAbility 미호출) 못 쓰는 경우
				//	UE_LOG(LogParadiseAI, Error, TEXT("🛑 [보스 AI] '%s' 불가 사유: 스킬이 이미 실행 중입니다!"), *AbilityTagToCheck.ToString());
				//}
				//else if (TimeRemaining > 0.f)
				//{
				//	// 정확히 쿨타임에 걸려있는 경우 (몇 초 남았는지 출력)
				//	UE_LOG(LogParadiseAI, Warning, TEXT("⏳ [보스 AI] '%s' 쿨타임 대기 중... (남은 시간: %.1f초 / 전체 쿨타임: %.1f초)"),
				//		*AbilityTagToCheck.ToString(), TimeRemaining, CooldownDuration);
				//}
				//else if (!FailureTags.IsEmpty())
				//{
				//	// 마나 부족 등 다른 태그에 의한 실패
				//	UE_LOG(LogParadiseAI, Warning, TEXT("🚫 [보스 AI] '%s' 불가 사유: %s"),
				//		*AbilityTagToCheck.ToString(), *FailureTags.ToStringSimple());
				//}
				//else
				//{
				//	// 그 외 시스템적 실패 (GE 설정 누락 등)
				//	UE_LOG(LogParadiseAI, Error, TEXT("❓ [보스 AI] '%s' 불가 사유: 알 수 없음 (태그 없음)"), *AbilityTagToCheck.ToString());
				//}

				return false;
			}
		}
	}

	return false;
}