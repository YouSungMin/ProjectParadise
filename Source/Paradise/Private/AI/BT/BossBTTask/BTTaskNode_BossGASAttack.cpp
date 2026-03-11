// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/BossBTTask/BTTaskNode_BossGASAttack.h"
#include "Paradise/Paradise.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"

UBTTaskNode_BossGASAttack::UBTTaskNode_BossGASAttack()
{
	NodeName = TEXT("Boss GAS Action (By Tag)");
	bNotifyTick = false;
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTaskNode_BossGASAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	if (!AICon) return EBTNodeResult::Failed;

	APawn* BossPawn = AICon->GetPawn();
	if (!BossPawn) return EBTNodeResult::Failed;

	//적을 바라보게 회전
	AActor* Target = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(TargetKey.SelectedKeyName));
	if (Target)
	{
		FVector LookDir = Target->GetActorLocation() - BossPawn->GetActorLocation();
		LookDir.Z = 0.f;
		BossPawn->SetActorRotation(LookDir.Rotation());
	}

	//ASC 가져오기
	CachedASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(BossPawn);
	if (!CachedASC || !AbilityTagToActivate.IsValid())
	{
		return EBTNodeResult::Failed;
	}

	CachedOwnerComp = &OwnerComp;


	AbilityEndedDelegateHandle = CachedASC->OnAbilityEnded.AddUObject(this, &UBTTaskNode_BossGASAttack::OnAbilityEnded);


	bool bSuccess = CachedASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AbilityTagToActivate));

	if (bSuccess)
	{
		// 성공했으니 트리는 틱 연산 없이 완전한 수면(대기) 상태로 들어갑니다.
		return EBTNodeResult::InProgress;
	}
	else
	{
		// 발동 실패 시 찌꺼기가 남지 않게 구독 취소 후 실패 반환
		CachedASC->OnAbilityEnded.Remove(AbilityEndedDelegateHandle);
		return EBTNodeResult::Failed;
	}
}

EBTNodeResult::Type UBTTaskNode_BossGASAttack::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (CachedASC && AbilityEndedDelegateHandle.IsValid())
	{
		CachedASC->OnAbilityEnded.Remove(AbilityEndedDelegateHandle);
		AbilityEndedDelegateHandle.Reset();
	}

	return Super::AbortTask(OwnerComp, NodeMemory);
}

void UBTTaskNode_BossGASAttack::OnAbilityEnded(const FAbilityEndedData& AbilityEndedData)
{
	if (AbilityEndedData.AbilityThatEnded && AbilityEndedData.AbilityThatEnded->AbilityTags.HasTag(AbilityTagToActivate))
	{
		// 더 이상 연락받지 않도록 구독 취소 (메모리 누수 방지)
		if (CachedASC && AbilityEndedDelegateHandle.IsValid())
		{
			CachedASC->OnAbilityEnded.Remove(AbilityEndedDelegateHandle);
			AbilityEndedDelegateHandle.Reset();
		}

		// 자고 있던 트리를 깨워서 다음 노드(Move To 등)로 넘김!
		if (CachedOwnerComp)
		{
			FinishLatentTask(*CachedOwnerComp, EBTNodeResult::Succeeded);
		}
	}
}

