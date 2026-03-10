// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "GameplayTagContainer.h"
#include "BTDecorator_CanUseAbilityByTag.generated.h"

/**
 * 
 */
UCLASS()
class PARADISE_API UBTDecorator_CanUseAbilityByTag : public UBTDecorator
{
	GENERATED_BODY()
	
public:
	UBTDecorator_CanUseAbilityByTag();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

protected:
	/** 사용 가능한지 검사할 어빌리티 태그 (예: Ability.Monster.Skill.1) */
	UPROPERTY(EditAnywhere, Category = "GAS")
	FGameplayTag AbilityTagToCheck;
};
