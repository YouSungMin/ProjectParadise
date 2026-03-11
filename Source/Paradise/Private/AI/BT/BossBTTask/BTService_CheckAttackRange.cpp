// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/BossBTTask/BTService_CheckAttackRange.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/AIUnit/SkillCasterUnit.h"
UBTService_CheckAttackRange::UBTService_CheckAttackRange()
{
	NodeName = TEXT("Check Attack Range (Service)");
	Interval = 0.1f; // 🚨 0.1초마다 매우 빠르게 감시해서 즉각 반응하게 만듭니다.
	RandomDeviation = 0.0f;
}

void UBTService_CheckAttackRange::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return;

	ASkillCasterUnit* BossChar = Cast<ASkillCasterUnit>(AIController->GetPawn());
	if (!BossChar) return;

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp) return;

	AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName));
	if (!TargetActor)
	{
		BlackboardComp->SetValueAsBool(ResultBoolKey.SelectedKeyName, false);
		return;
	}

	// 1. 사거리 및 거리 계산
	float AttackRange = (ActionIndex == -1) ? BossChar->GetAttackRange() : BossChar->GetSkillActionData(ActionIndex).Stats.AttackRange;
	float DistanceToTarget = BossChar->GetDistanceTo(TargetActor);
	bool bIsInRange = DistanceToTarget <= (AttackRange + AcceptableRadius);

	// 2. 🚨 블랙보드 값 업데이트 (이전 상태와 다를 때만 갱신하여 최적화 및 강제 취소 유발)
	if (BlackboardComp->GetValueAsBool(ResultBoolKey.SelectedKeyName) != bIsInRange)
	{
		BlackboardComp->SetValueAsBool(ResultBoolKey.SelectedKeyName, bIsInRange);
	}
}
