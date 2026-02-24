// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/BTService_FindClosestTarget.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Characters/AIUnit/UnitBase.h"
#include "EngineUtils.h"

UBTService_FindClosestTarget::UBTService_FindClosestTarget()
{
	NodeName = "Find Closest Target";
	Interval = 0.2f; // 0.2초마다 탐색
}

void UBTService_FindClosestTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	APawn* ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
	AUnitBase* SelfUnit = Cast<AUnitBase>(ControllingPawn);
	if (!SelfUnit) return;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return;

	// 1. 현재 블랙보드에 등록된 타겟을 가져옴
	AActor* CurrentTarget = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	AUnitBase* TargetUnit = Cast<AUnitBase>(CurrentTarget);

	// 2. 이미 타겟이 있고, 그 타겟이 살아있다면
	if (TargetUnit && !TargetUnit->IsDead())
	{
		// 타겟을 바꾸지는 않지만, 거리는 매 틱(0.2초)마다 업데이트
		float CurrentDistance = FVector::Dist(SelfUnit->GetActorLocation(), TargetUnit->GetActorLocation());
		BB->SetValueAsFloat(FName("DistanceToTarget"), CurrentDistance);
		return;
	}

	// 3. 타겟이 없거나 죽었다면 새로운 적을 찾음
	AUnitBase* ClosestEnemy = nullptr;
	float MinDistance = SearchRadius;

	for (TActorIterator<AUnitBase> It(GetWorld()); It; ++It)
	{
		AUnitBase* OtherUnit = *It;

		// 나 자신이 아니고, 죽지 않았으며, 적군인 경우만 체크
		if (OtherUnit != SelfUnit && !OtherUnit->IsDead() && SelfUnit->IsEnemy(OtherUnit))
		{
			float Distance = FVector::Dist(SelfUnit->GetActorLocation(), OtherUnit->GetActorLocation());
			if (Distance < MinDistance)
			{
				MinDistance = Distance;
				ClosestEnemy = OtherUnit;
			}
		}
	}

	// 4. 결과 기록
	if (ClosestEnemy)
	{
		BB->SetValueAsObject(TargetActorKey.SelectedKeyName, ClosestEnemy);
		BB->SetValueAsFloat(FName("DistanceToTarget"), MinDistance);
	}
	else
	{
		// 적이 주변에 아예 없으면 타겟을 비우고 거리를 아주 큰 값으로 설정
		BB->SetValueAsObject(TargetActorKey.SelectedKeyName, nullptr);
		BB->SetValueAsFloat(FName("DistanceToTarget"), 999999.0f);
	}
}