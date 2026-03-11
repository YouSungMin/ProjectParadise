// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_CheckAttackRange.generated.h"

/**
 * 
 */
UCLASS()
class PARADISE_API UBTDecorator_CheckAttackRange : public UBTDecorator
{
	GENERATED_BODY()
	

public:
	UBTDecorator_CheckAttackRange();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
public:
	UPROPERTY(EditAnywhere, Category = "Condition")
	FBlackboardKeySelector TargetKey;

	// -1: 평타, 0 이상: 스킬 인덱스
	UPROPERTY(EditAnywhere, Category = "Condition", meta = (ToolTip = "평타 사거리를 검사하려면 -1, 스킬은 0 이상의 인덱스를 입력하세요."))
	int32 ActionIndex = -1;

	UPROPERTY(EditAnywhere, Category = "Condition")
	float AcceptableRadius = 50.0f;
};
