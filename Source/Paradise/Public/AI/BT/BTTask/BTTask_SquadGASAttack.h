// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_SquadGASAttack.generated.h"

/**
 * 
 */
UCLASS()
class PARADISE_API UBTTask_SquadGASAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_SquadGASAttack();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
};
