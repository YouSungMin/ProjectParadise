// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_CalcLaneTarget.generated.h"

/**
 * 
 */
UCLASS()
class PARADISE_API UBTService_CalcLaneTarget : public UBTService
{
	GENERATED_BODY()
	
public:
	UBTService_CalcLaneTarget();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;       // MyAIController가 감지한 적

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector EnemyBaseKey;         // 적 기지

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector MoveDestinationKey;   // 최종 계산된 이동 좌표

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector AssignedLaneYKey;     // 스폰 시 할당된 레인 Y좌표
};
