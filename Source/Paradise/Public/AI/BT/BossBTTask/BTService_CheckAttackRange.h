// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_CheckAttackRange.generated.h"

/**
 * 
 */
UCLASS()
class PARADISE_API UBTService_CheckAttackRange : public UBTService
{
	GENERATED_BODY()
public:
	UBTService_CheckAttackRange();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

public:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetKey;

	// 🚨 [핵심] 사거리 진입 여부를 저장할 블랙보드 Bool 키
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector ResultBoolKey;

	UPROPERTY(EditAnywhere, Category = "Condition")
	int32 ActionIndex = -1;

	UPROPERTY(EditAnywhere, Category = "Condition")
	float AcceptableRadius = 100.0f;
};
