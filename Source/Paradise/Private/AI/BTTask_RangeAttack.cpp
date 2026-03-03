// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/BTTask_RangeAttack.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/AIUnit/UnitBase.h"

UBTTask_RangeAttack::UBTTask_RangeAttack()
{
	NodeName = "Range Attack";
}

EBTNodeResult::Type UBTTask_RangeAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIC = OwnerComp.GetAIOwner();
    APawn* SelfPawn = AIC->GetPawn();
    AUnitBase* SelfUnit = Cast<AUnitBase>(SelfPawn);
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();

    if (!SelfUnit || !BB) return EBTNodeResult::Failed;

    // 1. 블랙보드에서 타겟 가져오기
    AActor* Target = Cast<AActor>(BB->GetValueAsObject(GetSelectedBlackboardKey()));
    if (!Target) return EBTNodeResult::Failed;

    // 2. 거리 계산
    float Distance = FVector::Dist(SelfPawn->GetActorLocation(), Target->GetActorLocation());

    // 3. 사거리 판단
    if (Distance <= AttackRange)
    {
        // 사거리 안 -> 공격 시작
        AIC->StopMovement(); // 공격을 위해 정지

        // 적을 바라보게 회전
        FVector Direction = Target->GetActorLocation() - SelfPawn->GetActorLocation();
        Direction.Z = 0.f;
        SelfPawn->SetActorRotation(Direction.Rotation());

        // 공격 함수 실행
        SelfUnit->PlayRangeAttack();

        // 성공 반환
        return EBTNodeResult::Succeeded;
    }
    else
    {
        // 사거리 밖 -> 추격
        return EBTNodeResult::Failed;
    }
}