// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/BossBTTask/BTTaskNode_BossGASAttack.h"
#include "Paradise/Paradise.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"

UBTTaskNode_BossGASAttack::UBTTaskNode_BossGASAttack()
{
	NodeName = TEXT("Boss GAS Action (By Tag)");
	bNotifyTick = true;
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
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(BossPawn);

	if (ASC && AbilityTagToActivate.IsValid())
	{
		//에디터에서 세팅한 태그 어빌리티 발동
		bool bSuccess = ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AbilityTagToActivate));

		if (bSuccess)
		{
			return EBTNodeResult::InProgress;
		}
	}

	return EBTNodeResult::Failed;
}

void UBTTaskNode_BossGASAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AICon = OwnerComp.GetAIOwner();
	if (!AICon) return;

	APawn* BossPawn = AICon->GetPawn();
	if (!BossPawn) return;

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(BossPawn);
	if (!ASC)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 우리가 실행했던 태그의 스킬이 아직 실행 중(Active)인지 검사합니다.
	bool bIsAbilityActive = false;

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->AbilityTags.HasTag(AbilityTagToActivate))
		{
			if (Spec.IsActive())
			{
				bIsAbilityActive = true;
				break;
			}
		}
	}

	// 스킬 실행이 모두 끝났다면 (몽타주 종료 등)
	if (!bIsAbilityActive)
	{
		// 이 노드 작업을 성공적으로 끝마쳤다고 BT에게 알려주고, 비로소 다음 노드로 넘깁니다!
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
