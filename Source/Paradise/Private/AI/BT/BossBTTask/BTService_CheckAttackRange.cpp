// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/BossBTTask/BTService_CheckAttackRange.h"
#include "AIController.h"
#include "Paradise/Paradise.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/AIUnit/SkillCasterUnit.h"
UBTService_CheckAttackRange::UBTService_CheckAttackRange()
{
	NodeName = TEXT("Check Attack Range (Service)");
	Interval = 0.2f; // 🚨 0.1초마다 매우 빠르게 감시해서 즉각 반응하게 만듭니다.
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

	// 1. 사거리 값 꺼내기 및 액션 이름 설정
	float AttackRange = 0.0f;
	FString ActionName = TEXT("평타"); // 로그 출력을 위한 이름

	if (ActionIndex == -1)
	{
		AttackRange = BossChar->GetAttackRange();
	}
	else
	{
		AttackRange = BossChar->GetSkillActionData(ActionIndex).Stats.AttackRange;
		ActionName = FString::Printf(TEXT("%d번 스킬"), ActionIndex);
	}

	//콜라이더 반경(Radius)
	float BossRadius = BossChar->GetSimpleCollisionRadius();
	float TargetRadius = TargetActor->GetSimpleCollisionRadius();

	//실제 거리 계산 (중심점 기준)
	float DistanceToTarget = BossChar->GetHorizontalDistanceTo(TargetActor); // (추천: 2D 수평 거리)

	//사거리 진입 여부 계산 (콜라이더 겉표면 기준)
	float RequiredDistance = AttackRange + AcceptableRadius + BossRadius + TargetRadius;

	bool bIsInRange = DistanceToTarget <= RequiredDistance;

	//계산된 결과를 블랙보드에 Set
	BlackboardComp->SetValueAsBool(ResultBoolKey.SelectedKeyName, bIsInRange);
}
