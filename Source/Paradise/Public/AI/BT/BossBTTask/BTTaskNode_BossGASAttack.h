// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "Data/Enums/GameEnums.h"
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

protected:

	/** 발동할 GAS 어빌리티 InputID */
	UPROPERTY(EditAnywhere, Category = "GAS")
	EInputID ActionInputID = EInputID::Attack;
};
