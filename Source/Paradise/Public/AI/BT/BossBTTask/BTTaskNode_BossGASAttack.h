// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTaskNode_BossGASAttack.generated.h"

/**
 * 
 */
UCLASS()
class PARADISE_API UBTTaskNode_BossGASAttack : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTaskNode_BossGASAttack();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
protected:

	/** 에디터에서 보스가 발동할 어빌리티 태그 (Ability.Type.BossPattern 서브태그)지정 */
	UPROPERTY(EditAnywhere, Category = "GAS")
	FGameplayTag AbilityTagToActivate;

	/** 타겟을 바라보기 위해 블랙보드에서 타겟을 가져올 키 */
	UPROPERTY(EditAnywhere, Category = "GAS")
	FBlackboardKeySelector TargetKey;
};
