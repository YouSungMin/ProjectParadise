// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "Data/Enums/GameEnums.h"
#include "BTDecorator_CanUseAbility.generated.h"

/**
 * 
 */
UCLASS()
class PARADISE_API UBTDecorator_CanUseAbility : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_CanUseAbility();

protected:

	/** 어빌리티를 찾아서 해당 어빌리티의 조건(마나 ,쿨타임)을 계산하는 함수 */
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;


protected:
	/** 사용가능한지 검사할 어빌리티 액션 ID */
	UPROPERTY(EditAnywhere, Category = "GAS")
	EInputID ActionInputID = EInputID::Skill;
	
};
