// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Unit/BTDecorator_IsInRange.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"

UBTDecorator_IsInRange::UBTDecorator_IsInRange()
{
	NodeName = TEXT("Is Target In Range");

	bNotifyTick = false;
}

bool UBTDecorator_IsInRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	AAIController* AICon = OwnerComp.GetAIOwner();
	if (!BBComp || !AICon || !AICon->GetPawn()) return false;

	APawn* MyPawn = AICon->GetPawn();
	AActor* TargetActor = Cast<AActor>(BBComp->GetValueAsObject(TargetActorKey.SelectedKeyName));

	if (!TargetActor) return false;

	// 1. 블랙보드에서 사거리 값 가져오기
	float AttackRange = BBComp->GetValueAsFloat(AttackRangeKey.SelectedKeyName);

	// 2. 중심에서 중심까지의 거리 계산
	float CenterDistance = MyPawn->GetDistanceTo(TargetActor);

	// 3. 양쪽 액터의 콜리전 반지름(Radius) 가져오기
	float MyRadius = MyPawn->GetSimpleCollisionRadius();
	float TargetRadius = TargetActor->GetSimpleCollisionRadius();

	// 4. 콜리전 끝과 끝(Edge to Edge) 거리 계산
	float EdgeToEdgeDistance = FMath::Max(0.0f, CenterDistance - MyRadius - TargetRadius);

	// 5. 실제 사거리(+오차 허용치) 내에 들어왔는지 판정
	bool bIsInRange = EdgeToEdgeDistance <= (AttackRange + AcceptableRadius);

	return bIsInRange;
}