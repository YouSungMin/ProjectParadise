// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/BossBTTask/BTDecorator_CheckAttackRange.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/AIUnit/SkillCasterUnit.h"
#include "Paradise/Paradise.h"

struct FAttackRangeMemory
{
	bool bWasInRange;
};

UBTDecorator_CheckAttackRange::UBTDecorator_CheckAttackRange()
{
	NodeName = TEXT("Check Attack Range");
	bNotifyTick = false;    
}

bool UBTDecorator_CheckAttackRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return false;

	ASkillCasterUnit* BossChar = Cast<ASkillCasterUnit>(AIController->GetPawn());
	if (!BossChar) return false;

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp) return false;

	AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName));
	if (!TargetActor) return false;

	// 1. 사거리 값 꺼내기
	float AttackRange = 0.0f;
	FString ActionName = TEXT("평타");

	if (ActionIndex == -1) // 🚨 변수명 변경 적용
	{
		AttackRange = BossChar->GetAttackRange();
	}
	else
	{
		AttackRange = BossChar->GetSkillActionData(ActionIndex).Stats.AttackRange; // 🚨 변수명 변경 적용
		ActionName = FString::Printf(TEXT("%d번 스킬"), ActionIndex);
	}

	//보스와 타겟 사이의 거리 계산
	float DistanceToTarget = BossChar->GetDistanceTo(TargetActor);

	//타겟이 "사거리 + 오차 허용치" 안쪽에 들어왔는지 판별
	bool bIsInRange = DistanceToTarget <= (AttackRange + AcceptableRadius);
	
	UE_LOG(LogParadiseAI, Warning, TEXT("📏 [사거리 체크] 대상: %s | 액션: %s | 사거리(%.1f) + 오차(%.1f) vs 현재 거리(%.1f) => 결과: %s"),
		*TargetActor->GetName(),
		*ActionName,
		AttackRange,
		AcceptableRadius,
		DistanceToTarget,
		bIsInRange ? TEXT("✅ 통과 (공격 가능)") : TEXT("❌ 불가 (너무 멂)")
	);

	return bIsInRange;
}

void UBTDecorator_CheckAttackRange::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	FAttackRangeMemory* Memory = (FAttackRangeMemory*)NodeMemory;
	bool bIsNowInRange = CalculateRawConditionValue(OwnerComp, NodeMemory);

	// 사거리 밖(False)에 있다가 -> 사거리 안(True)으로 들어오는 '순간'에만 트리를 강제 재평가!
	if (bIsNowInRange && !Memory->bWasInRange)
	{
		OwnerComp.RequestExecution(this); // "Move To 때려치우고 당장 내 쪽(공격)을 실행해라!"
	}

	// 현재 상태 저장
	Memory->bWasInRange = bIsNowInRange;
}

uint16 UBTDecorator_CheckAttackRange::GetInstanceMemorySize() const
{
	return sizeof(FAttackRangeMemory);
}

void UBTDecorator_CheckAttackRange::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	FAttackRangeMemory* Memory = (FAttackRangeMemory*)NodeMemory;
	Memory->bWasInRange = false;
}
