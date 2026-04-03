// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_IsInRange.generated.h"

/**
 * 
 */
UCLASS()
class PARADISE_API UBTDecorator_IsInRange : public UBTDecorator
{
	GENERATED_BODY()
public:
	UBTDecorator_IsInRange();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

public:
	// 타겟 액터를 가리킬 블랙보드 키
	UPROPERTY(EditAnywhere, Category = "Condition")
	struct FBlackboardKeySelector TargetActorKey;

	// 사거리 값을 가진 블랙보드 키 (또는 유닛에서 직접 가져와도 무방함)
	UPROPERTY(EditAnywhere, Category = "Condition")
	struct FBlackboardKeySelector AttackRangeKey;

	// 타겟이 살짝 움직여서 공격이 취소되는 것을 방지하는 오차 허용치
	UPROPERTY(EditAnywhere, Category = "Condition")
	float AcceptableRadius = 70.0f;
};
